#include "Texture.hpp"
#include "util/umath.hpp"

// Default spatial texture
vec3 GradientTexture::value(const glm::vec2&, const vec3& p) const {
    return normalize(vec3(
        0.5f*p.x+0.5f, 
        0.5f*p.y+0.5f, 
        0.5f*p.z+0.5f)
    );   
}



vec3 ImageTexture::value(const glm::vec2 &uv, const vec3& p) const {
    if (!img->isLoaded()) return glm::vec3(1.0f, 0.0f, 1.0f);
    // Just realized that img->getPixel does most of the work haha
    glm::vec3 pixelColor;
    img->getPixel(uv.s, uv.t, pixelColor);
    // idea for tinting images: object in json file specifying the 
    // file, procedural (or flat) color, and alpha value to blend
    return glm::mix(pixelColor, color, alpha);
}



// Default checker textures
const std::shared_ptr<Texture> CheckerTexture::white = std::make_shared<ColorTexture>(vec3(1.0f));
const std::shared_ptr<Texture> CheckerTexture::black = std::make_shared<ColorTexture>(vec3(0.0f));

vec3 CheckerTexture::value(const glm::vec2 &uv, const vec3& p) const {
    // add epsilion for better numerical stability, which is possibly
    // because of values very close to nearest integer in bad spots
    vec3 pInt = glm::floor(p + CONSTANTS::EPSILION);
    // Raytracing the next week reference
    bool isEven = static_cast<int>(pInt.x + pInt.y + pInt.z) % 2 == 0;
    return isEven ? even->value(uv, p) : odd->value(uv, p);
}