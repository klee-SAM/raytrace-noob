#pragma once
#ifndef RAYTRACER_H
#define RAYTRACER_H

#include "../stn.hpp"
#include "../util/umath.hpp"
#include "../util/RowQueue.hpp"
#include "../Ray.hpp"
#include "../Texture.hpp"

class Scene;
class Light;

// Common functionality for all raytracers.

class Raytracer {
public:
    typedef const glm::mat4 CMatrix;

    // having a lower epsilion value does not bode well with meshes
    // TODO: test if this is still the case
    const float EPSILION = 5E-4f; // 5E-3f was prev val
    const float MAX_DIST = CONSTANTS::MAX;
    const uint MAX_RECURSIONS = 7;
    const float MINIMUM_COEFF = 0.005f;

    Raytracer() {};
    virtual ~Raytracer() = default;

    // todo:
    // perhaps this takes a struct as an argument which contains the 
    // P and MV matrices, scene, and camera...
    // or perhaps the camera holds the P and MV matrices
    std::unique_ptr<Image> render(std::unique_ptr<Scene>&, CMatrix &P, CMatrix &MV) const;
    void processRows(const std::unique_ptr<Scene> &scene, std::unique_ptr<Image> &image);
    virtual void setRow(const std::unique_ptr<Scene> &scene, std::unique_ptr<Image> &image, uint y) = 0;

    // TODO: for cast secondary ray, need to copy some members of camera
    // as the raytracer's members so that castSecondary ray can be used;
    // want to minimize the overhead for this 

    // Might move all these methods to private, and just have getRayColor be the function that
    // actually needs to be overridden. 
    Ray castPrimaryRay(uint idx, uint idy, const glm::vec2 &offset = glm::vec2(.5f)) const;
    Ray castSecondaryRay(const Ray &primaryRay) const;

    // what was deleted here should be in camera, actually
    // (following raytracing in one weekend example)

    // the sky should belong in Scene.hpp,
    // camera settings should become public
    // make variables computed in render() as
    // protected variables here, because I
    // am not going to bother defining a whole
    // set of getters and setters for them

protected:
    RowQueue r_queue;

    // variables computed in render()
    glm::vec4 cameraPos; // contains world-space position of camera
    glm::mat4 C;         // Camera Matrix, inverse of View Matrix
    glm::mat4 invP;      // inverse of projection mat
    glm::vec4 dof_u;     // right cam basis vec
    glm::vec4 dof_v;     // up cam basis vec
    float sample_scale;

    // Contains common info used for BRDF calculations
    struct IntParams {
        const std::unique_ptr<Scene> &scene;
        const Interval &interval;
        const Hit &rec;
    };  

    // ...
};

#endif