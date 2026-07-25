#pragma once
#ifndef RAYTRACER_H
#define RAYTRACER_H

#include "../stn.hpp"
#include "../util/umath.hpp"
#include "../util/RowQueue.hpp"
#include "../util/MatrixStack.hpp"
#include "../util/prand.hpp"

#include "../Ray.hpp"
#include "../Texture.hpp"

class Scene;
class Light;
class Camera;

class TCamera {
public:
    using radian_t = double;
    using degree_t = double;
    using sample_t = unsigned int;

    static constexpr sample_t MAX_LIGHT_SAMPLES = 256U;
    static constexpr sample_t DEFAULT_LIGHT_SAMPLES = 32U;

    // Relative translation, which is indirectly used in computing cameraPos
    glm::vec3 translation;
    // Rotate the scene around the origin by angles specified in each axis in radians.
    glm::vec3 rotation; 

    uint width, height;    // dimensions of the output image

    double aspectRatio;    // Ratio of width to height
    radian_t fovy;         // vertical field of view
    double znear = 1.0;    // dist of near plane from camera for clipping    
    double zfar = 10000.0; // dist of far plane from camera for clipping

    float focusLength = 5.f;    // dist where everything is in focus
    float focalRadius = 0.f;    // > 0.f for DoF effect

    sample_t samplesPerPixel = 1U;  // samples per pixel; should be at least 1
    sample_t occlusionSamples = 0U; // ambient occlusion samples per ray
    float occludingRadius = 0.25f;  // radius of occluding hemisphere
    sample_t lightSamples = 1U;     // samples to take per shadow ray of area lights

    // Ambient occlusion samples are taken for every color ray, including
    // anti-aliasing rays; recommended to reduce AO samples if increasing AA rays
    TCamera() 
    : translation(0.f), rotation(0.f),
      width(1), height(1),
      aspectRatio(1.0), 
      fovy(glm::radians(45.0)),
      position(0.f), 
      lookAtPos{0.f, 0.f, -1.f}, 
      camUpVec{0.f, 1.f, 0.f} { }

    TCamera(uint w, uint h) 
    : translation(0.f), rotation(0.f),
      width(w), height(h),
      aspectRatio((double)w / (double)h), 
      fovy(glm::radians(45.0)),
      position(0.f), 
      lookAtPos{0.f, 0.f, -1.f}, 
      camUpVec{0.f, 1.f, 0.f} { }

    TCamera(uint w, uint h, degree_t fov)
    : translation(0.f), rotation(0.f),
      width(w), height(h),
      aspectRatio((double)w / (double)h), 
      fovy(glm::radians(fov)),
      position(0.f), 
      lookAtPos{0.f, 0.f, -1.f}, 
      camUpVec{0.f, 1.f, 0.f} { }

    virtual ~TCamera() = default;

    // Provided for compatibility
    void setInitDistance(double dist) { translation.z = -std::abs(dist); }

    // Clamps/limits the provided sample count given, unlike direct member access
    void setLightSamples(sample_t count) { 
      lightSamples = std::clamp(count, 1U, MAX_LIGHT_SAMPLES); 
    }

    // Setter for setting basis of view matrix, with some NaN prevention 
    void setCameraPos(const glm::vec3 &pos) {
        using namespace CONSTANTS; 
        if (glm::length(pos - lookAtPos) < EPSILION) return;
        else position = pos; 
    }
    // Setter for setting basis of view matrix, with some NaN prevention 
    void setLookAtPos(const glm::vec3 &lookpos) { 
        using namespace CONSTANTS; 
        if (glm::length(lookpos - position) < EPSILION) return;
        else lookAtPos = lookpos; 
    }
    // As a failsafe, make the up vector face in the +y axis
    // if the magnitude of upVec is 0.
    void setUpVector(const glm::vec3 &upVec) { 
        using namespace CONSTANTS; 
        if (glm::length(upVec) > EPSILION) camUpVec = glm::normalize(upVec);
        else camUpVec = glm::normalize(upVec+glm::vec3(0.f, EPSILION, 0.f));
    }

    // Position of the camera in world-space
    glm::vec3 getCameraPos() const { return position; }
    // Position the camera looks at in world-space  
    glm::vec3 getLookAtPos() const { return lookAtPos; }
    // Up vector of the camera. 
    glm::vec3 getUpDir() const { return camUpVec; }

private:
    glm::vec3 position;    
    glm::vec3 lookAtPos;       
    glm::vec3 camUpVec;    
};

// Common functionality for all raytracers.

class Raytracer {
public:
    // Contains common info used for BRDF calculations
    struct IntParams {
        const Interval &interval;
        const Hit &rec;
    };

    static prand::diskRand diskRandGen;
    static prand::uniformRand unifRandGen;

    const float EPSILION = 5E-4f; // 5E-3f was prev val
    const float MAX_DIST = CONSTANTS::MAX;
    const uint MAX_RECURSIONS = 7;
    const float MINIMUM_COEFF = 0.005f;

    bool FULL_SHADOWS = false;
    bool SHOW_NORMALS = false;

    void bindScene(std::unique_ptr<Scene>&& scene);
    void bindCamera(std::unique_ptr<TCamera>&& cam);

    std::unique_ptr<Image> render();
    glm::vec3 getRayColor(const Ray&) const; // override this

protected:
    // Guarantee that fields in these objects cannot be changed
    const std::unique_ptr<Scene>& getScene() const { return scene; }
    const std::unique_ptr<TCamera>& getCamera() const { return camera; }

private:
    std::unique_ptr<Scene> scene;
    std::unique_ptr<TCamera> camera;

    RowQueue r_queue; // Multithreading by row slices
    void processRows(std::unique_ptr<Image> &image);
    void setRow(std::unique_ptr<Image> &image, uint y);

    // variables computed in render()
    glm::vec4 cameraPos; // contains world-space position of camera
    glm::mat4 C;         // Camera Matrix, inverse of View Matrix
    glm::mat4 invP;      // inverse of projection mat
    glm::vec4 dof_u;     // right cam basis vec
    glm::vec4 dof_v;     // up cam basis vec

    float f_width, f_height;

    void applyProjection(MatrixStack&);
    void applyView(MatrixStack&);

    struct Pixel { uint x, y; };
    Ray castPrimaryRay(Pixel id, const glm::vec2 &offset) const;
    Ray castSecondaryRay(const Ray &primaryRay) const;
};

#endif