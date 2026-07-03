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

    virtual glm::vec3 value(const glm::vec2 &uv, const glm::vec3 &p) const = 0;

protected:
    // ????
    Texture(const glm::vec3& clr) : color(clr), img(blankImage) { }

    // Load an image file and assigns it to this texture
    Texture(const std::string &str) : color(0.0f) { img = std::make_shared<Image>(str); }

    // Assigns an image to this texture
    Texture(const std::shared_ptr<Image> &img) : color(0.0f) { this->img = img; } 

    glm::vec3 color; // for flat colors or procedural textures
    // glm::mat3 textureMat; // Texture transformation matrix

    // multiple distinct materials could point to the same
    // image data, so shared ptr it is
    std::shared_ptr<Image> img;
};

class ColorTexture : public Texture {
public:
    ColorTexture() {};
    ColorTexture(const glm::vec3& clr) : Texture(clr) {}
    virtual ~ColorTexture() = default;

    glm::vec3 value(const glm::vec2& = glm::vec2(0.f), 
                    const glm::vec3& = glm::vec3(0.f)) 
                    const override { return color; }
};

class GradientTexture : public Texture {
public:
    GradientTexture() {};
    virtual ~GradientTexture() = default;

    inline glm::vec3 transform(const glm::vec3&) const;

    glm::vec3 value(const glm::vec2&, const glm::vec3& p) const override;
};

class ImageTexture : public Texture {
public: 
    ImageTexture() : alpha{1.f} {};
    ImageTexture(const std::string &str) : Texture(str), alpha{0.f} {}
    ImageTexture(const std::shared_ptr<Image> &img) : Texture(img), alpha{0.f} {} 
    virtual ~ImageTexture() = default;

    void setAlpha(float x) { alpha = std::clamp(x, 0.f, 1.f); }

    glm::vec3 value(const glm::vec2& uv, 
                    const glm::vec3& = glm::vec3(0.f)) const override;
private:
    float alpha;
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

    glm::vec3 value(const glm::vec2& uv, const glm::vec3& p) const override;

private:
    std::shared_ptr<Texture> even;
    std::shared_ptr<Texture> odd;
};