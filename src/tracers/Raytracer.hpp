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

// class Raytracer {
// public:
//     typedef const glm::mat4 CMatrix;

//     // having a lower epsilion value does not bode well with meshes
//     // TODO: test if this is still the case
//     const float EPSILION = 5E-4f; // 5E-3f was prev val
//     const float MAX_DIST = CONSTANTS::MAX;
//     const uint MAX_RECURSIONS = 7;
//     const float MINIMUM_COEFF = 0.005f;

//     Raytracer() {};
//     virtual ~Raytracer() = default;

//     std::unique_ptr<Image> render(std::unique_ptr<Scene>&, CMatrix &P, CMatrix &MV) const;
//     void processRows(const std::unique_ptr<Scene> &scene, std::unique_ptr<Image> &image);
//     virtual void setRow(const std::unique_ptr<Scene> &scene, std::unique_ptr<Image> &image, uint y) = 0;

//     // Might move all these methods to private, and just have getRayColor be the function that
//     // actually needs to be overridden. 
//     Ray castPrimaryRay(uint idx, uint idy, const glm::vec2 &offset = glm::vec2(.5f)) const;
//     Ray castSecondaryRay(const Ray &primaryRay) const;
    
// protected:
//     RowQueue r_queue;

//     // variables computed in render()
//     glm::vec4 cameraPos; // contains world-space position of camera
//     glm::mat4 C;         // Camera Matrix, inverse of View Matrix
//     glm::mat4 invP;      // inverse of projection mat
//     glm::vec4 dof_u;     // right cam basis vec
//     glm::vec4 dof_v;     // up cam basis vec

//     // Contains common info used for BRDF calculations
//     struct IntParams {
//         const std::unique_ptr<Scene> &scene;
//         const Interval &interval;
//         const Hit &rec;
//     };  

//     // ...
// };

class Raytracer {
public:
    const float EPSILION = 5E-4f; // 5E-3f was prev val
    const float MAX_DIST = CONSTANTS::MAX;
    const uint MAX_RECURSIONS = 7;
    const float MINIMUM_COEFF = 0.005f;

    void bindScene(std::unique_ptr<Scene>&& scene);
    // void bindCamera(std::unique_ptr<Camera>&& cam);

    std::unique_ptr<Image> render() const;
    // glm::vec3 getRayColor(...) const;

    // Contains common info used for BRDF calculations
    struct IntParams {
        const Interval &interval;
        const Hit &rec;
    };

private:
    std::unique_ptr<Scene> scene;
    // std::unique_ptr<Camera> camera;

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

    struct Pixel { uint x, y; };

    Ray castPrimaryRay(Pixel id, const glm::vec2 &offset) const;
    Ray castSecondaryRay(const Ray &primaryRay) const;
};

#endif