#include "Raytracer.hpp"

#include "../stn.hpp"
// #include "../Scene.hpp"
// #include "../Camera.hpp"

#include "../util/counter.hpp"
#include "../util/prand.hpp"

#include <exception>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using std::unique_ptr;
using std::vector;

/*
The only difference between RecursiveTracer and SphereTracer
is the getRayColor() function, so share the multithreading/task
delegation code by putting them in Raytracer.hpp

- inheritance by overriding only getRayColor()
-x make scene and camera be members of Raytracer.hpp
-x compute P and V matrices inside render(), using a bound camera object 

-x there should be checks to ensure that a camera and
scene object are bound when render() is called

-x the sky should belong in Scene.hpp,
-x camera settings should become public

-x random number gens should be held as public static members

// make variables computed in render() as
// private variables here, because only getRayColor()
// needs to be overridden

// TODO: for cast secondary ray, need to copy some members of camera
// as the raytracer's members so that castSecondary ray can be used;
// want to minimize the overhead for this 

// TODO: invariants must be maintained at the accessor level

// TODO: TScene class, then try to derive into SphereTracer


// NOTE: I would need to change many SceneLoader functions
// also NOTE: this may be a good opportunity to rewrite
// the entirety of sceneloader to be less of a mess
*/

constexpr glm::vec3 NOT_IMPL_CLR = glm::vec3(1.f, 0.f, 1.f);

prand::diskRand Raytracer::diskRandGen;
prand::uniformRand Raytracer::unifRandGen;

void Raytracer::setScene(unique_ptr<Scene>&& scene) {
    this->scene = std::move(scene);
}

void Raytracer::setCamera(unique_ptr<Camera>&& camera) {
    this->camera = std::move(camera);
    this->camera->validateLookAtVectors();
}

void Raytracer::applyProjection(MatrixStack& MS) 
{
    auto& cam = this->camera;
    MS.mult(glm::perspective(cam->fovy, cam->aspectRatio, cam->znear, cam->zfar));
}
void Raytracer::applyView(MatrixStack& MS) 
{
    auto& cam = this->camera;

    MS.push();
    MS.translate(cam->translation);
    // yaw, pitch, then roll
    MS.rotate(cam->rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
    MS.rotate(cam->rotation.y, glm::vec3(1.0f, 0.0f, 0.0f));
    MS.rotate(cam->rotation.x, glm::vec3(0.0f, 1.0f, 0.0f));

    // inverse of view matrix so that the 
    // center, eye, and up vectors are 
    // specified wrt old transforms
    const auto cameraMat = glm::inverse(MS.top());

    glm::vec3 center = cam->lookAtPos, 
    eye = cameraMat * glm::vec4(cam->position, 1.f), 
    up  = cameraMat * glm::vec4(cam->camUpVec, 0.f);

    glm::mat4 lookAtMat = glm::lookAt(eye, center, up);

    // Check for NaNs in the lookAt matrix
    for (int i = 0; i < lookAtMat.length(); ++i) {
        if (glm::any(glm::isnan(lookAtMat[i]))) {
            std::cerr << "NaNs detected in lookAtMat; lookAtMat not applied.\n";
            return; 
        }
    }

    MS.pop();
    MS.mult(lookAtMat);
}

unique_ptr<Image> Raytracer::render() 
{
    if (!scene || !camera) 
        throw std::logic_error("scene or camera not bound");

    MatrixStack matStk;
    matStk.push();
    this->applyProjection(matStk);
    glm::mat4 P = matStk.top();
    matStk.pop();
    matStk.push();
    this->applyView(matStk);
    glm::mat4 V = matStk.top();
    matStk.pop();

    // Precompute as much as possible before loops
    uint width = camera->width, height = camera->height;
    C = glm::inverse(V);
    invP = glm::inverse(P);
    cameraPos = C[3]; 
    cameraPos.w = 1.0f;
    f_width = static_cast<float>(width);
    f_height = static_cast<float>(height);

    // Camera basis vectors in world space
    dof_u = C[0]; // right
    dof_v = C[1]; // up

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
    uint totalCasts = height * width;
    // horrific; +1 thread than cores works b/c it's i/o bound (sleep)
    auto countScans = [this, jobsFinished](uint totalCasts, uint numThreads) 
    {
        while (r_queue.rowsProcessed < camera->height && jobsFinished < numThreads) 
        {
            std::clog << '\r' << r_queue.rowsProcessed*camera->width << '/' 
                    << totalCasts << " scans completed " << std::flush;
            // Results in displayed times being larger than actual times
            // for very simple and fast scenes, but less thread switching 
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
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
    glm::vec4 focalPoint = pray.pos + camera->focusLength*pray.dir;
    focalPoint.w = 1.f;

    const glm::vec2 samp = camera->focalRadius * diskRandGen.rand();
    const glm::vec4 wld_offset = glm::vec4(dof_u*samp.x + dof_v*samp.y);

    Ray dray;
    dray.pos = pray.pos + wld_offset; 
    dray.pos.w = 1.f; // to correct for wld_offset having a w =/= 1
    dray.dir = glm::normalize(focalPoint - dray.pos);
    return dray;
}

void Raytracer::setRow(unique_ptr<Image>& image, uint y) 
{
    for (uint x = 0; x < camera->width; ++x) 
    {
        glm::vec3 color = glm::vec3(0.0f);

        Pixel pxl{x, y};
        Ray cray = castPrimaryRay(pxl, glm::vec2(.5f));
        color = getRayColor(cray);
        
        // breakpoints have experimentally OK magic numbers
        const uint breakpoint = std::max(camera->samplesPerPixel / 4, 8U);

        VarianceCounter<glm::vec3> s_counter;
        s_counter.add(color);
        
        for (uint i = 1; i < camera->samplesPerPixel; ++i) 
        {
            const glm::vec2 offset = 0.5f*diskRandGen.rand(i) + 0.5f;
            cray = castPrimaryRay(pxl, offset);

            const bool useSecRay = camera->focalRadius > Raytracer::EPSILION;
            Ray dray = useSecRay ? castSecondaryRay(cray) : cray;

            glm::vec3 drayColor = getRayColor(cray);
            bool lowVari = s_counter.add(drayColor);
            
            // Stop sampling this pixel if the contribution
            // of the new sample is < epsilion values for all comps
            if (lowVari && i > breakpoint) break;
        }

        color = s_counter.getMean();
        
        image->setPixel(x, y, color);
    }
}

glm::vec3 Raytracer::getRayColor(const Ray& ray) const 
{
    switch(this->mode)
    {
        case RenderMode::Raymarch:
            return rayMarch(ray);
        default:
            return NOT_IMPL_CLR;
    }   
}