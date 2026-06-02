#include "Camera.hpp"
#include "prand.hpp"

// #define SHOW_NORMALS

using namespace std;
using namespace glm;

typedef const vector<shared_ptr<Shape>>& ShapesVector;

// Arbitrary size, but computing these numbers
// beforehand saves actual seconds
// initializing the vectors on the heap instead of
// stack is better when low or no antialiasing done 
// (1.3 sec w/o unique and 1.0 sec w/ unique)
std::unique_ptr<prand::uniformRand> unifRandGen = 
    std::make_unique<prand::uniformRand>(RAND_GEN_SIZE);
std::unique_ptr<prand::diskRand> diskRandGen = 
    std::make_unique<prand::diskRand>(RAND_GEN_SIZE);



void Camera::applyProjection(MatrixStack& MS) {
    MS.mult(perspective(fovy, aspectRatio, znear, zfar));
}
void Camera::applyView(MatrixStack& MS) {
    glm::mat4 lookAtMat;
    glm::vec3 center, eye, upVec;

    MS.push();
    MS.translate(translation);
    // yaw, pitch, then roll
    MS.rotate(rotation.z, vec3(0.0f, 0.0f, 1.0f));
    MS.rotate(rotation.y, vec3(1.0f, 0.0f, 0.0f));
    MS.rotate(rotation.x, vec3(0.0f, 1.0f, 0.0f));

    // inverse of view matrix so that the 
    // center, eye, and up vectors are 
    // specified wrt old transforms
    auto cameraMat = glm::inverse(MS.top());
    center = this->lookAtPos;
    eye = cameraMat * vec4(position, 1.f);
    upVec = cameraMat * vec4(camUpVec, 0.f);

    lookAtMat = glm::lookAt(eye, center, upVec);
    MS.pop();

    MS.mult(lookAtMat);
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
    vec2 rndVec = diskRandGen->rand();
    cray.time = std::fmod(dot(rndVec, rndVec), 1.f);

    return cray;
}

void Camera::setRow(const unique_ptr<Scene>& scene, unique_ptr<Image>& image, uint y) 
{
    for (uint x = 0; x < width; ++x) {
        vec3 color = vec3(0.0f);

        Ray cray = castPrimaryRay(x, y);
        color = getRayColor(scene, cray);
        
        // breakpoints have experimentally OK magic numbers
        uint breakpoint = std::max(AAsamples / 4, 8U);
        float r_samplesDone = sample_scale;
        for (uint i = 1; i < AAsamples; ++i) {
            // Branching isn't great if it doesn't lead to early breaks
            vec2 offset = 0.5f*diskRandGen->rand(i) + vec2(0.5f);
            
            float dx = offset.x, dy = offset.y;
            Ray cray = castPrimaryRay(x, y, dx, dy);
            vec3 rayColor = getRayColor(scene, cray);
            color += rayColor;

            if (i < breakpoint) continue; 
            
            // Stop sampling this pixel if the contribution
            // of the new sample is < 2 values for all comps
            auto eqCmp = glm::epsilonEqual(color, 
                static_cast<float>(i+1)*rayColor, 2.f/255.f);
            if (eqCmp.r && eqCmp.g && eqCmp.b) {
                r_samplesDone = 1.f / static_cast<float>(i + 1);
                break;
            }
        }

        color *= r_samplesDone;
        
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

bool hit(ShapesVector shapes, const Ray& ray, const Interval& interval, Hit& closestHit) 
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
// bool hit(ShapesVector shapes, const Ray& ray, const Interval& interval) 
// {
//     bool intersected_any = false;
//     vector<Hit> temp_hits;
//     temp_hits.reserve(16); // magic number
//     for (const shared_ptr<Shape>& shape : shapes) {
//         shape->intersect(ray, temp_hits);
//         if (temp_hits.empty()) continue;
//         for (Hit& hit : temp_hits) {
//             if (!interval.contains(hit.t)) continue;
//             intersected_any = true;
//             break;
//         }
//     }
//     return intersected_any;
// }

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
    Ray reflRay = ray; 
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
    Ray refrRay = ray;

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
        Ray nearRay = ray;
        nearRay.setDir(normalize(ray.getDir() + glm::sphericalRand(hit.m->fuzz)));
        reflClr += getRayColor(scene, reflectRay(nearRay, hit), interval, recursions);
    }
    // reflSamples must be at least 1.
    reflClr /= hit.m->reflSamples;
    return reflClr;
}

// Whitted-style ray-tracing, but sometimes amalmagated
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

    vec3 bp_clr = rec.ambient() + globalAmbient;
    if (occlusionSamples > 0 && recursiveDepth < 2) {
        // the maximum is arbitrary, but it should be small 
        // so that faraway objects are not considered
        vec3 occlFac = occlusionFactor(rec, scene, Interval(interval.min, occludingRadius));
        // sqrt is a hack that makes the shadows softer
        bp_clr.r *= std::sqrt(occlFac.r);
        bp_clr.g *= std::sqrt(occlFac.g);
        bp_clr.b *= std::sqrt(occlFac.b); 
    }

    for (auto& light : scene->getLights()) {
        vec3 ld = light->pos - rec.x;
        vec3 lv = normalize(ld);
        // Construct shadow rays for each light, using world coordinates.
        // Boolean parameter to cull the number of neglible rays casted for shadows 
        float lightVisibility = shadowFactor(light, rec, scene, interval, recursiveDepth < 2);

        float Li = light->intensity;
        bp_clr += lightVisibility * Li * lightingFactor(rec, lv, ev);
    }

    // not really ideal
    clr += (1.0f - rec.m->reflCoeff)*(1.0f - rec.m->transparency)*bp_clr;

    clr += reflectClr*reflectance + refractClr*(1.0f-reflectance);

    clr += rec.emissive();

    return clr;
}

