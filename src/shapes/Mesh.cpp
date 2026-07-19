#include "../Shape.hpp"

// Comment this out to show the actual model
// #define SHOW_BOUNDING_SPHERE

using CONSTANTS::EPSILION;

void Mesh::initialize() {
	// The MeshBuffer's bounding matrix is defined to go from unit space
	// to model space, so create a matrix that goes from
	// world -> model -> bounding
	this->inv_boundMat = meshDat->getInvBoundMat()*inverse(modelMat);
}

// This assumes that the position buffer size is a 
// multiple of 9. posBufOffset is the index of the
// x component of the 1st vertex.
// https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
bool Mesh::intersect_triangle(const vec3& orig, const vec3& dir, 
							  const size_t &i, /* posBufOffset */ 
							  float &t, float &u, float &v) 
{
	using namespace fastVecOp; // sin

	vec3 edge1, edge2, pvec, qvec, tvec;
	float det, inv_det;

	const auto& posBuf = meshDat->getPosBuf();
	const float *vt0 = &posBuf.at(0+i); 
	const float *vt1 = &posBuf.at(3+i); 
	const float *vt2 = &posBuf.at(6+i);

	edge1 = SUB(vt1, vt0);
	edge2 = SUB(vt2, vt0);

	pvec = CROSS(&dir.x, &edge2.x);

	det = DOT(&edge1.x, &pvec.x);

	// Handle the case where ray is not parallel to plane (det == 0) implicitly.
	#ifdef BACKFACE_CULLING
	// cull backfacing tris (det is negative for backfacing)
	if (det < EPSILION) return false;
	#else 
	// keep backfacing tris
	if (fabs(det) < EPSILION) return false; 
	#endif

	inv_det = 1.0f/det;

	tvec = SUB(&orig.x, vt0);
	u = DOT(&tvec.x, &pvec.x)*inv_det;
	// barycentric coord test
	if (u < 0.0f || u > 1.0f) return false;

	qvec = CROSS(&tvec.x, &edge1.x);
	v = DOT(&dir.x, &qvec.x)*inv_det;
	if (v < 0.0f || u + v > 1.0f) return false;

	t = DOT(&edge2.x, &qvec.x) * inv_det;
	
	return true;
}

#ifdef SHOW_BOUNDING_SPHERE
static std::shared_ptr<Material> debugBV = std::make_shared<Material>(
	glm::vec3(.5f), glm::vec3(1.f, 0.f, 1.f), glm::vec3(0.f), 0.f);
#endif

// Some floating-point error possible, or the normals are not normal
void Mesh::intersect(const Ray& ray, HitArray& hits) {
	// TODO: rework bounding shape function so that it's
	// compatible with motion blur, allowing me to let this be
	// affected by motion blur properties

	// transform ray to local coords
	mat4 inv_modelMat = inverse(modelMat);
	vec3 l_rorig = vec3(inv_modelMat*ray.pos);

	// bounding sphere test
	vec3 pk, vx, vk;
	// here, use inv_sphereMat to accurately represent the bounding sphere
	pk = vec3(inv_boundMat * ray.pos);
	vx = vec3(inv_boundMat * ray.dir);
	vk = normalize(vx);
	
	float a, b, c;
	float d;
	a = dot(vk, vk);
	b = 2*dot(vk, pk);
	c = dot(pk, pk) - 1;
	d = b*b - 4*a*c;
	
	// Return early if the ray does not intersect the bounding uniform sphere
	if (d <= 0.0f) return;

	#ifdef SHOW_BOUNDING_SPHERE
	float den = 1.0f/(2*a);
	float t0 = (-b - glm::sqrt(d))*den; 
	float t1 = (-b + glm::sqrt(d))*den;

	const mat4 sphereMat = inverse(inv_boundMat);

	vec3 x0 = pk + t0*vk;
	vec3 wld_x0 = vec3(sphereMat*vec4(x0, 1.0f));
	// sphereMat used instead of inv_T, since size is uniform
	vec3 wld_n0 = normalize(vec3(sphereMat*vec4(x0, 0.0f)));
	float wld_t0 = t0/length(vx);
	Hit h0;
	h0.x = wld_x0;
	h0.n = wld_n0;
	h0.t = wld_t0;
	h0.uv = vec2(0.f);
	h0.m = debugBV.get();
	hits.push_back(h0);

	vec3 x1 = pk + t1*vk;
	vec3 wld_x1 = vec3(sphereMat*vec4(x1, 1.0f));
	vec3 wld_n1 = normalize(vec3(sphereMat*vec4(x1, 0.0f)));
	float wld_t1 = t1/length(vx);
	Hit h1;
	h1.x = wld_x1;
	h1.n = wld_n1;
	h1.t = wld_t1;
	h1.uv = vec2(0.f);
	h1.m = debugBV.get();
	hits.push_back(h1);
	return;
	#endif

	vec3 r_vk = normalize(vec3(inv_modelMat*ray.dir));

	// u and v refer to barycentric coordinates.
	float t, u, v;

	mat4 invT_modelMat = transpose(inv_modelMat);
	for (size_t i = 0, j = 0; i < meshDat->getPosSize(); i += 9, j += 6) {
		if (!intersect_triangle(l_rorig, r_vk, i, t, u, v)) continue;
		float w = 1.0f - v - u;
		// Because of branch prediction, I have chosen not to check for t here;
		// that should be handled in hits, given a positive min value
		vec3 rv = l_rorig + t*r_vk;
		vec3 wld_x = vec3(modelMat*vec4(rv, 1.0f));

		vec3 wld_n; float tex_u = 0.0f, tex_v = 0.0f;

		// loadmesh ensures a vertex only has 1 normal associated with it,
		// if there are any attribute normals
		if (i < meshDat->getNorSize()) {
			const auto& norBuf = meshDat->getNorBuf();
			float nx = w*norBuf.at(0+i) + u*norBuf.at(3+i) + v*norBuf.at(6+i);
			float ny = w*norBuf.at(1+i) + u*norBuf.at(4+i) + v*norBuf.at(7+i);
			float nz = w*norBuf.at(2+i) + u*norBuf.at(5+i) + v*norBuf.at(8+i);

			// Because of floating-point error, the interpolated normal is no longer
			// normalized, so explicitly normalize it again.
			wld_n = normalize(vec3(invT_modelMat*vec4(nx, ny, nz, 0.0f)));
		}

		// Only two texture components per vertex, if there are any
		if (j < meshDat->getTexSize()) {
			const auto& texBuf = meshDat->getTexBuf();
			tex_u = w*texBuf.at(0+j) + u*texBuf.at(2+j) + v*texBuf.at(4+j);
			tex_v = w*texBuf.at(1+j) + u*texBuf.at(3+j) + v*texBuf.at(5+j);
		}

		Hit h; 
		h.x = wld_x; h.n = wld_n; h.t = t; 
		h.uv.s = tex_u; h.uv.t = tex_v;
		h.m = this->material.get();
		hits.push_back(h);
	}
}	