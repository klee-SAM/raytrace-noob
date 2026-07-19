#include "../Shape.hpp"

// https://en.wikipedia.org/wiki/UV_mapping#Finding_UV_on_a_sphere
vec2 Sphere::computeUV(const vec3& p) const {
    return sphereMap(p);
}

vec4 Sphere::computeNormal(const glm::vec3& x) const { 
	return vec4(x, 0.0f); 
};

void Sphere::intersect(const Ray& ray, HitArray& hits) {
	mat4 modelMat = this->getModelMatrix(ray.time);
	mat4 inv_modelMat = inverse(modelMat);

	vec3 pk = inv_modelMat*ray.pos;
	vec3 vx = inv_modelMat*ray.dir;
	vec3 vk = normalize(vx);
	float a, b, c;
	float d, den;

	a = dot(vk, vk);
	b = 2*dot(vk, pk);
	c = dot(pk, pk) - 1.0f;
	d = b*b - 4.0f*a*c;
	den = 1.0f/(2.0f*a);

	if (d > 0.0f) {
		float t0 = (-b - glm::sqrt(d))*den; 
		float t1 = (-b + glm::sqrt(d))*den;

		vec3 x0 = pk + t0*vk;
        Hit h0 = toWorldSpaceHit(x0, vx, modelMat, inv_modelMat, t0);
		hits.push_back(h0);

		vec3 x1 = pk + t1*vk;
		Hit h1 = toWorldSpaceHit(x1, vx, modelMat, inv_modelMat, t1);
		hits.push_back(h1);
	}
}