#include "../Shape.hpp"

// Assume that the plane isn't rotating over time,
// so set uvec and vvec only "once" knowing that
// the normal is finalized; compute UV vectors
void Plane::initialize() { 
	// https://computergraphics.stackexchange.com/questions/8382
	vec3 normal = vec3(modelMat[1]);

    vec3 a = cross(normal, vec3(1, 0, 0));
    vec3 b = cross(normal, vec3(0, 1, 0));
    vec3 max_ab = dot(a, a) < dot(b, b) ? b : a;
    vec3 c = cross(normal, vec3(0, 0, 1));

    uvec = normalize(dot(max_ab, max_ab) < dot(c, c) ? c : max_ab);
    vvec = cross(normal, uvec);
}

// The rotation of the plane is used as the normal
void Plane::intersect(const Ray& ray, HitArray& hits) {
	// Compute the distance from the ray origin using:
	vec4 &n = modelMat[1];
	float num = dot(n, modelMat[3]-ray.pos);
	float den = dot(n, ray.dir);
	float t = num / den; 

	// and the position of intersection by
	vec3 offset = t*ray.dir;
	vec3 x = ray.getPos() + offset;

	// Compute u, v coordinates
	float u = dot(offset, uvec);
	float v = dot(offset, vvec);

	// Finally, store the above into a hits object. the normal is the 
	// normal of the hit surface, which is just the plane for this task.
    Hit hit; 
    hit.x = x; hit.n = n; hit.t = t; 
    hit.m = material.get();
    hit.uv.s = u; hit.uv.t = v;
	hits.push_back(hit);
}