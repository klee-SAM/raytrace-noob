#pragma once
#include "stn.hpp"
#include "Material.hpp"

struct Ray {
    glm::vec3 pos;
    glm::vec3 dir;
    glm::vec3 clr; // Stores the color from a previous computation
};

struct Hit {
    glm::vec3 x; // hit location
    glm::vec3 n; // hit normal
    float t; // dist from origin to hit
    float u; // texture u coord
    float v; // texture v coord
    std::shared_ptr<Material> m;
};