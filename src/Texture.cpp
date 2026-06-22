#include "Texture.hpp"

// Default spatial texture
vec3 GradientTexture::value(const glm::vec2&, const vec3& p) const {
    return normalize(vec3(
        0.5f*p.x+0.5f, 
        0.5f*p.y+0.5f, 
        0.5f*p.z+0.5f)
    );   
}



vec3 ImageTexture::value(const glm::vec2 &uv, const vec3& p) const {
    #ifndef NDEBUG
    if (!img->isLoaded()) return glm::vec3(1.0f, 0.0f, 1.0f);
    #endif 
    // Just realized that img->getPixel does most of the work haha
    glm::vec3 pixelColor;
    img->getPixel(uv.s, uv.t, pixelColor);
    return pixelColor;
}



// Default checker textures
const std::shared_ptr<Texture> CheckerTexture::white = std::make_shared<ColorTexture>(vec3(1.0f));
const std::shared_ptr<Texture> CheckerTexture::black = std::make_shared<ColorTexture>(vec3(0.0f));

vec3 CheckerTexture::value(const glm::vec2 &uv, const vec3& p) const {
    // Use floor to pull values to -INF
    int xInt = static_cast<int>(std::floor(p.x));
    int yInt = static_cast<int>(std::floor(p.y));
    int zInt = static_cast<int>(std::floor(p.z));
    // Raytracing the next week reference
    bool isEven = (xInt + yInt + zInt) % 2 == 0;
    return isEven ? even->value(uv, p) : odd->value(uv, p);
}