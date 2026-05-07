#pragma once
#include "stn.h"
#include "Texture.h"

class Material {
public:
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float exponent;

    float reflCoeff;

    std::shared_ptr<Texture> texture;
    float textureOpacity;
    // later concern is figuring out how to
    // implement textures without having to
    // do branching; only possible if treating
    // the flat ambient and diffuse colors
    // as textures, but on the cpu
    // branch prediction is okay if 
    // we just have a simple boolean check
    // (just check if texture loaded)

    Material();
    virtual ~Material();

    glm::vec3 getLightContrib(
        const glm::vec3& eye, 
        const glm::vec3& normal, 
        const glm::vec3& lv, 
        float u, float v);

};