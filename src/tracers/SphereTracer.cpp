#include "../stn.hpp"
#include "SphereTracer.hpp"
#include "../Ray.hpp"
#include "../util/counter.hpp"

#include <iostream>
#include <vector>

using glm::vec3, glm::mat4;

using CONSTANTS::EPSILION;
constexpr float MAX_DIST = 1000.f;

using std::unique_ptr;
using std::vector;

// // ...Basic working example for raymarcher here

// // ????

// // TODO: copy the render(), setRow(), processRows(), and alldat

vec3 getRayColor(const unique_ptr<Scene> &scene, const Ray &ray, 
                 const Interval &interval = Interval(CONSTANTS::EPSILION, MAX_DIST));

unique_ptr<Image> SphereTracer::render(unique_ptr<Scene>& scene, const mat4& P, const mat4& V) 
{
    // Precompute as much as possible before loops
    C = glm::inverse(V);
    invP = glm::inverse(P);
    cameraPos = C[3]; 
    cameraPos.w = 1.0f;

    // V matrix is nans for some reason, need to debug camera->applyView

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

void SphereTracer::setRow(const std::unique_ptr<Scene> &scene, std::unique_ptr<Image> &image, uint y)
{
    for (uint x = 0; x < width; ++x) {
        vec3 color = vec3(0.0f);

        Ray cray = castPrimaryRay(x, y);
        color = getRayColor(scene, cray);
        
        // breakpoints have experimentally OK magic numbers
        // const uint breakpoint = 8U;

        VarianceCounter<vec3> s_counter;
        s_counter.add(color, CounterCmps::vec3_cmp);
        
        // for (uint i = 1; i < AAsamples; ++i) {
        //     const vec2 offset = 0.5f*diskRandGen.rand(i) + 0.5f;
        //     cray = castPrimaryRay(x, y, offset);

        //     const bool useSecRay = focalRadius > Camera::EPSILION;
        //     Ray dray = useSecRay ? castSecondaryRay(cray) : cray;

        //     const vec3 rayColor = getRayColor(scene, dray);
        //     bool lowVari = s_counter.add(rayColor, CounterCmps::vec3_cmp);
            
        //     // Stop sampling this pixel if the contribution
        //     // of the new sample is < epsilion values for all comps
        //     if (lowVari && i > breakpoint) break;
        // }

        color = s_counter.getMean();
        
        image->setPixel(x, y, color);
    }
}


float dist_to_sphere(vec3 p, vec3 center, float radius)
{
    return glm::length(p - center) - radius;
}

float sceneSDF(vec3 p) {
    // why does this shrink to zero as dist from
    // camera approach 3?
    float sphere0 = dist_to_sphere(p, vec3(0.f, 0.f, 0.25f), 1.f);

    return sphere0;
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


    // negate b/c conventions???? image flipping why?
    return glm::normalize(vec3(-gX, gY, -gZ));
}

vec3 getRayColor(const unique_ptr<Scene> &scene, const Ray &ray, const Interval &interval) 
{
    float total_dist = 0.0f;
    const int MAX_STEPS = 32;
    const float MIN_HIT_DIST = interval.min;
    const float MAXIMUM_TRACE_DIST = interval.max;

    for (int i = 0; i < MAX_STEPS; ++i) 
    {
        vec3 curr_pos = ray.getPos() + (i * total_dist) * ray.getDir();

        float dist_to_sdf = sceneSDF(curr_pos);

        if (dist_to_sdf < MIN_HIT_DIST) 
        {
            // inside
            vec3 normal = sceneNormal(curr_pos);

            vec3 light_position = vec3(2.0, 5.0, 3.0);

            // Calculate the unit direction vector that points from
            // the point of intersection to the light source
            vec3 direction_to_light = glm::normalize(light_position - curr_pos);

            const vec3 h = normalize(direction_to_light - ray.getDir());
            float dI = std::max(0.f, glm::dot(normal, direction_to_light));
            float sI = std::pow(std::max(0.0f, glm::dot(normal, h)), 100.f);

            return vec3(0.1) + vec3(0.0, 1.0, 0.0)*dI + vec3(1.0f)*sI;

        }
        if (total_dist > MAXIMUM_TRACE_DIST) 
        {
            // miss
            return vec3(0.f);
        }

        total_dist += dist_to_sdf;
    }

    return vec3(0.1f);
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