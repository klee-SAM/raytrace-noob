#include "Camera.hpp"

using namespace std;
using namespace glm;

void Camera::applyProjection(MatrixStack& MS) {
    MS.mult(perspective(fovy, aspectRatio, znear, zfar));
}
void Camera::applyView(MatrixStack& MS) {
    MS.translate(translation);
    // yaw, pitch, then roll
    MS.rotate(rotation.z, vec3(0.0f, 0.0f, 1.0f));
    MS.rotate(rotation.y, vec3(1.0f, 0.0f, 0.0f));
    MS.rotate(rotation.x, vec3(0.0f, 1.0f, 0.0f));
}

Ray Camera::castPrimaryRay(uint idx, uint idy, double offsetx, double offsety) {
    // https://www.realtimerendering.com/blog/the-center-of-the-pixel-is-0-50-5/

    double ndc_y = 2*((double)idy + offsety)/((double)height) - 1.0;
    double ndc_x = 2*((double)idx + offsetx)/((double)width) - 1.0;

    glm::vec4 coord((float)ndc_x, (float)ndc_y, -1.0f, 1.0f); // px coord
    coord = invP*coord; // eye coord
    coord.w = 1.0f;
    coord = glm::normalize(C*coord - cameraPos); // n_pw

    Ray cray; 
    cray.pos = vec3(cameraPos);
    cray.dir = vec3(coord);

    return cray;
}

int mand;

shared_ptr<Image> Camera::render(
    shared_ptr<Scene> scene, 
    const mat4& P,
    const mat4& V) 
{
    // Precompute as much as possible before loops
    C = inverse(V);
    invP = glm::inverse(P);
    cameraPos = C[3]; 
    cameraPos.w = 1.0f;

    float sample_scale = 1.0/samples;

    uint totalCasts = height*width;

    shared_ptr<Image> image = make_shared<Image>(width, height);

    for (uint y = 0; y < height; ++y) {
        std::clog << '\r' << y*width << '/' << totalCasts << " scans completed " << std::flush;
        for (uint x = 0; x < width; ++x) {
            vec3 color = vec3(0.0f);

            if (samples <= 1) {
                Ray cray = castPrimaryRay(x, y);
                color = getRayColor(scene, cray);
            } else {
                for (uint i = 0; i < samples; ++i) {
                    double dx = linearRand(0.001f, 0.999f);
                    double dy = linearRand(0.001f, 0.999f);
                    Ray cray = castPrimaryRay(x, y, dx, dy);
                    color += getRayColor(scene, cray);
                }
                color *= sample_scale;
            }
            
            image->setPixel(x, y, color);
        }
    }
    clog << mand << '\n';
    std::clog << "\rDone." << std::string(30, ' ') << '\n';
    return image;
}

bool hit(
    vector<shared_ptr<Shape>>& shapes, 
    const Ray& ray, 
    const Interval& interval,
    Hit& closestHit) 
{
    float minDist = interval.max;
    bool intersected_any = false;

    vector<Hit> temp_hits; // maintain a list of hits for csg
    temp_hits.reserve(16); // magic number

    for (shared_ptr<Shape>& shape : shapes) {
        shape->intersect(ray, temp_hits);
        if (temp_hits.empty()) continue;
        for (Hit& hit : temp_hits) {
            if (!interval.contains(hit.t)) continue;
            intersected_any = true;
            
            if (minDist > hit.t) {
                minDist = hit.t;
                closestHit = hit;
            }
        }
        temp_hits.clear();
    }
    return intersected_any;
}

const bool SHOW_NORMALS = false;

vec3 Camera::getSkyColor(const Ray& ray) {
    float cx, cy, cz;
    switch(this->sky) {
    case (Camera::SkyType::Haze):
        cx = .5*(ray.dir.x)+.5;
        cy = .5*(ray.dir.y)+.5;
        cz = .5*(ray.dir.z)+.5;
        return vec3(cx, cz, cy);
        break;
    case (Camera::SkyType::Void):
    default:
        return vec3(0.1f);
        break;
    }
    return vec3(0.0f);
}

