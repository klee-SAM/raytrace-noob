#include "Camera.hpp"
#include "util/prand.hpp"

// #define SHOW_NORMALS

using std::vector;
using std::shared_ptr;
using std::unique_ptr;

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

constexpr float SAMP_DIFF_EPSILION = 2.f/255.f;

// Returns the closest intersection in the interval.
bool hit(ShapesVector shapes, const Ray& ray, const Interval& interval, Hit& closestHit);
// True if currClr*(i+1) equals culmClr within sampBreak threshold; should be called
// after currClr is added to culmClr.
bool has_no_change(uint i, uint break_i, const vec3 &culmClr, 
    const vec3 &currClr, float sampBreak = SAMP_DIFF_EPSILION);

void Camera::applyProjection(MatrixStack& MS) {
    MS.mult(glm::perspective(fovy, aspectRatio, znear, zfar));
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
    const auto cameraMat = glm::inverse(MS.top());
    center = this->lookAtPos;
    eye = cameraMat * vec4(position, 1.f);
    upVec = cameraMat * vec4(camUpVec, 0.f);

    lookAtMat = glm::lookAt(eye, center, upVec);
    MS.pop();

    MS.mult(lookAtMat);
}

Ray Camera::castPrimaryRay(uint idx, uint idy, float offsetx, float offsety) {
    // https://www.realtimerendering.com/blog/the-center-of-the-pixel-is-0-50-5/

    const float ndc_y = 2*((float)idy + offsety)/((float)height) - 1.0;
    const float ndc_x = 2*((float)idx + offsetx)/((float)width) - 1.0;

    glm::vec4 coord(ndc_x, ndc_y, -1.0f, 1.0f); // px coord
    coord = invP*coord; // eye coord
    coord.w = 1.0f;
    coord = glm::normalize(C*coord - cameraPos); // n_pw

    Ray cray; 
    cray.pos = cameraPos;
    cray.dir = coord;
    const vec2 rndVec = diskRandGen->rand();
    cray.time = std::fmod(dot(rndVec, rndVec), 1.f);

    return cray;
}

bool has_no_change(const uint i, const uint min_i, const vec3 &culmClr, 
    const vec3 &currClr, const float sampBreak)
{
    if (i < min_i) return false;
    // Stop sampling this pixel if the contribution
    // of the new sample is < sampBreak for all comps
    const vec3 extCurrClr = static_cast<float>(i+1)*currClr;
    const auto eqCmp = glm::epsilonEqual(culmClr, extCurrClr, sampBreak);
    return eqCmp.r && eqCmp.g && eqCmp.b;
}

