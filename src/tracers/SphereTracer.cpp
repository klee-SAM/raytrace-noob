#include "../stn.hpp"
#include "SphereTracer.hpp"
#include "../Ray.hpp"
#include "../util/counter.hpp"

#include <iostream>
#include <vector>

#include <glm/gtc/noise.hpp>

#include "../util/prand.hpp"

using glm::vec3, glm::vec4, glm::mat4;

using CONSTANTS::EPSILION;
constexpr float MIN_DIST = 0.005f;
constexpr float MAX_DIST = 1000.f;

using std::unique_ptr;
using std::vector;

// ...Basic working example for raymarcher here

// ????

/*
The only difference between RecursiveTracer and SphereTracer
is the getRayColor() function, so share the multithreading/task
delegation code by putting them in Raytracer.hpp

- inheritance by overriding only getRayColor()
- make scene and camera be members of Raytracer.hpp
- compute P and V matrices inside render(), using a bound camera object 

- there should be checks to ensure that a camera and
scene object are bound when render() is called

- the sky should belong in Scene.hpp,
- camera settings should become public

- random number gens should be held as public static members

// make variables computed in render() as
// private variables here, because only getRayColor()
// needs to be overridden

// TODO: for cast secondary ray, need to copy some members of camera
// as the raytracer's members so that castSecondary ray can be used;
// want to minimize the overhead for this 

*/

vec3 getRayColor(const unique_ptr<Scene> &scene, const Ray &ray, 
                 const Interval &interval = Interval(MIN_DIST, MAX_DIST));

