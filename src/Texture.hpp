#pragma once
#include "stn.hpp"

#include "Image.hpp"

// Interface-ish
class Texture {
public:
    Texture() : color(0.0f), img(blankImage) {};
    Texture(const std::string &str) : Texture() { init(str); }
    Texture(const std::shared_ptr<Image> &img) : Texture() { init(img); } 
    Texture(const glm::vec3& clr) : Texture() { init(clr); }

    virtual ~Texture() = default;

    // Load an image file and assigns it to this texture
    void init(const std::string &str) { img = std::make_shared<Image>(str); };

    // Assigns an image to this texture
    void init(const std::shared_ptr<Image> &img) { this->img = img; };

    // Initialize with a flat color
    void init(const glm::vec3 &clr) {this->color = clr; };

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

    glm::vec3 value(float, float, const glm::vec3&) const override { return color; }
};

class SpatialTexture : public Texture {
public:
    SpatialTexture() {};
    virtual ~SpatialTexture() = default;

    glm::vec3 value(float u, float v, const glm::vec3& p) const override {
        // TODO: Checkers?
        return glm::vec3(0.0f);
    }
};

class ImageTexture : public Texture {
public: 
    ImageTexture() {};
    virtual ~ImageTexture() = default;

    glm::vec3 value(float u, float v, const glm::vec3&) const override {
        #ifndef NDEBUG
        if (!img->isLoaded()) return glm::vec3(1.0f, 0.0f, 1.0f);
        #endif 
        // Just realized that img->getPixel does most of the work haha
        glm::vec3 pixelColor;
        img->getPixel(u, v, pixelColor);
        return pixelColor;
    }
};