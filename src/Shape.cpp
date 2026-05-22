#include "stn.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

// #define BACKFACE_CULLING
#include "Shape.hpp"

using namespace std;
using namespace glm;
using namespace CONSTANTS;

void Shape::setModelMatrix(const mat4& m) {
	modelMat = m;
	inv_modelMat = inverse(m);
	invT_modelMat = inverse(transpose(m));
}

Hit Shape::toWorldSpaceHit(const vec3& x, const vec3& vx, float t) const {
	vec3 wld_x = vec3(modelMat*vec4(x, 1.0f));
	vec3 wld_n = normalize(vec3(invT_modelMat*computeNormal(x)));
	float wld_t = t/length(vx);

	Hit h; 
	h.x = wld_x; 
	h.n = wld_n; 
	h.t = wld_t;
	h.m = material;
	vec2 uv = computeUV(x);
	h.u = uv.x;
	h.v = uv.y;

	return h;
}

// https://en.wikipedia.org/wiki/UV_mapping#Finding_UV_on_a_sphere
vec2 Sphere::computeUV(const vec3& p) const {
    float u = (1.0 + std::atan2(p.z, p.x)*R_PI)/2;
    float v = 0.5 + std::asin(p.y)*R_PI;
    return vec2(u, v);
}

vec4 Sphere::computeNormal(const glm::vec3& x) const { 
	return vec4(x, 0.0f); 
};

void Sphere::intersect(const Ray& ray, vector<Hit>& hits) {
	vec3 pk = vec3(inv_modelMat*ray.pos);
	vec3 vx = vec3(inv_modelMat*ray.dir);
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
        Hit h0 = toWorldSpaceHit(x0, vx, t0);
		hits.push_back(h0);

		vec3 x1 = pk + t1*vk;
		Hit h1 = toWorldSpaceHit(x1, vx, t1);
		hits.push_back(h1);
	}
	// We can do the same calculations for a unit sphere
	// because of the transformations to local space

	// However, we want to only push the hits in world space.
	// Know that x' = n' for a unit sphere.

	// When transforming the normals, use the inverse transpose
	// for handling nonuniform scales

	// worth reiterating: the denominator for t is
	// the length of the unnormalized v'.

	// If instead use the normalized v, then the given
	// t value for the sphere will be incorrect, leading to
	// other objects clipping the sphere, or otherwise
}



void Plane::computeUVvectors(const glm::vec3& normal) {
    // https://computergraphics.stackexchange.com/questions/8382
    vec3 a = cross(normal, vec3(1, 0, 0));
    vec3 b = cross(normal, vec3(0, 1, 0));
    vec3 max_ab = dot(a, a) < dot(b, b) ? b : a;
    vec3 c = cross(normal, vec3(0, 0, 1));

    uvec = normalize(dot(max_ab, max_ab) < dot(c, c) ? c : max_ab);
    vvec = cross(normal, uvec);
}

// Assume that the plane isn't rotating over time,
// so set uvec and vvec only "once" knowing that
// the normal is finalized
void Plane::initialize() {
	this->normal = vec3(modelMat[1]);
	computeUVvectors(this->normal);
}

// The rotation of the plane is used as the normal
void Plane::intersect(const Ray& ray, vector<Hit>& hits) {
	vec3 &n = normal;
	// Compute the distance from the ray origin using:
	float t = dot(n, vec3(modelMat[3])-ray.getPos())/dot(n, ray.getDir());

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
    hit.m = material;
    hit.u = u; hit.v = v;
	hits.push_back(hit);
}


struct Pair { double min, max; };	

void swap(float &a, float &b) { float tmp = a; a = b; b = tmp; }

Pair checkAxis(float pos, float dir) {
	float tmin_num = -1 - pos;
	float tmax_num = 1 - pos;
	float tmin, tmax;
	if (std::abs(dir) < EPSILION) {
		// Direction is effectively 0,
		// but do this to preserve signs
		tmin = tmin_num*INF;
		tmax = tmax_num*INF;
	} else {
		tmin = tmin_num/dir;
		tmax = tmax_num/dir;
	}
	if (tmin > tmax) swap(tmin, tmax);
	Pair p; p.min = tmin; p.max = tmax;
	return p;
}

