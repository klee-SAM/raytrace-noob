#include "Texture.hpp"

using namespace std;
using namespace glm;


// Default spatial texture
vec3 GradientTexture::value(float, float, const vec3& p) const {
    return normalize(vec3(
        0.5f*p.x+0.5f, 
        0.5f*p.y+0.5f, 
        0.5f*p.z+0.5f)
    );   
}



vec3 ImageTexture::value(float u, float v, const vec3& p) const {
    #ifndef NDEBUG
    if (!img->isLoaded()) return glm::vec3(1.0f, 0.0f, 1.0f);
    #endif 
    // Just realized that img->getPixel does most of the work haha
    glm::vec3 pixelColor;
    img->getPixel(u, v, pixelColor);
    return pixelColor;
}



// Default checker textures
const shared_ptr<Texture> CheckerTexture::white = make_shared<ColorTexture>(vec3(1.0f));
const shared_ptr<Texture> CheckerTexture::black = make_shared<ColorTexture>(vec3(0.0f));

vec3 CheckerTexture::value(float u, float v, const vec3& p) const {
    // Use floor to pull values to -INF
    int xInt = static_cast<int>(std::floor(p.x));
    int yInt = static_cast<int>(std::floor(p.y));
    int zInt = static_cast<int>(std::floor(p.z));
    // Raytracing the next week reference
    bool isEven = (xInt + yInt + zInt) % 2 == 0;
    return isEven ? even->value(u, v, p) : odd->value(u, v, p);
}