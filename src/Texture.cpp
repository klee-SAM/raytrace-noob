#include "Texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Texture::Texture() {}
Texture::~Texture() {}

void Texture::loadImage() {

}

void Texture::loadColor(const glm::vec3&) {

}

glm::vec3 Texture::getColor(float u, float v) {
    return glm::vec3(0.0f);
}