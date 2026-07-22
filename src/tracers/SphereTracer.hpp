#pragma once
#ifndef TRACER_SPHERE_H
#define TRACER_SPHERE_H

#include "../stn.hpp"
#include "../Image.hpp"
#include "../Scene.hpp"
#include "../util/RowQueue.hpp"


class SphereTracer {
public:

    std::unique_ptr<Image> render(std::unique_ptr<Scene>&, const glm::mat4& P, const glm::mat4& V);
    void processRows(const std::unique_ptr<Scene> &scene, std::unique_ptr<Image> &image);
    void setRow(const std::unique_ptr<Scene> &scene, std::unique_ptr<Image> &image, uint y);

private:
    uint width, height;

    // rippp
    RowQueue r_queue;

    // variables computed in render()
    glm::vec4 cameraPos; // contains world-space position of camera
    glm::mat4 C;         // Camera Matrix, inverse of View Matrix
    glm::mat4 invP;      // inverse of projection mat
    glm::vec4 dof_u;     // right cam basis vec
    glm::vec4 dof_v;     // up cam basis vec
};

#endif