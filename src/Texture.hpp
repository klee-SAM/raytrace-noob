#pragma once
#include "stn.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

class Texture {
public:
    // glm::mat3 T; // Texture transformation matrix, should be per shape

    Texture();
    virtual ~Texture();

    void loadImage();
    void loadColor(const glm::vec3&);

    glm::vec3 getColor(float u, float v);

private:
    glm::vec3 color; // temporary until i can load textures 
    std::vector<char> textureData;
};