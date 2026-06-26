#pragma once
#include "stn.hpp"
#include "Texture.hpp"

class Material {
public:
    std::shared_ptr<Texture> ambient;
    std::shared_ptr<Texture> diffuse;
    std::shared_ptr<Texture> specular;
    std::shared_ptr<Texture> emissive;
    float exponent;

    float reflCoeff;    // 1.0 is full reflection; 0.0 is no reflection
    float refrIndex;    // RI of material going from inside the material to a vacuum
    float transparency; // 1.0 is full refraction; 0.0 is no refraction
    float fresnelCoeff; // 1.0 is full fresnel refl; yk what 0.0 is

    uint reflSamples; // 1 ray is smooth reflection, 2+ for sampling rough reflections 
    float fuzz;       // reflection roughness; 0.0 indicates no roughness

    std::shared_ptr<Texture> texture;
    // float textureOpacity; // may or may not be implemented

    Material() : ambient{std::make_shared<ColorTexture>(glm::vec3(0.0f))}, 
                 diffuse{std::make_shared<ColorTexture>(glm::vec3(0.0f))}, 
                 specular{std::make_shared<ColorTexture>(glm::vec3(0.0f))}, 
                 emissive{std::make_shared<ColorTexture>(glm::vec3(0.0f))},
                 exponent{1.0f}, 
                 reflCoeff{0.0f}, 
                 refrIndex{1.0f}, 
                 transparency{0.0f},
                 fresnelCoeff{1.0f},
                 reflSamples{1U},
                 fuzz{0.05f}
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