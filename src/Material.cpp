#include "Material.h"

using namespace std;
using namespace glm;

Material::Material() :
    ambient{vec3(0.0f)},
    diffuse{vec3(0.0f)},
    specular{vec3(0.0f)}, 
    exponent{1.0f},
    reflCoeff{0.0f} 
{
}

Material::~Material() 
{
}


// All vectors should be in camera-space.
vec3 Material::getLightContrib(const vec3& e, const vec3& n, const vec3& l, float u, float v) {
}