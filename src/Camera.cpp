#include "Camera.hpp"

using namespace std;
using namespace glm;

void Camera::applyProjection(MatrixStack& MS) {
    MS.mult(perspective(fovy, aspectRatio, znear, zfar));
}
void Camera::applyView(MatrixStack& MS) {
    MS.translate(translation);
    MS.rotate(rotation.y, vec3(1.0f, 0.0f, 0.0f));
    MS.rotate(rotation.x, vec3(0.0f, 1.0f, 0.0f));
}

Ray Camera::castPrimaryRay(uint idx, uint idy, double offset) {
    // https://www.realtimerendering.com/blog/the-center-of-the-pixel-is-0-50-5/

    double ndc_y = 2*((double)idy + offset)/((double)height) - 1.0;
    double ndc_x = 2*((double)idx + offset)/((double)width) - 1.0;

    glm::vec4 coord((float)ndc_x, (float)ndc_y, -1.0f, 1.0f); // px coord
    coord = invP*coord; // eye coord
    coord.w = 1.0f;
    coord = glm::normalize(C*coord - cameraPos); // n_pw

    Ray cray; 
    cray.pos = vec3(cameraPos);
    cray.dir = vec3(coord);

    return cray;
}

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

    shared_ptr<Image> image = make_shared<Image>(width, height);

    for (uint y = 0; y < height; ++y) {
        for (uint x = 0; x < width; ++x) {
            vec3 color = vec3(0.0f);

            if (samples <= 1) {
                Ray cray = castPrimaryRay(x, y);
                color = getRayColor(scene, cray);
            } else {
                for (uint i = 0; i < samples; ++i) {
                    Ray cray = castPrimaryRay(x, y, linearRand(.01f, 0.99f));
                    color += getRayColor(scene, cray);
                }
                color *= sample_scale;
            }
            
            image->setPixel(x, y, color);
        }
    }
    return image;
}

bool hit(
    vector<shared_ptr<Shape>>& shapes, 
    const Ray& ray, 
    float min, 
    float max, 
    Hit& closestHit) 
{
    float minDist = max;
    bool intersected = false;

    vector<Hit> hits; // maintain a list of hits for csg
    hits.reserve(16); // magic number

    for (shared_ptr<Shape>& shape : shapes) {
        shape->intersect(ray, hits);
        if (hits.empty()) continue;
        for (Hit& hit : hits) {
            if (hit.t >= min && hit.t <= max) {
                intersected = true;
            } else {
                continue;
            }
            if (minDist > hit.t) {
                minDist = hit.t;
                closestHit = hit;
            }
        }
        hits.clear();
    }
    return intersected;
}

const bool SHOW_NORMALS = false;


vec3 Camera::getRayColor(
    shared_ptr<Scene> scene, 
    const Ray& ray, 
    float min, float max, 
    uint recursiveDepth) 
{
    Hit rec;
    vec3 clr = vec3(0.0);
    if (!hit(scene->shapes, ray, min, max, rec)) return clr;

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

        vec3 refl = glm::reflect(ray.dir, rec.n);
        Ray reflRay; reflRay.dir = refl;
        reflRay.pos = rec.x + rec.n*min;
        clr += rec.m->reflCoeff*getRayColor(scene, reflRay, min, max, recursiveDepth + 1);
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
        sray.pos = rec.x + min*rec.n;
        sray.dir = lv;

        Hit srec;
        if (hit(scene->shapes, sray, min, tl, srec)) {
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