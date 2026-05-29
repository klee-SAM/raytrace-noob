#include "Camera.hpp"

// #define SHOW_NORMALS

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

Ray Camera::castPrimaryRay(uint idx, uint idy, float offsetx, float offsety) {
    // https://www.realtimerendering.com/blog/the-center-of-the-pixel-is-0-50-5/

    float ndc_y = 2*((float)idy + offsety)/((float)height) - 1.0;
    float ndc_x = 2*((float)idx + offsetx)/((float)width) - 1.0;

    glm::vec4 coord(ndc_x, ndc_y, -1.0f, 1.0f); // px coord
    coord = invP*coord; // eye coord
    coord.w = 1.0f;
    coord = glm::normalize(C*coord - cameraPos); // n_pw

    Ray cray; 
    cray.pos = cameraPos;
    cray.dir = coord;

    return cray;
}

void Camera::setRow(const unique_ptr<Scene>& scene, unique_ptr<Image>& image, uint y) 
{
    for (uint x = 0; x < width; ++x) {
        vec3 color = vec3(0.0f);

        Ray cray = castPrimaryRay(x, y);
        color = getRayColor(scene, cray);
        
        for (uint i = 1; i < AAsamples; ++i) {
            vec2 offset;
            if (AAsamples > prand::N) {
                offset.x = linearRand(0.001f, 0.999f);
                offset.y = linearRand(0.001f, 0.999f);
            } else offset = 0.5f*prand::poissonDisk(i);
            
            float dx = offset.x + 0.5f;
            float dy = offset.y + 0.5f;
            Ray cray = castPrimaryRay(x, y, dx, dy);
            color += getRayColor(scene, cray);
        }

        color *= sample_scale;
        
        image->setPixel(x, y, color);
    }
}

void Camera::processRows(const unique_ptr<Scene>& scene, unique_ptr<Image>& image) 
{
    while (!r_queue.empty()) {
        RowQueue::extract_pair pair = r_queue.pop();
        if (!pair.success) return;
        setRow(scene, image, pair.row);
        r_queue.rowsProcessed++;
    }
}

