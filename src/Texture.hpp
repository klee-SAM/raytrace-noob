#pragma once
#include "stn.hpp"

#include "Image.hpp"

class Texture {
public:
    Texture() {};
    virtual ~Texture() = default;

    // Load an image file
    void load(const std::string&);

    // Initialize with a flat color
    void load(const glm::vec3&);

    virtual glm::vec3 value(float u, float v, const glm::vec3& p) const = 0;

protected:
    glm::vec3 color; // for flat colors or procedural textures

    // multiple distinct materials could point to the same
    // image data, so shared ptr it is
    std::shared_ptr<Image> img;
};

class ColorTexture : public Texture {
public:
    ColorTexture() {};
    virtual ~ColorTexture() = default;

    glm::vec3 value(float, float, const glm::vec3&) const override { 
        return color; 
    }
};

class SpatialTexture : public Texture {
public:
    SpatialTexture() {};
    virtual ~SpatialTexture() = default;

    glm::vec3 value(float, float, const glm::vec3& p) const override {
        return glm::vec3(0.0f);
    }
};

class ImageTexture : public Texture {
public: 
    ImageTexture() {};
    virtual ~ImageTexture() = default;

    glm::vec3 value(float u, float v, const glm::vec3&) const override {
        return glm::vec3(0.0f);
    }
};