vec4 Box::computeNormal(const vec3& p) const {
	// Assume values range from [-1, 1]; do reciprocal so that values
	// close to 1 do not get truncated to 0 (total hack)
	float r_maxc = 1.0f/std::max(std::max(abs(p.x), abs(p.y)), abs(p.z));
	float cx = static_cast<int>(p.x*r_maxc);
	float cy = static_cast<int>(p.y*r_maxc);
	float cz = static_cast<int>(p.z*r_maxc);
	return vec4(cx, cy, cz, 1.0f);
}

vec2 Box::computeUV(const glm::vec3& p) const {
	// For any hit on the box, at least one component has
	// a value of 1, indicating the face that is hit. 
	// The information of the other 2 components are used 
	// to get the UV. this is temporary
	float maxc = std::max(std::max(abs(p.x), abs(p.y)), abs(p.z));
	float u, v;
	if (abs(p.x) == maxc) u = decimal(p.y), v = decimal(p.z);
	if (abs(p.y) == maxc) u = decimal(p.x), v = decimal(p.z);
	else 				  u = decimal(p.x), v = decimal(p.y);
	return vec2(u, v);
};

// Axis-aligned bounding box intersection
void Box::intersect(const Ray& ray, vector<Hit>& hits) {
	// Transform the ray back to model space first;
	// axis tests assume the box is at the origin
	vec3 pk = vec3(inv_modelMat*ray.pos);
	vec3 vx = vec3(inv_modelMat*ray.dir);
	vec3 vk = normalize(vx);

	Pair tx = checkAxis(pk.x, vk.x);
	Pair ty = checkAxis(pk.y, vk.y);
	Pair tz = checkAxis(pk.z, vk.z);

	float tmin = std::max(std::max(tx.min, ty.min), tz.min);
	float tmax = std::min(std::min(tx.max, ty.max), tz.max);

	if (tmin > tmax) return;

	vec3 x0 = pk + tmin*vk;
	Hit h0 = toWorldSpaceHit(x0, vx, tmin);
	hits.push_back(h0);

	vec3 x1 = pk + tmax*vk;
	Hit h1 = toWorldSpaceHit(x1, vx, tmax);
	hits.push_back(h1);
}

vec4 Cylinder::computeNormal(const vec3& x0) const {
	return vec4(x0.x, 0.0f, x0.z, 0.0f);
}

vec2 Cylinder::computeUV(const vec3& p) const {
	// TODO: uv for cylinders
	return vec2(0.0f, 0.0f);
} 

void Cylinder::intersect(const Ray& ray, vector<Hit>& hits) 
{
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
	Hit h0 = toWorldSpaceHit(x0, vx, t0);
	hits.push_back(h0);

	vec3 x1 = pk + t1*vk;
	Hit h1 = toWorldSpaceHit(x1, vx, t1); 
	hits.push_back(h1);
}


void Mesh::loadMesh(const string& meshName, const string& directoryPath, bool printVerticesCount) 
{
    // Load geometry
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> obj_mats;
	std::string errMsg;

	std::string meshPath = directoryPath;

	if (directoryPath.back() != '/') 
		meshPath += '/' + meshName; 
	else meshPath += meshName; 

	// boolean parameter enables triangulation of faces with 4+ vertices
	bool success = tinyobj::LoadObj(&attrib, &shapes, &obj_mats, &errMsg, 
		meshPath.c_str(), directoryPath.c_str(), true);

	if (!errMsg.empty()) std::cerr << errMsg << '\n';
	if (!success) return; 

	for (auto& shape : shapes) {
		size_t index_offset = 0;
		// Loop over faces. 
		for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
			u_char fv = shape.mesh.num_face_vertices.at(f);
			// Loop over vertices in face.
			for (u_char v = 0; v < fv; v++) {
				tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
				posBuf.push_back(attrib.vertices[3*idx.vertex_index+0]);
				posBuf.push_back(attrib.vertices[3*idx.vertex_index+1]);
				posBuf.push_back(attrib.vertices[3*idx.vertex_index+2]);

				if (!attrib.normals.empty()) {
					norBuf.push_back(attrib.normals[3*idx.normal_index+0]);
					norBuf.push_back(attrib.normals[3*idx.normal_index+1]);
					norBuf.push_back(attrib.normals[3*idx.normal_index+2]);
				}

				if (!attrib.texcoords.empty()) {
					texBuf.push_back(attrib.texcoords[2*idx.texcoord_index+0]);
					texBuf.push_back(attrib.texcoords[2*idx.texcoord_index+1]);
				}
			}
			index_offset += fv; // b/c of triangulate, offset is always += 3.
			// shape.mesh.material_ids[f];
		}
	}

	setBoundingRadius();

	if (posBuf.size() % 9 != 0) {
		// Otherwise, a segfault may occur from invalid
		// memory access (2:57 AM brain thinking).
		cerr << "posBuf.size() == " << posBuf.size() 
			 << "; not a multiple of 9\n";
		return;
	} 
	if (texBuf.size() % 6 != 0) {
		cerr << "texBuf.size() == " << texBuf.size()
			 << "; not a multiple of 6\n";
		return;
	}

    if (printVerticesCount)
	    clog << "Number of vertices: " << posBuf.size()/3 << '\n';
}