unique_ptr<Image> Camera::render(unique_ptr<Scene>& scene, const mat4& P, const mat4& V) 
{
    // Precompute as much as possible before loops
    C = inverse(V);
    invP = glm::inverse(P);
    cameraPos = C[3]; 
    cameraPos.w = 1.0f;

    assert(AAsamples > 0);
    sample_scale = 1.0/AAsamples;

    // Divide by the AAsamples, because having occlusionSamples * AAsamples rays
    // per pixel is too much for a simple toy raytracer
    uint actualOcclusionSamples = std::max(1U, (occlusionSamples / AAsamples));
    bool useReducedOcclSamp = occlusionSamples > 0 && divideAObyAA;
    occlusionSamples = useReducedOcclSamp ? actualOcclusionSamples : 0;

    uint totalCasts = height*width;

    unique_ptr<Image> image = make_unique<Image>(width, height);

    uint numThreads = thread::hardware_concurrency();
    // create at least 2 threads: one for counting, the other traces
    if (numThreads < 2) numThreads = 2;
    std::clog << "Threads available: " << numThreads << '\n';

    uint jobsFinished = 0;

    // Push rows as jobs
    for (uint y = 0; y < height; ++y) { r_queue.push(y); }

    vector<thread> threads;
    // reserve 1 thread for outputting the current number of scans completed
    for (uint i = 0; i < numThreads-1; ++i) {
        // passing by non-const ref dangerous!!! thank goodness only 1
        // thread processes 1 row at a time
        threads.push_back(std::move(thread(&Camera::processRows, this, 
            std::ref(scene), std::ref(image))));
    }

    // horrific
    auto countScans = [this, numThreads, totalCasts, jobsFinished]() {
        while (r_queue.rowsProcessed < height && jobsFinished < numThreads-1) {
            std::clog << '\r' << r_queue.rowsProcessed*width << '/' 
                    << totalCasts << " scans completed " << std::flush;
            this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    };
    
    thread counter(countScans);

    for (auto& thread : threads) {
        if (!thread.joinable()) continue;
        thread.join();
        ++jobsFinished;
    }

    if (counter.joinable()) counter.join();

    std::clog << "\rDone." << std::string(30, ' ') << '\n';
    return image;
}

bool hit(const vector<shared_ptr<Shape>>& shapes, 
         const Ray& ray, 
         const Interval& interval,
         Hit& closestHit) 
{
    float minDist = interval.max;
    bool intersected_any = false;

    vector<Hit> temp_hits; // maintain a list of hits for csg
    temp_hits.reserve(16); // magic number

    for (const shared_ptr<Shape>& shape : shapes) {
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

// Like the above, except this is used
// for cases where the hit information is unused
bool hit(const vector<shared_ptr<Shape>>& shapes, 
         const Ray& ray, 
         const Interval& interval) 
{
    bool intersected_any = false;
    vector<Hit> temp_hits;
    temp_hits.reserve(16); // magic number

    for (const shared_ptr<Shape>& shape : shapes) {
        shape->intersect(ray, temp_hits);
        if (temp_hits.empty()) continue;
        for (Hit& hit : temp_hits) {
            if (!interval.contains(hit.t)) continue;
            intersected_any = true;
            break;
        }
    }
    return intersected_any;
}

vec3 Camera::getSkyColor(const Ray& ray) 
{
    float cx, cy, cz;
    switch(this->sky) {
    case (Camera::SkyType::Haze):
        cx = .5*(ray.dir.x)+.5;
        cy = .5*(ray.dir.y)+.5;
        cz = .5*(ray.dir.z)+.5;
        return vec3(cx, cz, cy);
    case (Camera::SkyType::Void):
    default:
        return vec3(0.0f);
        break;
    }
    return vec3(0.0f);
}

Ray reflectRay(const Ray &ray, const Hit &rec) 
{
    Ray reflRay; 
    // ray.dir - 2.0f * dot(rec.n, ray.dir) * rec.n
    reflRay.setDir(glm::reflect(ray.getDir(), rec.n));
    reflRay.setPos(rec.x + reflRay.getDir()*Camera::EPSILION);
    return reflRay;
}

// Creates a new ray with a direction dependant on the material's IoR
// Uses Schlick's approximation of Fresnel's equations for refraction.
Ray refractRay(const Ray &ray, const Hit &rec, float &reflectance, bool backFacing) 
{
    float n1, n2;
    const float &rf_i = rec.m->refrIndex;
    vec3 norm = rec.n;
    Ray refrRay;

    float cosI = -dot(normalize(ray.getDir()), norm);
    assert(fabs(cosI) < 1.01f);

    if (backFacing) {
        // Leaving the shape
        n1 = rf_i;
        n2 = 1.0f;
    } else {
        // Entering the shape, assume outside is air
        n1 = 1.0f;
        n2 = rf_i;
    }

    float eta = n1/n2;
    float sin2T = eta*eta*(1.0f-cosI*cosI);

    // Total internal reflection, but reflect instead
    if (sin2T > 1.0f) {
        reflectance = 1.0f;
        return refrRay;
    }
    
    // cos(t)^2
    float cosT = sqrt(1.0f - sin2T);
    float m = 1.0f - (n1 > n2 ? cosT : cosI);
    // atrocity for (1 - cos)^5: (m^2)^2 * m = m^5
    float r0 = (n1 - n2)/(n1 + n2); r0 *= r0;
    reflectance = r0 + (1.0f-r0)*(m*m*m*m*m);
    // reflectance = std::clamp(fabs(reflectance), 0.0f, 1.0f);

    refrRay.setDir( normalize(eta*ray.getDir()) + ((eta*(cosI) - cosT)*norm) );
    refrRay.setPos( rec.x + refrRay.getDir()*(float)Camera::EPSILION );

    return refrRay;
}

// this does not increment `recursions` when it recursively calls
// check to make sure that the recursion count is incremented when calling this
vec3 Camera::getReflectionColor(const std::unique_ptr<Scene> &scene,
                                const Ray &ray, const Hit &hit,
                                const Interval &interval, 
                                uint recursions) 
{
    vec3 reflClr = getRayColor(scene, reflectRay(ray, hit), interval, recursions);
    for (uint r = 1; r < hit.m->reflSamples; ++r) {
        Ray nearRay; nearRay.pos = ray.pos;
        nearRay.setDir(normalize(ray.getDir() + glm::sphericalRand(hit.m->fuzz)));
        reflClr += getRayColor(scene, reflectRay(nearRay, hit), interval, recursions);
    }
    // reflSamples must be at least 1.
    reflClr /= hit.m->reflSamples;
    return reflClr;
}

vec3 Camera::getRayColor(const unique_ptr<Scene>& scene, 
                         const Ray& ray, 
                         const Interval& interval, 
                         uint recursiveDepth) 
{
    Hit rec;
    vec3 clr = vec3(0.0f);
    if (!hit(scene->getShapes(), ray, interval, rec)) return getSkyColor(ray);

    #ifdef SHOW_NORMALS
    clr = rec.n;
    clr.r = .5f*clr.r+.5f;
    clr.g = .5f*clr.g+.5f;
    clr.b = .5f*clr.b+.5f;
    return clr;
    #endif

    vec3 reflectClr = vec3(0.0f);
    vec3 refractClr = vec3(0.0f);

    float reflectance = 1.0f; // If the material is not refractive, keep any reflections
    bool reflective = rec.m->reflCoeff > Camera::MINIMUM_COEFF;
    bool refractive = rec.m->transparency > Camera::MINIMUM_COEFF;

    // Determine if the ray is inside or outside the object,
    // only to handle the case of lighting for CSG.
    // Doing this means that refraction must account for
    // this possibility via parameter
    bool back_face = dot(ray.getDir(), rec.n) > 0.0f; // true if inside

    // Avoid casting additional reflection rays if inside the object, this
    // avoids massively expensive and unnecessary computation (3x increase)
    if (reflective && !back_face) { 
        if (recursiveDepth >= Camera::MAX_RECURSIONS) return clr;
        reflectClr = getReflectionColor(scene, ray, rec, interval, recursiveDepth+1);
        reflectClr *= rec.m->reflCoeff;
    }   

    if (refractive) {
        if (recursiveDepth >= Camera::MAX_RECURSIONS) return clr;
        // flip the normal for refraction, if inside
        // this must be done before calculating cosI for reflection, or
        // else strange glint appear on refractive surfaces, even if
        // the cosI and norm are negated after checks
        if (back_face) rec.n = -rec.n;  
        // If the above line is not nested in this if statement,
        // bright specks may appear on meshes w/ backface culling enabled. 
        refractClr = getRayColor(scene, refractRay(ray, rec, reflectance, back_face), 
                              interval, recursiveDepth+1);
        refractClr *= rec.m->transparency;
    }

    // The eye vector does not point to the camera when reflecting/refracting
    vec3 ev = -ray.dir;

    vec3 bp_clr = rec.ambient();
    if (occlusionSamples > 0) {
        // the maximum is arbitrary, but it should be small 
        // so that faraway objects are not considered
        bp_clr *= occlusionFactor(rec, scene, Interval(interval.min, 0.5f));
    }

    for (auto& light : scene->getLights()) {
        // Construct a shadow ray for each light, using
        // world coordinates.
        vec3 ld = light->pos - rec.x;
        vec3 lv = normalize(ld);
        // float tl = length(ld);

        // Ray sray;
        // sray.setPos(rec.x + (float)interval.min*rec.n);
        // sray.setDir(lv);

        // Hit srec;
        // bool behindShape = hit(scene->getShapes(), sray, Interval(interval.min, tl), srec);
        // bool shapeIsTransparent = srec.m != nullptr &&
        //                           srec.m->transparency > Camera::MINIMUM_COEFF;

        float lightVisibility = shadowFactor(light, rec, scene, interval);

        // if (behindShape) {
        //     if (!shapeIsTransparent) continue;
        //     s_transparency = srec.m->transparency;
        // }

        float Li = light->intensity;
        vec3 kd = rec.diffuse(), 
             ks = rec.specular();
        float s = rec.m->exponent;
        vec3 h = normalize(lv + ev);

        auto diff_cont = kd*std::max(0.0f, glm::dot(rec.n, lv));
        auto spec_cont = ks*std::pow(std::max(0.0f, glm::dot(rec.n, h)), s);
        bp_clr += lightVisibility * Li * (diff_cont + spec_cont);
    }

    // not really ideal
    clr += (1.0f - rec.m->reflCoeff)*(1.0f - rec.m->transparency)*bp_clr;

    clr += reflectClr*reflectance + refractClr*(1.0f-reflectance);

    return clr;
}


// orthonormal basis (TBN matrix)
void assignONBvec3s(const vec3& n, vec3& b1, vec3& b2) 
{   // thank you Duff et al
    const float sign = copysignf(1.0f, n.z); // sign should be 1 even when n.z == 0
    const float a = -1.0f / (sign + n.z);
    const float b = n.x * n.y * a;
    b1 = vec3(1.0f + sign * n.x * n.x * a, sign * b, -sign * n.x);
    b2 = vec3(b, sign + n.y * n.y * a, -n.y);
}

// This implements importance sampling.
// u1 and u2 are random uniform variables
vec3 cosineSampleHemisphere(float u1, float u2) 
{   // thank you rory
    const float r = sqrt(u1);
    const float theta = 2 * PI * u2;
    const float x = r*cos(theta);
    const float y = r*sin(theta);
    // Modification: assume u1 can never go above 1.0f
    return vec3(x, y, sqrt(1.0f - u1));
}

// Uses monte carlo integration
float Camera::occlusionFactor(const Hit &rec, 
                              const unique_ptr<Scene> &scene,
                              const Interval &interval) 
{
    float num_occluded = 0.0f;

    vec3 T, B;
    assignONBvec3s(rec.n, T, B);
    vec3 aorayPos = rec.x + (float)interval.min*rec.n;

    // Reuse the ray object, yes?
    Ray aoray;
    aoray.setPos(aorayPos);

    for (uint i = 0; i < occlusionSamples; ++i) {
        float u1 = glm::linearRand(0.0f, 0.999999f);
        float u2 = glm::linearRand(0.0f, 0.999999f);
        vec3 rDir = cosineSampleHemisphere(u1, u2);
        // Transform the sampled vector from tangent to world space
        rDir = vec3(rDir.x*T + rDir.y*B + rDir.z*rec.n);        
        aoray.setDir(normalize(rDir));

        bool occluded = hit(scene->getShapes(), aoray, interval);
        if (occluded) ++num_occluded;
    }
    float occlusionCoeff = 1.0f - (num_occluded / (float)occlusionSamples);
    
    return occlusionCoeff;
}

float Camera::shadowFactor(const shared_ptr<Light>& light, 
                           const Hit &rec, 
                           const unique_ptr<Scene> &scene,
                           const Interval &interval) 
{
    vec3 ld = light->pos - rec.x;
    vec3 lv = normalize(ld);
    float tl = length(ld);

    Ray sray;
    sray.setPos(rec.x + (float)interval.min*rec.n);
    sray.setDir(lv);

    Hit srec;

    auto getShadowContrib = [&]() {   
        bool behindShape = hit(scene->getShapes(), sray, Interval(interval.min, tl), srec);
        bool isTrns = srec.m && srec.m->transparency > Camera::MINIMUM_COEFF;

        // 1.0f is fully lit by default, which is when point has unobstructed path to light
        float s_transparency = !behindShape; // if behind, return value from 0.0f to 1.0f
        if (isTrns) s_transparency = glm::clamp(srec.m->transparency, 0.0f, 1.0f);
        return s_transparency;
    };

    float occlusion = getShadowContrib();

    // lots of branching, but i also don't have time for a better implementation
    // because atp it would be premature optimization
    if (light->radius < MINIMUM_COEFF) { return occlusion; }

    vec3 T, B;
    assignONBvec3s(lv, T, B);

    // arbitrary dynamic formula for area light sampling; max samples 
    // are done when light has a radius of 0.25 or more
    const int numSamples = 18 * glm::clamp(2*sqrt(light->radius), 0.05f, 1.0f); 
    for (int i = 1; i < numSamples; ++i) {
        vec2 rnd = light->radius*prand::poissonDisk(i);
        // is this mathematically correct for orienting a disk 
        // perpendicular to the light vector?
        vec3 offset = vec3(rnd.x*T + rnd.y*B);
        sray.setDir(normalize(lv+offset));
        occlusion += getShadowContrib();
    }

    return occlusion / (float)numSamples;
}