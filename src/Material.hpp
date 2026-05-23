#pragma once
#include "stn.hpp"
#include "Texture.hpp"

class Material {
public:
    std::shared_ptr<Texture> ambient;
    std::shared_ptr<Texture> diffuse;
    std::shared_ptr<Texture> specular;
    float exponent;

    float reflCoeff;
    float refrIndex;
    float transparency;

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

    Material() : ambient{std::make_shared<ColorTexture>(glm::vec3(0.0f))}, 
                 diffuse{std::make_shared<ColorTexture>(glm::vec3(0.0f))}, 
                 specular{std::make_shared<ColorTexture>(glm::vec3(0.0f))}, 
                 exponent{1.0f}, 
                 reflCoeff{0.0f}, 
                 refrIndex{1.0f}, 
                 transparency{0.0f}
                 {};
                 
    Material(glm::vec3 amb, glm::vec3 dif, glm::vec3 spe, float exp)
    : ambient{std::make_shared<ColorTexture>(amb)}, 
      diffuse{std::make_shared<ColorTexture>(dif)}, 
      specular{std::make_shared<ColorTexture>(spe)}, 
      exponent(exp) {}

    virtual ~Material() = default;

    void copy(Material& oth) { *this = oth; }

};

static const std::shared_ptr<Material> defaultMaterial = std::make_shared<Material>();