void Mesh::setBoundingRadius() 
{
	struct {
		Interval x;
		Interval y;
		Interval z;
	} bounds;

	if (posBuf.size() < 3) {
		this->boundingRadius = 0.0f;
		cerr << "Expected posBuf.size() > 3, got " << posBuf.size() << '\n';
		return;
	}

	// initialize
	bounds.x.min = INF; bounds.x.max = -INF;
	bounds.y.min = INF; bounds.y.max = -INF;
	bounds.z.min = INF; bounds.z.max = -INF;

	// Find the minimum and maximum of each component.
	for (size_t i = 0; i < posBuf.size(); i += 3) {
		float &x = posBuf.at(i+0);
		float &y = posBuf.at(i+1);
		float &z = posBuf.at(i+2);

		bounds.x.min = std::min(x, (float)bounds.x.min);
		bounds.y.min = std::min(y, (float)bounds.y.min);	
		bounds.z.min = std::min(z, (float)bounds.z.min);
		// what was I even thinking?
		bounds.x.max = std::max(x, (float)bounds.x.max);
		bounds.y.max = std::max(y, (float)bounds.y.max);	
		bounds.z.max = std::max(z, (float)bounds.z.max);
	}

	float minPos = std::min(std::min(bounds.x.min, bounds.y.min), bounds.z.min);
	float maxPos = std::max(std::max(bounds.x.max, bounds.y.max), bounds.z.max);

	// center the sphere wrt mesh's vertices
	float cx = (bounds.x.max + bounds.x.min)/2;
	float cy = (bounds.y.max + bounds.y.min)/2;
	float cz = (bounds.z.max + bounds.z.min)/2;
	this->meshCenter = vec3(cx, cy, cz);

	this->boundingRadius = (maxPos - minPos)/2;
}

void Mesh::fitToUnitBox()
{
	// Scale the vertex positions so that they fit within [-1, +1] in all three dimensions.
	glm::vec3 vmin(posBuf[0], posBuf[1], posBuf[2]);
	glm::vec3 vmax(posBuf[0], posBuf[1], posBuf[2]);
	for(size_t i = 0; i < posBuf.size(); i += 3) {
		glm::vec3 v(posBuf[i], posBuf[i+1], posBuf[i+2]);
		// I can't believe this is my (kSAM's) code
		vmin.x = std::min(vmin.x, v.x);
		vmin.y = std::min(vmin.y, v.y);
		vmin.z = std::min(vmin.z, v.z);
		vmax.x = std::max(vmax.x, v.x);
		vmax.y = std::max(vmax.y, v.y);
		vmax.z = std::max(vmax.z, v.z);
	}
	glm::vec3 center = 0.5f*(vmin + vmax);
	glm::vec3 diff = vmax - vmin;
	float diffmax = diff.x;
	diffmax = std::max(diffmax, diff.y);
	diffmax = std::max(diffmax, diff.z);
	float scale = 1.0f / diffmax;
	for(int i = 0; i < (int)posBuf.size(); i += 3) {
		posBuf[i+0] = (posBuf[i+0] - center.x) * scale;
		posBuf[i+1] = (posBuf[i+1] - center.y) * scale;
		posBuf[i+2] = (posBuf[i+2] - center.z) * scale;
	}

	// Readjust the bounding radius to account for changes in posBuf
	setBoundingRadius();
}