Ray reflectRay(const Ray &ray, const Hit &rec) 
{
    Ray reflRay; 
    // ray.dir - 2.0f * dot(rec.n, ray.dir) * rec.n
    reflRay.dir = glm::reflect(ray.dir, rec.n);
    reflRay.pos = rec.x + reflRay.dir*(float)Camera::EPSILION;
    return reflRay;
}

// Creates a new ray with a direction dependant on the material's IoR
// Major assistance from
// https://stackoverflow.com/questions/26087106/refraction-in-raytracing
Ray refractRay(const Ray &ray, const Hit &rec) {
    float n1, n2;
    vec3 norm = rec.n;

    float cosI = dot(ray.dir, norm);

    if (cosI > 0.0f) {
        // Leaving the shape
        n1 = rec.m->refrIndex;
        n2 = 1.0f;
        // Hitting from inside of the surface, so
        // make the normal face inside the shape
        norm = -norm;
    } else {
        // Entering the shape, assume outside is air
        n1 = 1.0f;
        n2 = rec.m->refrIndex;
        cosI = -cosI;
    }

    float etaRatio = n1/n2;
    
    float sin2T = etaRatio*etaRatio*(1-cosI*cosI);

    // Total internal reflection, but reflect instead
    if (sin2T > 1.0f) return reflectRay(ray, rec);
    // TODO: fresnal effect
    Ray refrRay;  
    refrRay.dir = glm::refract(ray.dir, norm, etaRatio);
    refrRay.pos = rec.x + refrRay.dir*(float)Camera::EPSILION;

    return refrRay;
}

vec3 Camera::getRayColor(
    shared_ptr<Scene> scene, 
    const Ray& ray, 
    const Interval& interval, 
    uint recursiveDepth) 
{
    Hit rec;
    vec3 clr = vec3(0.0);
    if (!hit(scene->shapes, ray, interval, rec)) return getSkyColor(ray);

    if (SHOW_NORMALS) {
        clr = rec.n;
        clr.r = .5f*clr.r+.5f;
        clr.g = .5f*clr.g+.5f;
        clr.b = .5f*clr.b+.5f;
        return clr;
    }

    if (rec.m->reflCoeff >= Camera::MINIMUM_REFL_COEFF) { 
        if (recursiveDepth >= Camera::MAX_RECURSIONS) 
            return clr;
        vec3 reflClr = getRayColor(scene, reflectRay(ray, rec), interval, recursiveDepth+1);
        clr += rec.m->reflCoeff*reflClr;
        
    } else if (glm::abs(rec.m->refrIndex-1.0f) > CONSTANTS::EPSILION) {
        if (recursiveDepth >= Camera::MAX_RECURSIONS)
            return clr;
        vec3 refrClr = getRayColor(scene, refractRay(ray, rec), interval, recursiveDepth+1);
        clr += refrClr;
    }

    // The eye vector does not point to the camera when reflecting
    vec3 ev = recursiveDepth > 0 ? normalize(-ray.dir) : normalize(vec3(cameraPos) - rec.x);

    // Determine if the ray is inside or outside the object
    bool back_face = dot(ray.dir, rec.n) > 0.0f; // true if inside
    if (back_face) rec.n = -rec.n;

    vec3 bp_clr = rec.m->ambient;
    for (auto& light : scene->lights) {
        // Construct a shadow ray for each light, using
        // world coordinates.
        vec3 ld = light->pos - rec.x;
        vec3 lv = normalize(ld);
        float tl = length(ld);

        Ray sray;
        sray.pos = rec.x + (float)interval.min*rec.n;
        sray.dir = lv;

        Hit srec;
        if (hit(scene->shapes, sray, Interval(interval.min, tl), srec)) {
            continue;
        }

        float Li = light->intensity;
        vec3 kd = rec.m->diffuse, ks = rec.m->specular;
        float s = rec.m->exponent;
        vec3 h = normalize(lv + ev);

        auto diff_cont = kd*std::max(0.0f, glm::dot(rec.n, lv));
        auto spec_cont = ks*std::pow(std::max(0.0f, glm::dot(rec.n, h)), s);
        bp_clr += Li * (diff_cont + spec_cont);
    }

    clr += (1.0f - rec.m->reflCoeff) * bp_clr;
    return clr;
}