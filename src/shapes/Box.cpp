#include "../Shape.hpp"

using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat3;
using glm::mat4;

using CONSTANTS::INF, CONSTANTS::EPSILION;

vec4 Box::computeNormal(const vec3& p) const {
	// epsilion just high enough to avoid precision problems
	constexpr float bias = 1.0f + 100*EPSILION; 
	float cx = static_cast<int>(p.x*bias);
	float cy = static_cast<int>(p.y*bias);
	float cz = static_cast<int>(p.z*bias);
	// only hits at corners are not normal, which is rare
	return vec4(cx, cy, cz, 0.0f);
}

vec2 Box::computeUV(const glm::vec3& p) const {
	// For any hit on the box, at least one component has
	// a value of 1, indicating the face that is hit. 
	// The information of the other 2 components are used 
	// to get the UV. this is temporary
	float maxc = std::max(std::max(abs(p.x), abs(p.y)), abs(p.z));
	float u, v;
	// flip on p.z for un-mirroring 
	if (abs(p.x) == maxc) 	   u = -p.z, v = p.y;
	else if (abs(p.y) == maxc) u =  p.x, v = p.z;
	else 				  	   u =  p.x, v = p.y;
	return vec2(0.5f*u-0.5f, 0.5f*v-0.5f);
};

// Axis-aligned bounding box intersection
void Box::intersect(const Ray& ray, HitArray& hits) {
	mat4 modelMat = this->getModelMatrix(ray.time);
	mat4 inv_modelMat = inverse(modelMat);
	// Transform the ray back to model space first;
	// axis tests assume the box is at the origin
	vec3 pk = vec3(inv_modelMat*ray.pos);
	vec3 vx = vec3(inv_modelMat*ray.dir);
	vec3 vk = normalize(vx);

	// https://tavianator.com/2022/ray_box_boundary.html
	// incrementally update the tmin and tmax values
	// based on the prev/initial values, accounting for 
	// NANs and INFs implicitly

	float tmin = 0.0f, tmax = INF;
	vec3 inv_dir = vec3(1.0f / vk.x, 1.0f / vk.y, 1.0f / vk.z);
	for (int i = 0; i < 3; ++i) {
		float t0 = (-1.0f - (&pk.x)[i]) * (&inv_dir.x)[i]; // evil syntax
		float t1 = (1.0f - (&pk.x)[i]) * (&inv_dir.x)[i];
		tmin = glm::min(glm::max(t0, tmin), glm::max(t1, tmin));
		tmax = glm::max(glm::min(t0, tmax), glm::min(t1, tmax));
	}

	if (tmax < tmin) return;

	vec3 x0 = pk + tmin*vk;
	Hit h0 = toWorldSpaceHit(x0, vx, modelMat, inv_modelMat, tmin);
	hits.push_back(h0);

	vec3 x1 = pk + tmax*vk;
	Hit h1 = toWorldSpaceHit(x1, vx, modelMat, inv_modelMat, tmax);
	hits.push_back(h1);
}