// construct new matrix just for the uniform sphere test
void Mesh::initialize() 
{
	sphereMat = glm::scale(modelMat, vec3(this->boundingRadius));
	inv_sphereMat = inverse(sphereMat);
}

// custom functions for speed; only do operations when needed

// TODO: test replacment with reference writing?
inline vec3 CROSS(const float *v1, const float *v2) {
	return vec3(v1[1]*v2[2]-v1[2]*v2[1],
				v1[2]*v2[0]-v1[0]*v2[2],
				v1[0]*v2[1]-v1[1]*v2[0]);
}

inline float DOT(const float *v1, const float *v2) {
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

inline vec3 SUB(const float *v1, const float *v2) {
	return vec3(v1[0]-v2[0], 
				v1[1]-v2[1], 
				v1[2]-v2[2]);
}

// This assumes that the position buffer size is a 
// multiple of 9. posBufOffset is the index of the
// x component of the 1st vertex.
// https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
bool Mesh::intersect_triangle(const vec3& orig, const vec3& dir, 
							  const size_t &i, /* posBufOffset */ 
							  float &t, float &u, float &v) 
{
	vec3 edge1, edge2, pvec, qvec, tvec;
	float det, inv_det;

	float *vt0 = &posBuf.at(0+i); 
	float *vt1 = &posBuf.at(3+i); 
	float *vt2 = &posBuf.at(6+i);

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

// Comment this out to show the actual model
// #define SHOW_BOUNDING_SPHERE

// Some floating-point error possible, or the normals are not normal
void Mesh::intersect(const Ray& ray, vector<Hit>& hits) {
	// transform ray to local coords
	vec3 l_rorig = vec3(inv_modelMat*ray.pos);

	// bounding sphere test
	vec3 pk, vx, vk;
	// here, use inv_sphereMat to accurately represent the bounding sphere
	pk = vec3(inv_sphereMat*(vec4(ray.getPos() - meshCenter, 1.0f)));
	vx = vec3(inv_sphereMat*(ray.dir));
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

	vec3 x0 = pk + t0*vk;
	vec3 wld_x0 = vec3(sphereMat*vec4(x0, 1.0f));
	// sphereMat used instead of inv_T, since size is uniform
	vec3 wld_n0 = normalize(vec3(sphereMat*vec4(x0, 0.0f)));
	float wld_t0 = t0/length(vx);
	Hit h0;
	h0.x = wld_x0;
	h0.n = wld_n0;
	h0.t = wld_t0;
	h0.m = material;
	hits.push_back(h0);

	vec3 x1 = pk + t1*vk;
	vec3 wld_x1 = vec3(sphereMat*vec4(x1, 1.0f));
	vec3 wld_n1 = normalize(vec3(sphereMat*vec4(x1, 0.0f)));
	float wld_t1 = t1/length(vx);
	Hit h1;
	h0.x = wld_x1;
	h0.n = wld_n1;
	h0.t = wld_t1;
	h0.m = material;
	hits.push_back(h1);
	return;
	#endif

	vec3 r_vk = normalize(vec3(inv_modelMat*ray.dir));

	// u and v refer to barycentric coordinates.
	float t, u, v;

	for (size_t i = 0, j = 0; i < posBuf.size(); i += 9, j += 6) {
		if (!intersect_triangle(l_rorig, r_vk, i, t, u, v)) continue;
		float w = 1.0f - v - u;
		// Because of branch prediction, I have chosen not to check for t here;
		// that should be handled in hits, given a positive min value
		vec3 rv = l_rorig + t*r_vk;
		vec3 wld_x = vec3(modelMat*vec4(rv, 1.0f));

		vec3 wld_n; float tex_u = 0.0f, tex_v = 0.0f;

		// loadmesh ensures a vertex only has 1 normal associated with it,
		// if there are any attribute normals
		if (i < norBuf.size()) {
			float nx = w*norBuf.at(0+i) + u*norBuf.at(3+i) + v*norBuf.at(6+i);
			float ny = w*norBuf.at(1+i) + u*norBuf.at(4+i) + v*norBuf.at(7+i);
			float nz = w*norBuf.at(2+i) + u*norBuf.at(5+i) + v*norBuf.at(8+i);

			// Because of floating-point error, the interpolated normal is no longer
			// normalized, so explicitly normalize it again.
			wld_n = normalize(vec3(invT_modelMat*vec4(nx, ny, nz, 0.0f)));
		}

		// Only two texture components per vertex, if there are any
		if (j < texBuf.size()) {
			tex_u = w*texBuf.at(0+j) + u*texBuf.at(2+j) + v*texBuf.at(4+j);
			tex_v = w*texBuf.at(1+j) + u*texBuf.at(3+j) + v*texBuf.at(5+j);
		}

		Hit h; 
		h.x = wld_x; h.n = wld_n; h.t = t; 
		h.u = tex_u; h.v = tex_v;
		h.m = this->material;
		hits.push_back(h);
	}
}	


bool cmp(const Hit& a, const Hit& b) { return a.t < b.t; }

void pushIntervals(vector<Interval>& intervals, const vector<Hit>& hits) {
	if (hits.empty()) {
		intervals.push_back(Interval::empty);
		return;
	}
	
	uint i = 0;
	if (hits.size() % 2 == 1) {
		// first hit originates inside the csg, which
		// occurs with rays other than primary
		intervals.push_back(Interval(0.0f, hits.at(0).t));
		i = 1;
	}

	// assume the hits list is sorted and is for one shape;
	// each pair represents enter and exit points for primitives
	for (; i < hits.size(); i += 2) {
		intervals.push_back(Interval(hits.at(i).t, hits.at(i+1).t));
	}
}

void CSG::intersect(const Ray& ray, std::vector<Hit>& hits) {
	vector<Hit> rightHits;
	this->left->intersect(ray, hits);
	this->right->intersect(ray, rightHits);

	vector<Interval> l_intervals;
	vector<Interval> r_intervals; 
	
	pushIntervals(l_intervals, hits);
	pushIntervals(r_intervals, rightHits);
	
	for (auto& it : rightHits) {
		// ensure that faces in difference csgs are properly lit
		if (operationType == OperationType::Difference) it.n = -it.n;
		// it barely makes a difference whether this copied or not,
		// so just leave this alone
		hits.push_back(std::move(it));
	}
	std::sort(hits.begin(), hits.end(), cmp);

	filter_intersections(l_intervals, r_intervals, hits);
}

bool intersection_allowed(OperationType op, bool inL, bool inR) {
	switch (op) {
	case OperationType::Union:
		return (inL && !inR) || (!inL && inR);
	case OperationType::Intersection:
		return (inL && inR); 
	case OperationType::Difference:
		return (inL && !inR);
	default:
		break;
	}
	return false;
}

bool insideIntervalAfter(
	const vector<Interval>& intervals,
	float t, float s = -1.0f) 
{
	for (auto& interval : intervals) {
		// Don't consider any intervals before this 
		// distance t in the ray; this is why a
		// loop over intervals is done 
		if (t > interval.max) continue;

		return (interval.min - t < EPSILION) && 
			   (interval.max - t > s*EPSILION);
	}
	return false;
}

void CSG::filter_intersections(
	const vector<Interval>& l_intervals,
	const vector<Interval>& r_intervals, 
	std::vector<Hit>& hits)
{	
	bool inl = false, inr = false; 
	// unoptimal way to make difference show right shape's faces; hacky
	float s = this->operationType == OperationType::Difference ? 1.0f : -1.0f;
	vector<Hit> new_hits;
	for (auto& hit : hits) {
		inl = insideIntervalAfter(l_intervals, hit.t, s);
		inr = insideIntervalAfter(r_intervals, hit.t, s);
		// Assume that the given hits list is sorted in ascending order
		// of t; that is, the first intersection is outside both shapes
		if (intersection_allowed(this->operationType, inl, inr)) {
			new_hits.push_back(hit);
		}
	}
	hits = std::move(new_hits);
}