#pragma once
#include "stn.hpp"

#include "Image.hpp"
#include "MatrixStack.hpp"
#include "Scene.hpp"
#include "Ray.hpp"
#include "umath.hpp"

// For processing image rows in parallel using a limited number of threads.
class RowQueue {
private:
    std::queue<uint> rqueue;
    mutable std::mutex mut;
public:
    std::atomic<uint> rowsProcessed;
    struct extract_pair { uint row; bool success; };

    RowQueue(): rqueue(), mut(), rowsProcessed(0U) {};
    
    // Assume that threads will only ever pop from the queue, and 
    // that the job of delegating tasks to threads is single-threaded.
    inline void push(const uint& val) { rqueue.push(val); }
    
    extract_pair pop() {
        std::unique_lock<std::mutex> lock(mut);
        extract_pair pair;
        // No more rows to process in the queue, so terminate early.
        if (rqueue.empty()) { 
            pair.success = false; 
            return pair; 
        }

        pair.row = rqueue.front();
        rqueue.pop();
        pair.success = true;
        return pair;
    }

    inline bool empty() const { return rqueue.empty(); }
};

class Camera {
public:
    // I could make these modifible via json files
    static constexpr float EPSILION = 5E-3f; // having a low epsilion value does not bode well with meshes
    static constexpr float MAX_DIST = std::numeric_limits<float>::max();
    static constexpr uint MAX_RECURSIONS = 7;
    static constexpr float MINIMUM_COEFF = 0.005f;

    Camera() : translation(0.f), rotation(0.f),
               position(0.f), lookAtPos(0.f), camUpVec{0.f, 1.f, 0.f},
               aspectRatio(1.0), fovy(glm::radians(45.0)), 
               width(1), height(1) { }
    Camera(uint w, uint h) : Camera() {
        this->width = w; this->height = h;
        this->aspectRatio = (double)width/(double)height;
    }
    // fov is in degrees
    Camera(uint w, uint h, double fov) : Camera(w, h) {
        this->fovy = glm::radians(fov);
        this->aspectRatio = (double)width/(double)height;
    }
    Camera(uint totalPixels, double aspect) : Camera() {
        assert(aspect > 0.0);

        this->aspectRatio = aspect;
        this->width = std::sqrt(totalPixels*aspect);
        this->height = std::sqrt(totalPixels*(1.0/aspect));
    }

    virtual ~Camera() = default;

    void applyProjection(MatrixStack&);
    void applyView(MatrixStack&);

    void setAspect(double a) { aspectRatio = a; }
    void setFOV(double FOVdeg) { fovy = glm::radians(FOVdeg); }

    // Provided for compatibility; don't use these
    void setInitDistance(double dist) { translation.z = -std::abs(dist); }
    void setTranslation(const glm::vec3& pos) { translation = pos; }
    void setRotation(const glm::vec3& rot) { rotation = rot; }

    void setCameraPos(const glm::vec3& pos) { position = pos; }
    void setLookAtPos(const glm::vec3& pos) { lookAtPos = pos; }
    // As a failsafe, make the up vector face in the +y axis
    // if the magnitude of upVec is 0.
    void setUpVector(const glm::vec3& upVec) { 
        if (glm::length(upVec) < CONSTANTS::EPSILION) {
            std::cerr << "Provided up vector has a length near zero\n";
            camUpVec = glm::normalize(upVec+vec3(0.f, EPSILION, 0.f)); 
        } else { camUpVec = glm::normalize(upVec); }
    }

    // Setting samples below 2 disables antialiasing.
    void setAntialiasSamples(uint count) { AAsamples = count > 1 ? count : 1; }
    // Ambient occlusion samples are taken for every color ray, including
    // anti-aliasing rays; recommended to reduce AO samples if increasing AA rays
    void setAmbientOcclusionSamples(uint count) { occlusionSamples = count > 0 ? count : 0; }
    void setGlobalAmbientColor(const glm::vec3& clr) { globalAmbient = clr; }
    void setAmbientOccludingRadius(float r) { occludingRadius = r; }

    enum class SkyType {Void, Haze};
    void setSky(SkyType s) { sky = s; }

    std::unique_ptr<Image> render(std::unique_ptr<Scene>&, const glm::mat4&, const glm::mat4&);
    void setRow(const std::unique_ptr<Scene>& scene, std::unique_ptr<Image>& image, uint y);
    void processRows(const std::unique_ptr<Scene>& scene, std::unique_ptr<Image>& image);

private:
    RowQueue r_queue;

    // Relative translation, which is indirectly used in computing cameraPos
    glm::vec3 translation;
    // Rotate the scene around the origin by angles specified in each axis in radians.
    glm::vec3 rotation; 

    glm::vec3 position;    // Position of the camera in world-space
    glm::vec3 lookAtPos;   // position the camera looks at in world-space      
    glm::vec3 camUpVec;    // Up vector of the camera. 

    double aspectRatio;
    double fovy; // radians
    double znear = 1.0f; 
    double zfar = 1000.0f;
    uint width, height;

    uint AAsamples = 1;                        // must be at least 1
    uint occlusionSamples = 0;                 // actual count is divided by AA samples
    glm::vec3 globalAmbient = glm::vec3(0.0f); // global ambient color.
    float occludingRadius = 0.25f;
    SkyType sky = SkyType::Void;

    // variables computed in render()
    glm::vec4 cameraPos; // contains world-space position of camera
    glm::mat4 C;         // Camera Matrix
    glm::mat4 invP;      // inverse of projection mat
    float sample_scale;

    glm::vec3 getRayColor(const std::unique_ptr<Scene>& scene, const Ray& ray, 
                          const Interval& interval = Interval(EPSILION, MAX_DIST), 
                          uint recursiveDepth = 0);
    
    glm::vec3 getReflectionColor(const std::unique_ptr<Scene> &scene,
                                 const Ray &ray, const Hit &rec, 
                                 const Interval &interval, uint recursions);

    glm::vec3 getSkyColor(const Ray& ray);
    
    Ray castPrimaryRay(uint idx, uint idy, float offsetx = 0.5f, float offsety = 0.5f);

    float occlusionFactor(const Hit &rec, const std::unique_ptr<Scene> &scene,
                          const Interval &interval);

    float shadowFactor(const std::shared_ptr<Light>& light, const Hit &rec, 
                       const std::unique_ptr<Scene> &scene, const Interval &interval,
                       bool);
};