void Camera::setRow(const unique_ptr<Scene>& scene, unique_ptr<Image>& image, uint y) 
{
    for (uint x = 0; x < width; ++x) {
        vec3 color = vec3(0.0f);

        Ray cray = castPrimaryRay(x, y);
        color = getRayColor(scene, cray);
        
        // breakpoints have experimentally OK magic numbers
        const uint breakpoint = std::max(AAsamples / 4, 8U);
        float r_samplesDone = sample_scale;
        for (uint i = 1; i < AAsamples; ++i) {
            // Branching isn't great if it doesn't lead to early breaks
            const vec2 offset = 0.5f*diskRandGen->rand(i) + vec2(0.5f);
            
            const float dx = offset.x, dy = offset.y;
            cray = castPrimaryRay(x, y, dx, dy);
            const vec3 rayColor = getRayColor(scene, cray);
            color += rayColor;
            
            // Stop sampling this pixel if the contribution
            // of the new sample is < epsilion values for all comps
            bool cond = has_no_change(i, breakpoint, color, rayColor);
            if (cond) { r_samplesDone = 1.f / static_cast<float>(i + 1); break; }
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

    unique_ptr<Image> image = std::make_unique<Image>(width, height);

    uint numThreads = std::thread::hardware_concurrency();
    // create at least 2 threads: one for counting, the other traces
    if (numThreads < 2) numThreads = 2;
    std::clog << "Threads available: " << numThreads << '\n';

    uint jobsFinished = 0;

    // Push rows as jobs
    for (uint y = 0; y < height; ++y) { r_queue.push(y); }

    vector<std::thread> threads;

    for (uint i = 0; i < numThreads; ++i) {
        // passing by non-const ref dangerous!!! thank goodness only 1
        // thread processes 1 row at a time
        threads.push_back(std::move(std::thread(&Camera::processRows, this, 
            std::ref(scene), std::ref(image))));
    }

    // horrific; +1 thread than cores works b/c it's i/o bound (sleep)
    auto countScans = [this, numThreads, totalCasts, jobsFinished]() {
        while (r_queue.rowsProcessed < height && jobsFinished < numThreads) {
            std::clog << '\r' << r_queue.rowsProcessed*width << '/' 
                    << totalCasts << " scans completed " << std::flush;
            // Results in displayed times being larger than actual times
            // for very simple and fast scenes, but less thread switching 
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    };
    
    std::thread counter(countScans);

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
// for cases where all hits for a ray are needed
bool hit(ShapesVector shapes, const Ray& ray, 
         const Interval& interval, vector<Hit>& allHits) 
{
    bool intersected_any = false;
    vector<Hit> temp_hits;
    temp_hits.reserve(16);
    for (const shared_ptr<Shape>& shape : shapes) {
        shape->intersect(ray, temp_hits);
        for (Hit& hit : temp_hits) {
            if (!interval.contains(hit.t)) continue;
            intersected_any = true;
            allHits.push_back(hit);
        }
        temp_hits.clear();
    }
    Hit::sortHits(allHits);
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
    reflRay.time = ray.time;
    return reflRay;
}

// Creates a new ray with a direction dependent on the material's IoR
// Returns a ray with dir = vec3(0.f) if TIR
Ray refractRay(const Ray &ray, const Hit &rec, float eta) {
    const vec3 &norm = rec.n;
    float cosI = -dot(normalize(ray.getDir()), norm);
    cosI = std::clamp(cosI, -1.f, 1.f);
    const float sinT2 = eta*eta*(1.0f-cosI*cosI);
    if (sinT2 > 1.f) return Ray(vec3(0.f), vec3(0.f), 0.f);
    const float cosT = std::sqrt(1.f - sinT2);
    const vec3 rayDir = eta*ray.getDir()+norm*(eta*cosI-cosT);
    const vec3 rayPos = rec.x + rayDir*Camera::EPSILION;
    return Ray(rayPos, normalize(rayDir), ray.time);
}

// Uses Schlick's approximation of Fresnel's equations for refraction.
// https://computergraphics.stackexchange.com/questions/4573/
float reflectanceFromIncidentRay(const Ray &ray, const Hit &rec) {
    float eta, n1, n2; 
    vec3 norm = rec.n;
    if (std::abs(rec.m->refrIndex - 1.f) < CONSTANTS::EPSILION)
        return 1.f;
    float cosX = -dot(norm, ray.getDir());
    cosX = std::clamp(cosX, -1.f, 1.f); // jus clamp lmao
    if (cosX < 0.f) { /*true if backfacing*/
        n1 = rec.m->refrIndex; 
        n2 = 1.f;
        cosX = -cosX;
        // n1 > n2
        eta = n1 / n2;
        const float sin2T = eta*eta*(1.0f-cosX*cosX);
        if (sin2T > 1.0f) { return 1.f; } // TIR
        cosX = sqrt(1.f - sin2T);
    } else {
        n1 = 1.f; 
        n2 = rec.m->refrIndex;
        norm = -norm;
    }
    const float x = 1.0f - cosX;
    float r0 = (n1 - n2)/(n1 + n2); r0 *= r0;
    return r0 + (1.0f-r0)*(x*x*x*x*x);
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
    // A modification I could make would be to make the reflCoeff itself affect the
    // Fresnel term by reflCoeff + (1 - reflCoeff)*rt, which would make transparent
    // objects fully reflective at reflCoeff = 1. Good for mixing reflection and
    // refraction independent of angle, but I need to change occlusion and shadow
    // to account for increased reflection (increase opacity of shadows when
    // reflectCoeff is higher, overriding transparency)
    // Do make the above change after reflectance refract rewrite
    // TODO: since 
    reflClr *= hit.m->reflCoeff;
    return reflClr;
}

// similar to getReflectionColor(), but can return vec3(0.f) b/c TIR
vec3 Camera::getRefractedColor(const std::unique_ptr<Scene> &scene,
                               const Ray &ray, const Hit &hit,
                               const Interval &interval, 
                               uint recursions, bool back_face) 
{
    float n1, n2;
    if (back_face) { n1 = hit.m->refrIndex; n2 = 1.f;} 
    else { n1 = 1.f; n2 = hit.m->refrIndex; }
    const Ray refrRay = refractRay(ray, hit, n1/n2);
    vec3 refrClr = vec3(0.f);
    if (vec3(0.f) != ray.getDir()) {
        refrClr = getRayColor(scene, refrRay, interval, recursions);
    } // otherwise, no refracted clr b/c TIR, modify reflected instead
    refrClr *= hit.m->transparency;
    return refrClr;
}

// Whitted-style ray-tracing, but amalmagated
vec3 Camera::getRayColor(const unique_ptr<Scene>& scene, const Ray& ray, 
                         const Interval& interval, uint recursiveDepth) 
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
    // vec3 absorbClr = vec3(0.f);

    // If the material is not refractive, keep any reflections
    float reflectance = reflectanceFromIncidentRay(ray, rec);
    const bool inReflThres = reflectance > CONSTANTS::EPSILION;
    // const bool inRefrThres = reflectance < 1.f - CONSTANTS::EPSILION;
    const bool reflective = rec.m->reflCoeff > MINIMUM_COEFF;
    const bool transparent = rec.m->transparency > MINIMUM_COEFF;

    // Determine if the ray is inside or outside the object,
    // only to handle the case of lighting for CSG.
    // Doing this means that refraction must account for
    // this possibility via parameter
    const bool back_face = dot(ray.getDir(), rec.n) > 0.0f; // true if inside

    // TODO: calculate reflectance first, and if it hits certain
    // thresholds, do not recurse one or the other

    // Avoid casting additional reflection rays if inside the object, this
    // avoids massively expensive and unnecessary computation (3x increase)
    // TIR reflections are done in refraction case
    if (reflective && !back_face && inReflThres) { 
        if (recursiveDepth >= Camera::MAX_RECURSIONS) return clr;
        reflectClr = getReflectionColor(scene, ray, rec, interval, recursiveDepth+1);
    }

    // Objects must be transparent in order to refract light.
    if (transparent) {
        if (recursiveDepth >= Camera::MAX_RECURSIONS) return clr;
        // flip the normal for refraction, if inside
        // this must be done since *every* following calculation needs to
        // compute the color as if the normal of the surface faced towards
        // the incident ray; otherwise, strange glints are caused by an
        // invisible surface. If it not nested in this if statement,
        // bright specks may appear on meshes w/ backface culling enabled.
        if (back_face) rec.n = -rec.n;  
        refractClr = getRefractedColor(scene, ray, rec, interval, 
                        recursiveDepth+1, back_face);
        
        // Deal with TIR here. dead code for now
        if (reflectance >= 1.f - CONSTANTS::EPSILION) {
            Ray refrRay = reflectRay(ray, rec);
            // this set the reflected ray outside the surface, making it bounce out
            // immediately again; this is not physically based.
            // refrRay.setPos( rec.x - 2.f*Camera::EPSILION*refrRay.getDir() );
            reflectClr = getRayColor(scene, refrRay, interval, recursiveDepth+1);
            reflectClr *= rec.m->reflCoeff;
        }
    }

    // The eye vector does not point to the camera when reflecting/refracting
    const vec3 ev = -ray.dir;
    vec3 diffuseFac = vec3(1.f);

    // Another term, ia, could be multiplied with the ambient and determined based on the 
    // distribution and number of lights throughout the scene automatically.
    // Not that important though.
    vec3 localClr = rec.ambient() + globalAmbient;
    const bool occlusionEnabled = occlusionSamples > 0 && occludingRadius > MINIMUM_COEFF;
    if (occlusionEnabled && recursiveDepth < 2) {
        // the maximum is arbitrary, but it should be small 
        // so that faraway objects are not considered
        const auto occlArea = Interval(interval.min, occludingRadius);
        const float occlFac = occlusionDiffuseFactor(rec, scene, 
            occlArea, diffuseFac, ray.time);
        // sqrt was a hack that made the shadows softer
        localClr *= occlFac;
    }

    for (auto& light : scene->getLights()) {
        const vec3 ld = light->pos - rec.x;
        const vec3 lv = normalize(ld);
        // Construct shadow rays for each light, using world coordinates.
        // Boolean parameter to naively cull the number of neglible rays casted for shadows 
        const vec3 lightVisibility = shadowFactor(light, rec, scene, 
            interval, ray.time, recursiveDepth < 2);

        float Li = light->intensity;
        localClr += lightVisibility * Li * lightingFactor(rec, lv, ev, diffuseFac);
    }

    // ki = 1 - refl*kr - (1-refl)*kt
    float localCoeff = 1.f - reflectance*rec.m->reflCoeff -
                       (1.f - reflectance)*rec.m->transparency;
    clr += localCoeff*localClr;
    clr += reflectClr*reflectance + refractClr*(1.0f-reflectance);
    // Change if reflCoeff acted like OBJECT_REFLECTIVITY from that raytracer demo
    // const float& reflMult = reflectance; // clr could be modified from refr branch
    // const float refrMult = (1.f - reflectance)*rec.m->transparency
    // const float localCoeff = 1.f - reflMult - refrMult;
    // clr += localCoeff*localClr + reflectClr*reflMult + refractClr*refrMult;
    // absorbDistance is 0 by default, meaning absorb = vec3(1). absorb-- as dist++
    // vec3 absorb = glm::exp(-OBJECT_ABSORB * absorbDistance);
    // clr *= absorb;
    clr += rec.emissive();

    return clr;
}

vec3 Camera::lightingFactor(const Hit &rec, const vec3 &lv, 
                            const vec3 &ev, const vec3 &diffAtt)
{
    const vec3 kd = rec.diffuse(), 
               ks = rec.specular();
    const float s = rec.m->exponent;

    const vec3 h = normalize(lv + ev);
    const vec3 diff_cont = kd*std::max(0.0f, glm::dot(rec.n, lv));
    const vec3 spec_cont = ks*std::pow(std::max(0.0f, glm::dot(rec.n, h)), s);
    return (diff_cont*diffAtt + spec_cont);
}

// Uses monte carlo integration
float Camera::occlusionDiffuseFactor(const Hit &rec, const unique_ptr<Scene> &scene,
                                    const Interval &interval, vec3 &diffuseFac,
                                    float time)
{
    float lightAbsorption = 0.f;
    vec3 diffuseAbsorption(0.f);

    vec3 T, B;
    assignONBvec3s(rec.n, T, B);
    vec3 aorayPos = rec.x + (float)interval.min*rec.n;

    // Reuse the ray object, yes?
    Ray aoray;
    aoray.setPos(aorayPos);
    aoray.time = time;

    const uint minConvergSamp = std::max(occlusionSamples / 4, 8U);
    const float r_tmax = 1.f / (float)interval.max;

    float currSamplesDone = static_cast<float>(occlusionSamples); 
    for (uint i = 0; i < occlusionSamples; ++i) {
        const float u1 = unifRandGen->rand();
        const float u2 = unifRandGen->rand();
        vec3 rDir = cosineSampleHemisphere(u1, u2);
        // Transform the sampled vector from tangent to world space
        rDir = vec3(rDir.x*T + rDir.y*B + rDir.z*rec.n);        
        aoray.setDir(normalize(rDir));

        Hit aoHit;
        const bool occluded = hit(scene->getShapes(), aoray, interval, aoHit);
        vec3 rayAbsorbed = vec3(0.f);

        // 5AM tomfoolery; phi function for weighting contributions
        // float `a` is an adjustment constant: higher `a` -> sqrt-like
        constexpr auto p = [](float a, float x) {
            const float df = ((a+1)/(2*a))*(-2.f/(1.f+a*x)+2.f);
            return df*df;
        };

        // For a red clr, green and blue are absorbed, but reflections and refractions
        // also need consideration. however, recursively calling getRayColor is expensive
        if (occluded && aoHit.m) {
            const float d = glm::clamp(aoHit.t * r_tmax, 0.f, 1.f);
            // Very crude approximation of color blending from diffuse reflection
            const float dCoeff = (1.f - rec.m->reflCoeff) * (1.f - rec.m->transparency);
            const vec3 kd = aoHit.diffuse() * dCoeff;
            const vec3 diff_cont = kd*std::max(0.0f, glm::dot(rec.n, rDir));
            rayAbsorbed = vec3(1.f) - diff_cont * p(.6f, d);
            // Transparent objects occlude less light. well, i think
            // the closer the occluding, the less that light reaches
            lightAbsorption += (1.f - d) * (1.f - rec.m->transparency);
        }
        diffuseAbsorption += rayAbsorbed;
        const bool cond = has_no_change(i, minConvergSamp, diffuseAbsorption, rayAbsorbed);
        // crazy, over 2x speedup with no visible changes in quality
        if (cond) { currSamplesDone = static_cast<float>(i + 1); break; }
    }
    const float r_samplesDone = 1.f / currSamplesDone;
    const float occlusionCoeff = 1.f - lightAbsorption*r_samplesDone;
    diffuseFac = vec3(1.f) - (diffuseAbsorption*r_samplesDone);
    // sqrt is a hack that make the color blends look nicer with the subtle occlusion
    diffuseFac = glm::sqrt(diffuseFac);
    
    return occlusionCoeff;
}

// Used for sampling points on spherical area lights
// at a somewhat reasonable efficiency
class sampleCone {
private:
    glm::vec3 dx, dy;
    glm::vec3 dz;
    const float radius;

    float dz_len_2;
    float sin_theta_max_2;
    float sin_theta_max;
    float cos_theta_max;
    // float r_pdf = 1.f;

public:
    // Akalin's method, ty scratchapixel for saving me from this area light torment nexus
    sampleCone(const glm::vec3 &ld, float sampleRadius) 
    : radius(sampleRadius)
    { 
        dz = ld;
        dz_len_2 = glm::dot(dz, dz);
        const float dz_len = std::sqrt(dz_len_2);
        dz /= dz_len;
        sin_theta_max_2 = radius * radius / dz_len_2;
        sin_theta_max = std::sqrt(sin_theta_max_2);
        cos_theta_max = std::sqrt(std::max(0.f, 1.f - sin_theta_max_2));

        // light attentuation
        // to 2*PI or not to 2*PI?
        // r_pdf = ((1.f - cos_theta_max));
        // const float c = 2*PI*std::max(2.f*std::sqrt(radius), radius);
        // r_pdf = 1.f - std::sqrt(std::max(0.f, 1.f - c/dz_len));
        // r_pdf *= 2*PI;

        assignONBvec3s(dz, dx, dy);
    }
    inline glm::vec3 operator()() const
    {
        // Faster to generate less random variables
        const float r1 = unifRandGen->rand();
        const float r2 = unifRandGen->rand();

        const float cos_theta = 1.f + (cos_theta_max - 1.f) * r1;
        const float sin_theta_2 = 1.f - cos_theta * cos_theta;

        const float cos_alpha = (sin_theta_2 / sin_theta_max) + 
            cos_theta * std::sqrt(1.f - sin_theta_2 / sin_theta_max_2);
        const float sin_alpha = std::sqrt(1.f - cos_alpha * cos_alpha);
        const float phi = 2 * PI * r2;

        return std::cos(phi)*sin_alpha*dx + std::sin(phi)*sin_alpha*dy + cos_alpha*dz;
    }

    // PDFs are useful for path tracing, but not for this Whitted-hybrid tracer 
    // constexpr float getrPDF() const { return r_pdf; }
};

constexpr auto aboveZero = [](const vec3 &clr) { return clr.r > 0 || clr.g > 0 || clr.b > 0; };
vec3 Camera::getShadowContrib(vector<Hit> &srecs, const Ray &sray,
                               const std::unique_ptr<Scene> &scene, 
                               const Interval &t_int) {  
    if (FULL_SHADOWS) {
        Hit srec;
        const bool behindShape = hit(scene->getShapes(), sray, t_int, srec);
        const bool isEmiss = srec.m && aboveZero(srec.emissive());
        return vec3(static_cast<float>(!behindShape || isEmiss));
    } else {
        // 1.0f is fully lit by default, which is when point has unobstructed path to light
        vec3 s_transparency(1.f); // if behind, return value from 0.0f to 1.0f
        hit(scene->getShapes(), sray, t_int, srecs);
        for (const Hit& srec : srecs) {
            const bool isTrns = srec.m && srec.m->transparency > Camera::MINIMUM_COEFF;
            const bool isEmiss = srec.m && aboveZero(srec.emissive());
            float trnsMult = (isTrns || !isEmiss)*srec.m->transparency + isEmiss;
            trnsMult = std::clamp(trnsMult, 0.f, 1.f);
            const vec3 diff_cont = srec.diffuse()*std::max(0.0f, dot(srec.n, sray.getDir()));
            // weird behavior with spheres perhaps (the transparency being
            // very low but not zero, and the diffuse being strong)
            // could fix by trnsMult * sum, but darker shadows and weaker color
            s_transparency *= vec3(trnsMult) + (1.f - trnsMult)*isTrns*diff_cont;
        }
        srecs.clear();
        return s_transparency;
    }        
};

// Randomly samples points on area lights depending on their radius.
vec3 Camera::shadowFactor(const shared_ptr<Light> &light, const Hit &rec, 
                           const unique_ptr<Scene> &scene,
                           const Interval &interval,
                           float time, bool sampleArea) 
{
    const vec3 ld = light->pos - rec.x;
    const float tl = length(ld);
    const vec3 lv = ld / tl;

    Ray sray;
    vec3 srayCPos = rec.x + (float)interval.min*rec.n;
    sray.setPos(srayCPos);
    sray.setDir(lv);
    sray.time = time;

    // Hit srec;
    vector<Hit> srecs;
    srecs.reserve(16);

    if (!sampleArea || light->getRadius() < MINIMUM_COEFF) { 
        return getShadowContrib(srecs, sray, scene, Interval(interval.min, tl)); 
    }
    
    const auto sampler = sampleCone(ld, light->getRadius());
    const int min_i = std::max(light->getSamples() / 4, 8);

    // Use this method instead of has_no_change() since it plays nicer with 
    // shadow implementation (prev. method did not give good results)
    VarianceCounter<vec3> counter;
    const Interval litThreshold(CONSTANTS::EPSILION, 1.f - CONSTANTS::EPSILION);

    for (int i = 0; i < light->getSamples(); ++i) {
        // A large enough light radius increases noise of the entire image 
        const vec3 sampLightPos = light->pos + sampler()*light->getRadius();
        const vec3 new_ld = sampLightPos - rec.x;
        const float tmax = length(new_ld);
        const vec3 new_lv = new_ld / tmax;
        sray.setDir(new_lv);
        
        const vec3 contrib = getShadowContrib(srecs, sray, 
            scene, Interval(interval.min, tmax));
        bool lowVari = counter.add(contrib, CounterCmps::vec3_cmp);

        if (i < min_i) continue;  
        // use dot product to avoid comparing 3 components (convenience),
        const float dotMean = dot(counter.getMean(), counter.getMean());
        const float currVis = dotMean / (3.f*counter.getSamplesDone());
        const bool fullOrNoLit = litThreshold.surrounds(currVis);
        if (lowVari || fullOrNoLit) { break; }         
    }
    return counter.getMean();
}