#include "../stn.hpp"
#include "SphereTracer.hpp"
#include "../Ray.hpp"

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

void processRows(const std::unique_ptr<Scene> &scene, std::unique_ptr<Image> &image) 
{

}

void setRow(const std::unique_ptr<Scene> &scene, std::unique_ptr<Image> &image, uint y);





float dist_to_sphere(vec3 p, vec3 center, float radius)
{
    return glm::length(p - center) - radius;
}

constexpr int MAX_STEPS = 10;
vec3 getRayColor(const unique_ptr<Scene> &scene, const Ray &ray, 
                 const Interval &interval = Interval(EPSILION, MAX_DIST)) 
{
    float total_dist = 0.0f;
    const int MAX_STEPS = 32;
    const float MIN_HIT_DIST = interval.min;
    const float MAXIMUM_TRACE_DISTANCE = interval.max;

    for (int i = 0; i < MAX_STEPS; ++i) 
    {
        vec3 curr_pos = ray.getPos() + (i * total_dist) * ray.getDir();

        float dist_to_sdf = dist_to_sphere(curr_pos, vec3(0.f), 1.f);

        if (dist_to_sdf < MIN_HIT_DIST) 
        {
            return vec3(0.f, 1.f, 0.f);
        }
        if (total_dist > MAXIMUM_TRACE_DISTANCE) 
        {
            break;
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