#include "Texture.hpp"
#include "util/umath.hpp"

// Default spatial texture
constexpr vec3 GradientTexture::transform(const vec3 &x) const {
    return .5f*glm::normalize(x)+.5f;
};

vec3 GradientTexture::value(const glm::vec2&, const vec3& p) const {
    return glm::clamp(vec3(transform(p)), 0.f, 1.f);
}



vec3 ImageTexture::value(const glm::vec2 &uv, const vec3& p) const {
    if (!img->isLoaded()) return glm::vec3(1.0f, 0.0f, 1.0f);
    // Just realized that img->getPixel does most of the work haha
    glm::vec3 pixelColor;
    img->getPixel(uv.s, uv.t, pixelColor);
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