vec3 Camera::lightingFactor(const Hit &rec, const vec3 &lv, const vec3 &ev)
{
    vec3 kd = rec.diffuse(), 
         ks = rec.specular();
    float s = rec.m->exponent;

    vec3 h = normalize(lv + ev);
    auto diff_cont = kd*std::max(0.0f, glm::dot(rec.n, lv));
    auto spec_cont = ks*std::pow(std::max(0.0f, glm::dot(rec.n, h)), s);
    return (diff_cont + spec_cont);
}

// Uses monte carlo integration
vec3 Camera::occlusionFactor(const Hit &rec, 
                             const unique_ptr<Scene> &scene,
                             const Interval &interval) 
{
    vec3 lightAbsorption(0.f);

    vec3 T, B;
    assignONBvec3s(rec.n, T, B);
    vec3 aorayPos = rec.x + (float)interval.min*rec.n;

    // Reuse the ray object, yes?
    Ray aoray;
    aoray.setPos(aorayPos);

    for (uint i = 0; i < occlusionSamples; ++i) {
        float u1 = unifRandGen->rand();
        float u2 = unifRandGen->rand();
        vec3 rDir = cosineSampleHemisphere(u1, u2);
        // Transform the sampled vector from tangent to world space
        rDir = vec3(rDir.x*T + rDir.y*B + rDir.z*rec.n);        
        aoray.setDir(normalize(rDir));

        Hit aoHit;
        bool occluded = hit(scene->getShapes(), aoray, interval, aoHit);
        if (occluded && aoHit.m) {
            float atten = glm::clamp(aoHit.t / (float)interval.max, 0.f, 1.f);
            lightAbsorption += vec3(1.f) - aoHit.diffuse() * atten;
        }
    }
    vec3 occlusionCoeff = vec3(1.f) - (lightAbsorption / (float)occlusionSamples);
    
    return occlusionCoeff;
}

// Used for sampling points on spherical area lights
// at a somewhat reasonable efficiency
class sampleCone {
private:
    glm::vec3 dx, dy;
public:
    // Akalin's method, ty scratchapixel for saving me from this area light torment nexus
    sampleCone(int N) { }
    inline glm::vec3 operator()(const glm::vec3 &ld, const float radius) {
        // Faster to generate less random variables
        float r1 = unifRandGen->rand();
        float r2 = unifRandGen->rand();

        glm::vec3 dz = ld;
        float dz_len_2 = glm::dot(dz, dz);
        float dz_len = std::sqrt(dz_len_2);
        dz /= -dz_len;

        assignONBvec3s(dz, dx, dy);

        float sin_theta_max_2 = radius * radius / dz_len_2;
        float sin_theta_max = std::sqrt(sin_theta_max_2);
        float cos_theta_max = std::sqrt(std::max(0.f, 1.f - sin_theta_max_2));

        float cos_theta = 1.f + (cos_theta_max - 1.f) * r1;
        float sin_theta_2 = 1.f - cos_theta * cos_theta;

        float cos_alpha = (sin_theta_2 / sin_theta_max) + 
            cos_theta * std::sqrt(1.f - sin_theta_2 / sin_theta_max_2);
        float sin_alpha = std::sqrt(1.f - cos_alpha * cos_alpha);
        float phi = 2 * PI * r2;

        // PDFs are useful for path tracing, but not for this Whitted-hybrid tracer 
        // float pdf 1.f / (2.f * PI * (1.f - cos_theta_max));

        return std::cos(phi)*sin_alpha*dx + std::sin(phi)*sin_alpha*dy + cos_alpha*dz;
    }
};

// Randomly samples points on area lights depending on their radius.
float Camera::shadowFactor(const shared_ptr<Light>& light, 
                           const Hit &rec, 
                           const unique_ptr<Scene> &scene,
                           const Interval &interval,
                           bool sampleArea = true) 
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
        bool isEmiss = srec.m && dot(srec.emissive(), srec.emissive()) > 0.0f;

        // 1.0f is fully lit by default, which is when point has unobstructed path to light
        float s_transparency = !behindShape; // if behind, return value from 0.0f to 1.0f
        if (isEmiss) s_transparency = 1.0f;
        else if (isTrns) s_transparency = glm::clamp(srec.m->transparency, 0.0f, 1.0f);

        return s_transparency;
    };

    float visibleLight = getShadowContrib();

    if (light->getRadius() < MINIMUM_COEFF || !sampleArea) { return visibleLight; }

    
    auto sampler = sampleCone(light->getSamples());
    
    for (int i = 1; i < light->getSamples(); ++i) {
        vec3 offset = sampler(ld, light->getRadius());
        vec3 new_ld = light->pos + offset*light->getRadius() - rec.x;
        vec3 new_lv = normalize(new_ld);
        tl = length(new_ld);
        sray.setDir(new_lv);
        
        visibleLight += getShadowContrib();
    }

    return visibleLight / (float)light->getSamples();
}