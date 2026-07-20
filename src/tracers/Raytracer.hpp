#pragma once
#ifndef RAYTRACER_H
#define RAYTRACER_H

#include "../stn.hpp"
#include "Texture.hpp"

// Common functionality for all raytracers.

class Raytracer {
public:

enum class SkyType {Void, Haze, SphereMap, Ambient};

// todo:

private:
    uint AAsamples = 1;                        // must be at least 1
    uint occlusionSamples = 0;                 // actual count is divided by AA samples
    float occludingRadius = 0.25f;             // radius of occluding hemisphere
    glm::vec3 globalAmbient = glm::vec3(0.0f); // ambient color added to all objects.
    SkyType sky = SkyType::Ambient;
    std::unique_ptr<ImageTexture> skyTexture;
};

#endif