#include "Raytracer.hpp"

#include "../stn.hpp"
#include "../Scene.hpp"
#include "../Camera.hpp"

#include "../util/counter.hpp"

#include <exception>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using std::unique_ptr;
using std::vector;

unique_ptr<Image> Raytracer::render() 
{
    if (!scene || !camera) 
        throw std::logic_error("scene or camera not bound");

    MatrixStack matStk;
    matStk.push();
    camera->applyProjection(matStk);
    glm::mat4 P = matStk.top();
    matStk.pop();
    matStk.push();
    camera->applyView(matStk);
    glm::mat4 V = matStk.top();
    matStk.pop();
    
                                                    uint width = 1, height = 1;

    // Precompute as much as possible before loops
    C = glm::inverse(V);
    invP = glm::inverse(P);
    cameraPos = C[3]; 
    cameraPos.w = 1.0f;
    f_width = static_cast<float>(width);
    f_height = static_cast<float>(height);

    // Camera basis vectors in world space
    dof_u = C[0]; // right
    dof_v = C[1]; // up

    uint totalCasts = height*width;

    unique_ptr<Image> image = std::make_unique<Image>(width, height);

    uint numThreads = std::thread::hardware_concurrency();
    if (numThreads < 1) numThreads = 1;
    std::clog << "Threads available: " << numThreads << '\n';

    // Push rows as jobs
    for (uint y = 0; y < height; ++y) r_queue.push(y);

    vector<std::thread> threads;
    for (uint i = 0; i < numThreads; ++i) 
    {
        // passing by non-const ref dangerous!!! thank goodness only 1
        // thread processes 1 row at a time
        auto worker = std::thread(&Raytracer::processRows, this, std::ref(image));
        threads.push_back(std::move(worker));
    }

    uint jobsFinished = 0;
    // horrific; +1 thread than cores works b/c it's i/o bound (sleep)
    auto countScans = [this, jobsFinished](uint totalCasts, uint numThreads) 
    {
        // while (r_queue.rowsProcessed < height && jobsFinished < numThreads) 
        // {
        //     std::clog << '\r' << r_queue.rowsProcessed*width << '/' 
        //             << totalCasts << " scans completed " << std::flush;
        //     // Results in displayed times being larger than actual times
        //     // for very simple and fast scenes, but less thread switching 
        //     std::this_thread::sleep_for(std::chrono::milliseconds(50));
        // }
    };
    std::thread counter(countScans, totalCasts, numThreads);
    for (auto& thread : threads) 
    {
        if (!thread.joinable()) continue;
        thread.join();
        ++jobsFinished;
    }
    if (counter.joinable()) counter.join();

    std::clog << "\rDone." << std::string(30, ' ') << '\n';

    return image;
};

void Raytracer::processRows(unique_ptr<Image> &image) 
{
    while (!r_queue.empty()) 
    {
        RowQueue::extract_pair pair = r_queue.pop();
        if (!pair.success) return;
        setRow(image, pair.row);
        r_queue.rowsProcessed++;
    }
}



Ray Raytracer::castPrimaryRay(Pixel p, const glm::vec2 &offset) const {
    const float ndc_y = 2.f*((float)p.y + offset.y)/(f_height) - 1.f;
    const float ndc_x = 2.f*((float)p.x + offset.x)/(f_width) - 1.f;

    const glm::vec4 rayClip(ndc_x, ndc_y, -1.0f, 1.0f);
    glm::vec4 rayEye = invP*rayClip;
    rayEye.w = 0.0f; // The ray is a direction, so set w to 0 to ignore translations

    Ray cray; 
    cray.pos = cameraPos;
    cray.dir = glm::normalize(C*rayEye);
    
    // Using the uniform distribution was too regular
    const glm::vec2 rndVec = diskRandGen.rand();
    cray.time = std::fmod(dot(rndVec, rndVec), 1.f);

    return cray;
}

Ray Raytracer::castSecondaryRay(const Ray &pray) const {
    // glm::vec4 focalPoint = pray.pos + camera->focusLength*pray.dir;
    // focalPoint.w = 1.f;

    // const glm::vec2 samp = camera->focalRadius * diskRandGen.rand();
    // const glm::vec4 wld_offset = vec4(dof_u*samp.x + dof_v*samp.y);

    Ray dray;
    // dray.pos = pray.pos + wld_offset; 
    // dray.pos.w = 1.f; // to correct for wld_offset having a w =/= 1
    // dray.dir = glm::normalize(focalPoint - dray.pos);
    return dray;
}

void Raytracer::setRow(unique_ptr<Image>& image, uint y) 
{
    // for (uint x = 0; x < width; ++x) 
    {
        glm::vec3 color = glm::vec3(0.0f);

        // Pixel pxl{x, y};
    //     Ray cray = castPrimaryRay(pxl, glm::vec2(.5f));
    //     color = getRayColor(cray);
        
    //     // breakpoints have experimentally OK magic numbers
    //     const uint breakpoint = std::max(AAsamples / 4, 8U);

    //     VarianceCounter<glm::vec3> s_counter;
    //     s_counter.add(color, CounterCmps::vec3_cmp);
        
    //     for (uint i = 1; i < AAsamples; ++i) 
    //     {
    //         const glm::vec2 offset = 0.5f*diskRandGen.rand(i) + 0.5f;
    //         cray = castPrimaryRay(pxl, offset);

    //         const bool useSecRay = camera->focalRadius > Camera::EPSILION;
    //         Ray dray = useSecRay ? castSecondaryRay(cray) : cray;

    //         const glm::vec3 rayColor = getRayColor(scene, dray);
    //         bool lowVari = s_counter.add(rayColor, CounterCmps::vec3_cmp);
            
    //         // Stop sampling this pixel if the contribution
    //         // of the new sample is < epsilion values for all comps
    //         if (lowVari && i > breakpoint) break;
    //     }

    //     color = s_counter.getMean();
        
        // image->setPixel(x, y, color);
    }
}