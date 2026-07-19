#include "stn.hpp"

#include <glm/gtx/quaternion.hpp>

// #define BACKFACE_CULLING
#include "Shape.hpp"

using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat3;
using glm::mat4;
using glm::quat;

glm::mat4 Shape::getModelMatrix(float time) const {
	// Faster to branch than to always do modelMatLerp()
	return this->moving ? modelMatLerp(time) : modelMat;
}

void Shape::setNextModelTransforms(const glm::vec3& trns,
								   const glm::vec3& rot,
								   const glm::vec3& scl) 
{
	m_translation = trns;
	m_scale = scl;
	// Because of how rotations are applied in umath/ModelMatConstr,
	// use angleAxis() instead of the quat() constructor ZXY Euler angles
	m_rotation = glm::angleAxis(rot.z, vec3(0.f, 0.f, 1.f))*
				 glm::angleAxis(rot.x, vec3(1.f, 0.f, 0.f))* 
				 glm::angleAxis(rot.y, vec3(0.f, 1.f, 0.f));
	moving = true;
}

Hit Shape::toWorldSpaceHit(const vec3 &x, const vec3 &vx, 
						   const mat4 &model,
						   const mat4 &inv_model,
						   float t) const 
{
	const vec3 wld_x = vec3(model*vec4(x, 1.0f));
	// Use the inverse transpose to ensure that the normals
	// face the correct direction for nonuniform scales.
	const vec3 wld_n = normalize(vec3(transpose(inv_model)*computeNormal(x)));
	const float wld_t = t/length(vx);

	Hit h; 
	h.x = wld_x; 
	h.n = wld_n; 
	h.t = wld_t;
	h.m = material.get();
	h.uv = computeUV(x);

	return h;
}

constexpr vec3 lerp(float t, vec3 a, vec3 b) { return a + t*(b-a); }
// Use to "move" the object before doing any intersection tests.
// TODO: have bounding box of object encompass whole range of motion
mat4 Shape::modelMatLerp(const float time) const {
	using glm::quat;

	const vec3 translations = vec3(this->modelMat[3]);
	const vec3 scales = vec3(length(this->modelMat[0]),
					   		 length(this->modelMat[1]),
					   		 length(this->modelMat[2]));
	const mat3 rM = mat3(this->modelMat[0] / scales.x,
						 this->modelMat[1] / scales.y,
						 this->modelMat[2] / scales.z);
	const quat rotations = f_toQuat(rM);
	
	// Rotations.
	mat4 model = glm::mat4_cast(glm::fastMix(rotations, m_rotation, time));

	// Scale.
	const vec3 l_scales = lerp(time, scales, m_scale);
	model[0] *= l_scales.x;
	model[1] *= l_scales.y;
	model[2] *= l_scales.z;
	
	// Translation.
	model[3] = vec4(lerp(time, translations, m_translation), 1.f);

	return model;
}