unique_ptr<Image> SphereTracer::render(unique_ptr<Scene>& scene, const mat4& P, const mat4& V) 
{
    // Precompute as much as possible before loops
    C = glm::inverse(V);
    invP = glm::inverse(P);
    cameraPos = C[3]; 
    cameraPos.w = 1.0f;

    // Camera basis vectors in world space
    dof_u = C[0]; // right
    dof_v = C[1]; // up

    // if (AAsamples < 1) AAsamples = 1;

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
        threads.push_back(std::move(std::thread(&SphereTracer::processRows, this, 
            std::ref(scene), std::ref(image))));
    }

    // horrific; +1 thread than cores works b/c it's i/o bound (sleep)
    auto countScans = [this, numThreads, totalCasts, jobsFinished]() {
        while (r_queue.rowsProcessed < height && jobsFinished < numThreads) {
            std::clog << '\r' << r_queue.rowsProcessed*width << '/' 
                    << totalCasts << " scans completed " << std::flush;
            // Results in displayed times being larger than actual times
            // for very simple and fast scenes, but less thread switching 
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
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

void SphereTracer::processRows(const std::unique_ptr<Scene> &scene, std::unique_ptr<Image> &image) 
{
    while (!r_queue.empty()) {
        RowQueue::extract_pair pair = r_queue.pop();
        if (!pair.success) return;
        setRow(scene, image, pair.row);
        r_queue.rowsProcessed++;
    }
}

Ray SphereTracer::castPrimaryRay(uint idx, uint idy, const glm::vec2 &offset) const {
    // https://www.realtimerendering.com/blog/the-center-of-the-pixel-is-0-50-5/
    const float ndc_y = 2.f*((float)idy + offset.y)/((float)height) - 1.f;
    const float ndc_x = 2.f*((float)idx + offset.x)/((float)width) - 1.f;

    const glm::vec4 rayClip(ndc_x, ndc_y, -1.0f, 1.0f);
    glm::vec4 rayEye = invP*rayClip;
    rayEye.w = 0.0f; // The ray is a direction, so set w to 0 to ignore translations

    Ray cray; 
    cray.pos = cameraPos;
    cray.dir = glm::normalize(C*rayEye);
    // cray.pos = glm::vec4(0.f, 0.f, -2.f, 1.f);
    // cray.dir = glm::vec4(ndc_x, ndc_y, 1.f, 0.f);
    
    // Using the uniform distribution was too regular
    // const vec2 rndVec = diskRandGen.rand();
    // cray.time = std::fmod(dot(rndVec, rndVec), 1.f);

    return cray;
}

static prand::diskRand diskRandGen; // I tell myself that this is temporary

void SphereTracer::setRow(const std::unique_ptr<Scene> &scene, std::unique_ptr<Image> &image, uint y)
{
    for (uint x = 0; x < width; ++x) {
        vec3 color = vec3(0.0f);

        Ray cray = castPrimaryRay(x, y);
        color = getRayColor(scene, cray);
        
        // breakpoints have experimentally OK magic numbers
        const uint breakpoint = 8U;

        VarianceCounter<vec3> s_counter;
        s_counter.add(color, CounterCmps::vec3_cmp);

        const uint AAsamples = 16U;

        for (uint i = 1; i < AAsamples; ++i) {
            const glm::vec2 offset = 0.5f*diskRandGen.rand(i) + 0.5f;
            cray = castPrimaryRay(x, y, offset);

            // const bool useSecRay = focalRadius > Camera::EPSILION;
            // Ray dray = useSecRay ? castSecondaryRay(cray) : cray;

            // const vec3 rayColor = getRayColor(scene, dray);
            const vec3 rayColor = getRayColor(scene, cray);
            bool lowVari = s_counter.add(rayColor, CounterCmps::vec3_cmp);
            
            // Stop sampling this pixel if the contribution
            // of the new sample is < epsilion values for all comps
            if (lowVari && i > breakpoint) break;
        }

        color = s_counter.getMean();
        
        image->setPixel(x, y, color);
    }
}


float dist_to_sphere(vec3 p, vec3 center, float radius)
{
    return glm::length(p - center) - radius;
}

// good to do
// Multiple shapes
// CSG operations (unions, intersections, subtractions, etc.)
// Fractal SDFs
// Shadows and/or ambient occlusion

// https://raw.githubusercontent.com/pedrotrschneider/shader-fractals/refs/heads/main/3D/Mandelbox.glsl
// Some of the following SDF code was ripped from the above source, with some
// modifications. I really liked this.

// Converts a color from the HSV colorspace to RGB
vec3 hsv2rgb (vec3 c) {
  vec4 K = vec4 (1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
  vec3 hue_comp = vec3(c.x);
  vec3 p = glm::abs((glm::fract(hue_comp + vec3(K)) * 6.f) - K.w);
  return c.z * glm::mix(vec3(K.x), glm::clamp(p - vec3(K.x), 0.f, 1.f), c.y);
}

// SDF FUNCTIONS //
vec4 sphere(vec4 z) {
  float r2 = glm::dot(vec3(z), vec3(z));
  if (r2 < 2.0)
    z *= (1.0 / r2);
  else z *= 0.5;

  return z;
}

vec3 box(vec3 z) {
  return glm::clamp(z, vec3(-1.0), vec3(1.0)) * 2.f - z;
}

// mandelbox
float DE2 (vec3 pos) {
  vec3 params = vec3(0.5f);
  vec4 scale = vec4(-20.f * 0.272321);
  vec4 p = vec4(pos, 1.f);
  vec4 c = vec4(params, 0.5f) - 0.5f; // param = 0..1

  for (float i = 0.0; i < 10.0; i++) {
    vec3 b = box(vec3(p));
    p.x = b.x;
    p.y = b.y;
    p.z = b.z;
    p = sphere(p);
    p = p * scale + c;
  }

  return glm::length(vec3(p)) / p.w;
}

float sceneSDF(vec3 p) {
    // why does this shrink to zero as dist from
    // camera approach 3?
    // answer: i multipled by i, and it just did that for some reason
    // float s = 2.5f;
    // p.x = p.x - s*std::round(p.x/s);
    // p.y = p.y - s*std::round(p.y/s);
    // float sphere0 = dist_to_sphere(p, vec3(0.f, 0.f, 0.f), 1.f);
    // float k = 4.f;
    // float displacement = cos(k*p.x) * cos(k*p.y) * sin(k*p.z) * .25f;
    // float dP = glm::perlin(p);
    
    // return sphere0 + displacement;
    return DE2(p);
}

vec3 sceneNormal(vec3 p) {
    // Observe how the SDF output changes to get the normal;
    // also known as calculating the gradient

    constexpr float SMALL_STEP = 0.01f;
    constexpr vec3 stepX = vec3(SMALL_STEP, 0.f, 0.f);
    constexpr vec3 stepY = vec3(0.f, SMALL_STEP, 0.f);
    constexpr vec3 stepZ = vec3(0.f, 0.f, SMALL_STEP);

    float gX = sceneSDF(p + stepX) - sceneSDF(p - stepX);
    float gY = sceneSDF(p + stepY) - sceneSDF(p - stepY);
    float gZ = sceneSDF(p + stepZ) - sceneSDF(p - stepZ);

    return glm::normalize(vec3(gX, gY, gZ));
}

vec3 getRayColor(const unique_ptr<Scene> &scene, const Ray &ray, const Interval &interval) 
{
    float total_dist = 0.0f;
    const int MAX_STEPS = 128;
    const float MIN_HIT_DIST = interval.min;
    const float MAXIMUM_TRACE_DIST = interval.max;

    float min_dist_to_sdf = CONSTANTS::INF;
    vec3 min_dist_to_sdf_pos = ray.getPos();

    for (int i = 0; i < MAX_STEPS; ++i) 
    {
        vec3 curr_pos = ray.getPos() + total_dist * ray.getDir();

        float dist_to_sdf = sceneSDF(curr_pos);

        if (min_dist_to_sdf > dist_to_sdf) {
            min_dist_to_sdf = dist_to_sdf;
            min_dist_to_sdf_pos = curr_pos;
        }

        if (dist_to_sdf < MIN_HIT_DIST) 
        {
            // inside
            vec3 normal = sceneNormal(curr_pos);

            // handedness is consistent actually; +x goes to the right
            vec3 light_position = vec3(2.0, 5.0, 3.0);

            // Calculate the unit direction vector that points from
            // the point of intersection to the light source
            vec3 direction_to_light = glm::normalize(light_position - curr_pos);

            const vec3 h = normalize(direction_to_light - ray.getDir());
            float dI = std::max(0.f, glm::dot(normal, direction_to_light));
            float sI = std::pow(std::max(0.0f, glm::dot(normal, h)), 100.f);

            vec3 Kd = vec3(0.0078, 0.6745, 0.8392);

            const float hF = 10.0, sat = 1.0, val = 0.8;
            const vec3 hsvClr = vec3(0.8 + (glm::length(curr_pos) / hF), sat, val);
            Kd = glm::mix(Kd, hsv2rgb(hsvClr), .35f);

            vec3 clr = vec3(0.1) + Kd*dI + vec3(1.0f)*sI;

            clr /= i * 0.08; // Ambient occlusion
            clr /= glm::distance(ray.getPos(), min_dist_to_sdf_pos);
            clr *= 2.0;            

            return clr;
            // return vec3(0.f, 1.f, 0.f);
        }
        if (total_dist > MAXIMUM_TRACE_DIST) 
        {
            // miss
            break;
        }

        total_dist += dist_to_sdf;
    }

    return vec3(0.3f, 0.4f, 0.6f) * 0.5f;
}


// float march() {
//     float start = 0.f; 
//     float MAX_MARCHING_STEPS = 20.f;
//     float eye, depth, viewRayDirection;

//     float sceneSDF(float x);
//     float EPSILON = 0.005f;

//     float depth = start;
//     float end;

//     for (int i = 0; i < MAX_MARCHING_STEPS; i++) {
//         float dist = sceneSDF(eye + depth * viewRayDirection);
//         if (dist < EPSILON) {
//             // We're inside the scene surface!
//             return depth;
//         }
//         // Move along the view ray
//         depth += dist;

//         if (depth >= end) {
//             // Gone too far; give up
//             return end;
//         }
//     }
//     return end;
// }