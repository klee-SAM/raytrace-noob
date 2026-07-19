#include "../Shape.hpp"

using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat3;
using glm::mat4;

using CONSTANTS::R_PI, CONSTANTS::EPSILION;

vec4 Cylinder::computeNormal(const vec3& x0) const {
	return vec4(x0.x, 0.0f, x0.z, 0.0f);
}

vec2 Cylinder::computeUV(const vec3& p) const {
	const float u = 0.5f + f_atan2(p.z, p.x)*R_PI*0.5f;
	const float &v = p.y;
	return vec2(u, v);
} 

void Cylinder::intersect(const Ray& ray, HitArray& hits) 
{
	mat4 modelMat = this->getModelMatrix(ray.time);
	mat4 inv_modelMat = inverse(modelMat);

	vec3 pk = vec3(inv_modelMat*ray.pos);
	vec3 vx = vec3(inv_modelMat*ray.dir);
	vec3 vk = normalize(vx);

	float a = vk.x*vk.x + vk.z*vk.z;
	// ray is parallel to y-axis
	if (a < EPSILION) return;

	float b = 2*(pk.x*vk.x + pk.z*vk.z);
	float c = pk.x*pk.x + pk.z*pk.z - 1;
	float d = b*b - 4.0f*a*c;
	if (d < EPSILION) return;
	float den = 1.0f/(2*a);
	
	float t0 = (-b - glm::sqrt(d))*den;
	float t1 = (-b + glm::sqrt(d))*den;

	vec3 x0 = pk + t0*vk;
	Hit h0 = toWorldSpaceHit(x0, vx, modelMat, inv_modelMat, t0);
	hits.push_back(h0);

	vec3 x1 = pk + t1*vk;
	Hit h1 = toWorldSpaceHit(x1, vx, modelMat, inv_modelMat, t1); 
	hits.push_back(h1);
}