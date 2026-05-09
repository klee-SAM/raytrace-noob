#include "Material.hpp"

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