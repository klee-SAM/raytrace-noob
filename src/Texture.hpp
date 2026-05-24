#pragma once
#include "stn.hpp"

#include "Image.hpp"

// what is this
class Texture {
public:
    Texture() : color(0.0f), img(blankImage) {};

    virtual ~Texture() = default;

    // Initialize with a flat color.
    // Provided b/c materials are constructed with ColorTextures by default
    void init(const glm::vec3 &clr) { this->color = clr; };

    virtual glm::vec3 value(float u, float v, const glm::vec3& p) const = 0;

protected:
    // ????
    Texture(const glm::vec3& clr) : color(clr), img(blankImage) { }

    // Load an image file and assigns it to this texture
    Texture(const std::string &str) : color(0.0f) { img = std::make_shared<Image>(str); }

    // Assigns an image to this texture
    Texture(const std::shared_ptr<Image> &img) : color(0.0f) { this->img = img; } 

    glm::vec3 color; // for flat colors or procedural textures

    // multiple distinct materials could point to the same
    // image data, so shared ptr it is
    std::shared_ptr<Image> img;
};

class ColorTexture : public Texture {
public:
    ColorTexture() {};
    ColorTexture(const glm::vec3& clr) : Texture(clr) {}
    virtual ~ColorTexture() = default;

    glm::vec3 value(float, float, const glm::vec3&) const override { return color; }
};

class GradientTexture : public Texture {
public:
    GradientTexture() {};
    virtual ~GradientTexture() = default;

    // perhaps a function to generate gradient here

    glm::vec3 value(float u, float v, const glm::vec3& p) const override;
};

class ImageTexture : public Texture {
public: 
    ImageTexture() {};
    ImageTexture(const std::string &str) : Texture(str) {}
    ImageTexture(const std::shared_ptr<Image> &img) : Texture(img) {} 
    virtual ~ImageTexture() = default;

    glm::vec3 value(float u, float v, const glm::vec3&) const override;
};


// Spatial Texture
class CheckerTexture : public Texture {
public:
    static const std::shared_ptr<Texture> white;
    static const std::shared_ptr<Texture> black;

    CheckerTexture() : even(white), odd(black) {};

    CheckerTexture(const glm::vec3& e, const glm::vec3& o) 
                   : even(std::make_shared<ColorTexture>(e)),
                     odd(std::make_shared<ColorTexture>(o)) {}

    CheckerTexture(const std::shared_ptr<Texture>& e, 
                   const std::shared_ptr<Texture>& o) 
                   : even(e), odd(o) {};

    virtual ~CheckerTexture() = default;

    void setEven(const std::shared_ptr<Texture>& e) { even = e; }
    void setOdd(const std::shared_ptr<Texture>& o) { odd = o; }

    glm::vec3 value(float u, float v, const glm::vec3& p) const override;

private:
    std::shared_ptr<Texture> even;
    std::shared_ptr<Texture> odd;
};