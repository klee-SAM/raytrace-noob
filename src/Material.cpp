#include "Material.h"

using namespace std;
using namespace glm;

Material::Material() :
ambient{vec3(0.0f)},
diffuse{vec3(0.0f)},
specular{vec3(0.0f)}, 
exponent{1.0f},
reflCoeff{0.0f} {
}

Material::~Material() {
}

// All vectors should be in camera-space.
vec3 Material::getLightContrib(
const vec3& e, 
const vec3& n, 
const vec3& l) {
    auto& ka = ambient, kd = diffuse, ks = specular;
    auto& s = exponent;
    vec3 h = normalize(l + e);
    return ka + kd*std::max(0.0f, dot(n, l)) + ks*std::pow(std::max(0.0f, dot(n, h)), s);
}