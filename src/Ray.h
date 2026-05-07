#pragma once
#include "stn.h"
#include "Material.h"

struct Ray {
    glm::vec3 pos;
    glm::vec3 dir;
    glm::vec3 clr; // Stores the color from a previous computation
};

struct Hit {
    float x; // hit location
    float n; // hit normal
    float t; // dist from origin to hit
    float u; // texture coords
    float v; // above
    std::shared_ptr<Material> m;
};