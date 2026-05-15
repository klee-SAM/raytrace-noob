#include "stn.hpp"
#include "umath.hpp"

// #define TINYOBJLOADER_IMPLEMENTATION
// #include "tiny_obj_loader.h"

#include "Shape.hpp"
// #include "raytri.c"

using namespace std;
using namespace glm;

void Shape::setModelMatrix(const mat4& m) {
	modelMat = m;
	inv_modelMat = inverse(m);
	invT_modelMat = inverse(transpose(m));
}



// https://en.wikipedia.org/wiki/UV_mapping#Finding_UV_on_a_sphere
vec2 sphere_computeUV(const vec3& p) {
    float u = (1.0 + std::atan2(p.z, p.x)*R_PI)/2;
    float v = 0.5 + std::asin(p.y)*R_PI;
    return vec2(u, v);
}

void Sphere::intersect(const Ray& ray, vector<Hit>& hits) {
	vec3 pk = vec3(inv_modelMat*vec4(ray.pos, 1.0f));
	vec3 vx = vec3(inv_modelMat*vec4(ray.dir, 0.0f));
	vec3 vk = normalize(vx);
	float a, b, c;
	float d, den;

	a = dot(vk, vk);
	b = 2*dot(vk, pk);
	c = dot(pk, pk) - 1.0f;
	d = b*b - 4.0f*a*c;
	den = 1.0f/(2.0f*a);

	if (d > 0.0f) {
		// We can do the same calculations for a unit sphere
		// because of the transformations to local space
		float t0 = (-b - glm::sqrt(d))*den; 
		float t1 = (-b + glm::sqrt(d))*den;

		// However, we want to only push the hits in world space.
		// Know that x' = n' for a unit sphere.
		vec3 x0 = pk + t0*vk;
		vec3 wld_x0 = vec3(modelMat*vec4(x0, 1.0f));
		// When transforming the normals, use the inverse transpose
		// for handling nonuniform scales
		vec3 wld_n0 = normalize(vec3(invT_modelMat*vec4(x0, 0.0f)));
		// worth reiterating: the denominator for t is
		// the length of the unnormalized v'.
		float wld_t0 = t0/length(vx);

        Hit h0; 
        h0.x = wld_x0; 
        h0.n = wld_n0; 
        h0.t = wld_t0;
        h0.m = material;
        vec2 uv0 = sphere_computeUV(x0);
        h0.u = uv0.x;
        h0.v = uv0.y;
		hits.push_back(h0);

		vec3 x1 = pk + t1*vk;
		vec3 wld_x1 = vec3(modelMat*vec4(x1, 1.0f));
		vec3 wld_n1 = normalize(vec3(invT_modelMat*vec4(x1, 0.0f)));
		// If instead use the normalized v, then the given
		// t value for the sphere will be incorrect, leading to
		// other objects clipping the sphere, or otherwise
		float wld_t1 = t1/length(vx);

		Hit h1; 
        h1.x = wld_x1; 
        h1.n = wld_n1; 
        h1.t = wld_t1;
        h1.m = material;
        vec2 uv1 = sphere_computeUV(x1);
        h1.u = uv1.x;
        h1.v = uv1.y;
		hits.push_back(h1);
	}
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

// The rotation of the plane is used as the normal
void Plane::intersect(const Ray& ray, vector<Hit>& hits) {
	vec3 n = vec3(modelMat[1]);
	// Compute the distance from the ray origin using:
	float t = dot(n, vec3(modelMat[3])-ray.pos)/dot(n, ray.dir);

	// and the position of intersection by
	vec3 offset = t*ray.dir;
	vec3 x = ray.pos + offset;

	// Compute u, v coordinates
    if (!computedUVvectors) { 
        // Assume that the plane isn't rotating over time,
        // so set uvec and vvec only "once" knowing that
        // the normal is finalized
        computeUVvectors(n);
        computedUVvectors = true;
    }
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

vec4 box_normal_at(const vec3& p) {
	// Assume values range from [-1, 1]; do reciprocal so that values
	// close to 1 do not get truncated to 0
	float r_maxc = 1.0f/std::max(std::max(abs(p.x), abs(p.y)), abs(p.z));
	float cx = static_cast<int>(p.x*r_maxc);
	float cy = static_cast<int>(p.y*r_maxc);
	float cz = static_cast<int>(p.z*r_maxc);
	return vec4(cx, cy, cz, 1.0f);
}

// Axis-aligned bounding box intersection
void Box::intersect(const Ray& ray, vector<Hit>& hits) {
	// Transform the ray back to model space first;
	// axis tests assume the box is at the origin
	vec3 pk = vec3(inv_modelMat*vec4(ray.pos, 1.0f));
	vec3 vx = vec3(inv_modelMat*vec4(ray.dir, 0.0f));
	vec3 vk = normalize(vx);

	Pair tx = checkAxis(pk.x, vk.x);
	Pair ty = checkAxis(pk.y, vk.y);
	Pair tz = checkAxis(pk.z, vk.z);

	float tmin = std::max(std::max(tx.min, ty.min), tz.min);
	float tmax = std::min(std::min(tx.max, ty.max), tz.max);

	if (tmin > tmax) return;

	vec3 x0 = pk + tmin*vk;
	vec3 wld_x0 = vec3(modelMat*vec4(x0, 1.0f));
	vec3 wld_n0 = normalize(vec3(invT_modelMat*box_normal_at(x0)));
	float wld_t0 = tmin/length(vx);

	vec3 x1 = pk + tmax*vk;
	vec3 wld_x1 = vec3(modelMat*vec4(x1, 1.0f));
	vec3 wld_n1 = normalize(vec3(invT_modelMat*box_normal_at(x1)));
	float wld_t1 = tmax/length(vx);

	Hit h0; 
	h0.x = wld_x0; 
	h0.n = wld_n0; 
	h0.t = wld_t0;
	h0.m = material;
	h0.u = 0;
	h0.v = 0;
	hits.push_back(h0);

	Hit h1; 
	h1.x = wld_x1; 
	h1.n = wld_n1; 
	h1.t = wld_t1;
	h1.m = material;
	h1.u = 0;
	h1.v = 0;
	hits.push_back(h1);
}



void Cylinder::intersect(const Ray& ray, vector<Hit>& hits) {
	vec3 pk = vec3(inv_modelMat*vec4(ray.pos, 1.0f));
	vec3 vx = vec3(inv_modelMat*vec4(ray.dir, 0.0f));
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
	vec3 wld_x0 = vec3(modelMat*vec4(x0, 1.0f));
	vec3 wld_n0 = normalize(vec3(invT_modelMat*vec4(x0.x, 0.0f, x0.z, 0.0f)));
	float wld_t0 = t0/glm::length(vx);

	Hit h0; 
	h0.x = wld_x0; 
	h0.n = wld_n0; 
	h0.t = wld_t0;
	h0.m = material;
	h0.u = 0;
	h0.v = 0;
	hits.push_back(h0);

	vec3 x1 = pk + t1*vk;
	vec3 wld_x1 = vec3(modelMat*vec4(x1, 1.0f));
	vec3 wld_n1 = normalize(vec3(invT_modelMat*vec4(x0.x, 0.0f, x0.z, 0.0f)));
	float wld_t1 = t1/glm::length(vx);

	Hit h1; 
	h1.x = wld_x1; 
	h1.n = wld_n1; 
	h1.t = wld_t1;
	h1.m = material;
	h1.u = 0;
	h1.v = 0;
	hits.push_back(h1);
}
/*
Mesh::Mesh() {
}
Mesh::~Mesh() {
}

const float MAX_FLOAT = numeric_limits<float>::max();

void Mesh::loadMesh(const string& meshName, bool printVertices) 
{
    // Load geometry
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	string warnStr, errStr;
	bool rc = tinyobj::LoadObj(&attrib, &shapes, &materials, &warnStr, &errStr, meshName.c_str());
	if(!rc) {
		cerr << errStr << endl;
	} else {
		// Some OBJ files have different indices for vertex positions, normals,
		// and texture coordinates. For example, a cube corner vertex may have
		// three different normals. Here, we are going to duplicate all such
		// vertices.
		// Loop over shapes
		for(size_t s = 0; s < shapes.size(); s++) {
			// Loop over faces (polygons)
			size_t index_offset = 0;
			for(size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
				size_t fv = shapes[s].mesh.num_face_vertices[f];
				// Loop over vertices in the face.
				for(size_t v = 0; v < fv; v++) {
					// access to vertex
					tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
					posBuf.push_back(attrib.vertices[3*idx.vertex_index+0]);
					posBuf.push_back(attrib.vertices[3*idx.vertex_index+1]);
					posBuf.push_back(attrib.vertices[3*idx.vertex_index+2]);
					if(!attrib.normals.empty()) {
						norBuf.push_back(attrib.normals[3*idx.normal_index+0]);
						norBuf.push_back(attrib.normals[3*idx.normal_index+1]);
						norBuf.push_back(attrib.normals[3*idx.normal_index+2]);
					}
					if(!attrib.texcoords.empty()) {
						texBuf.push_back(attrib.texcoords[2*idx.texcoord_index+0]);
						texBuf.push_back(attrib.texcoords[2*idx.texcoord_index+1]);
					}
				}
				index_offset += fv;
				// per-face material (IGNORE)
				// shapes[s].mesh.material_ids[f];
			}
		}
	}

	setBoundingRadius();

    if (printVertices)
	    cout << "Number of vertices: " << posBuf.size()/3 << endl;
}

// returned vector contains data in this order:
// { minX, maxY, minZ, ...}
// old code so bad I rewrite it to be verbose
//...
void Mesh::setBoundingRadius() 
{
	float minPos = MAX_FLOAT;
	float maxPos = -MAX_FLOAT;
	vector<float> bounds = getBoundingBox();
	for (float& x : bounds) {
		if (minPos > x) minPos = x;
		if (maxPos < x) maxPos = x;
	}
	// if parts of mesh are cut off, blame this
	this->boundingRadius = (maxPos - minPos)/2;

	float cx = (bounds.at(3) + bounds.at(0))/2;
	float cy = (bounds.at(4) + bounds.at(1))/2;
	float cz = (bounds.at(5) + bounds.at(2))/2;
	this->meshCenter = vec3(cx, cy, cz);
}

// returned vector contains data in this order:
// { minX, minY, minZ, maxX, maxY, maxZ}
vector<float> Mesh::getBoundingBox() 
{
	vector<float> bounds(6, 0.0f);
	if (posBuf.size() < 6) return bounds;
	// initialize
	bounds.at(0) = MAX_FLOAT;
	bounds.at(1) = MAX_FLOAT;
	bounds.at(2) = MAX_FLOAT;

	bounds.at(3) = -MAX_FLOAT;
	bounds.at(4) = -MAX_FLOAT;
	bounds.at(5) = -MAX_FLOAT;

	for (size_t i = 0; i < posBuf.size(); i += 6) {
		// write the minimum of the coord
		bounds.at(0) = posBuf.at(i  ) < bounds.at(0) ? posBuf.at(i  ) : bounds.at(0);
		bounds.at(1) = posBuf.at(i+1) < bounds.at(1) ? posBuf.at(i+1) : bounds.at(1);	
		bounds.at(2) = posBuf.at(i+2) < bounds.at(2) ? posBuf.at(i+2) : bounds.at(2);
		// max
		bounds.at(3) = posBuf.at(i+3) > bounds.at(3) ? posBuf.at(i+3) : bounds.at(3);
		bounds.at(4) = posBuf.at(i+4) > bounds.at(4) ? posBuf.at(i+4) : bounds.at(4);	
		bounds.at(5) = posBuf.at(i+5) > bounds.at(5) ? posBuf.at(i+5) : bounds.at(5);
	}
	return bounds;
}

void Mesh::fitToUnitBox()
{
	// Scale the vertex positions so that they fit within [-1, +1] in all three dimensions.
	glm::vec3 vmin(posBuf[0], posBuf[1], posBuf[2]);
	glm::vec3 vmax(posBuf[0], posBuf[1], posBuf[2]);
	for(int i = 0; i < (int)posBuf.size(); i += 3) {
		glm::vec3 v(posBuf[i], posBuf[i+1], posBuf[i+2]);
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
		posBuf[i  ] = (posBuf[i  ] - center.x) * scale;
		posBuf[i+1] = (posBuf[i+1] - center.y) * scale;
		posBuf[i+2] = (posBuf[i+2] - center.z) * scale;
	}

	// Readjust the bounding radius to account for changes in posBuf
	setBoundingRadius();
}

// construct new matrix just for the sphere test
void Mesh::transform() {
	MatrixStack MV;
	MV.pushMatrix();
	MV.translate(position);
	MV.rotate(rotation.z, vec3(0,0,1));
	MV.rotate(rotation.y, vec3(0,1,0));
	MV.rotate(rotation.x, vec3(1,0,0));
	MV.scale(scale);

	MV.pushMatrix();
	MV.scale(boundingRadius);
	sphereMat = MV.topMatrix(); // only useful for debugging
	inv_sphereMat = inverse(MV.topMatrix());
	invT_sphereMat = inverse(transpose(MV.topMatrix())); // likewise
	MV.popMatrix();

	modelMat = MV.topMatrix();
	inv_modelMat = inverse(MV.topMatrix());
	invT_modelMat = inverse(transpose(MV.topMatrix()));
	// I am lazy.
}

const bool TEST_BOUNDING_SPHERE = false;

// Some floating-point error possible, or the normals are not normal
void Mesh::intersect(const Ray& ray, vector<Hit>& hits) {
	// transform ray to local coords
	vec3 l_rorig = vec3(inv_modelMat*vec4(ray.origin, 1.0f));

	vec3 pk;
	vec3 vx;
	if (TEST_BOUNDING_SPHERE) {
		// here, use inv_sphereMat to accurately represent the bounding sphere
		pk = vec3(inv_sphereMat*vec4(ray.origin, 1.0f)) - meshCenter;
		vx = vec3(inv_sphereMat*vec4(ray.direction, 0.0f));
	} else {
		pk = l_rorig - meshCenter;
		vx = vec3(inv_modelMat*vec4(ray.direction, 0.0f));
	}
	vec3 vk = normalize(vx);
	float a, b, c;
	float d, den;

	a = dot(vk, vk);
	b = 2*dot(vk, pk);
	c = dot(pk, pk) - 1;
	d = b*b - 4*a*c;
	den = 1.0f/(2*a);

	if (d <= 0.0f) return;

	if (TEST_BOUNDING_SPHERE) {
		float t0 = (-b - glm::sqrt(d))*den; 
		float t1 = (-b + glm::sqrt(d))*den;

		vec3 x0 = pk + t0*vk;
		vec3 wld_x0 = vec3(sphereMat*vec4(x0, 1.0f));
		vec3 wld_n0 = normalize(vec3(invT_sphereMat*vec4(x0, 0.0f)));
		float wld_t0 = t0/length(vx);
		hits.push_back(Hit(wld_x0, wld_n0, wld_t0, material));

		vec3 x1 = pk + t1*vk;
		vec3 wld_x1 = vec3(sphereMat*vec4(x1, 1.0f));
		vec3 wld_n1 = normalize(vec3(invT_sphereMat*vec4(x1, 0.0f)));
		float wld_t1 = t1/length(vx);
		hits.push_back(Hit(wld_x1, wld_n1, wld_t1, material));
		return;
	}

	float rorig[3] = {l_rorig.x, l_rorig.y, l_rorig.z};
	float rdir[3] = {vk.x, vk.y, vk.z};

	float t, u, v;

	if (posBuf.size() % 9 != 0) {
		// Otherwise, a segfault may occur from invalid
		// memory access (2:57 AM brain thinking).
		cerr << "posBuf.size() == " 
			 << posBuf.size() 
			 << "; unsafe size\n";
		return;
	} 
	for (size_t i = 0; i < posBuf.size(); i += 9) {
		// float vert0[3], float vert1[3], and float vert2[3] are represented
		// by respective offsets. Use pointer magic for speed; this gives me a ~2x speedup:
		if (!intersect_triangle3(&l_rorig.x, &vk.x, &posBuf.at(0+i), &posBuf.at(3+i), &posBuf.at(6+i), &t, &u, &v)) continue;
		float w = 1.0f - v - u;
		// Because of branch prediction, I have chosen not to check for t here;
		// that should be handled in hits, given a positive min value
		float rx = rorig[0]+t*rdir[0];
		float ry = rorig[1]+t*rdir[1];
		float rz = rorig[2]+t*rdir[2];
		vec3 wld_x = vec3(modelMat*vec4(rx, ry, rz, 1.0f));
		// assume that hopefully the normals for the 3 vertices are the same
		// loadmesh ensures a vertex only has 1 normal associated with it
		float nx = w*norBuf.at(0+i) + u*norBuf.at(3+i) + v*norBuf.at(6+i);
		float ny = w*norBuf.at(1+i) + u*norBuf.at(4+i) + v*norBuf.at(7+i);
		float nz = w*norBuf.at(2+i) + u*norBuf.at(5+i) + v*norBuf.at(8+i);
		// Because of floating-point error, the interpolated normal is no longer
		// normalized, so explicitly normalize it again. This is why we 
		// renormalize the attribute normal in fragment shaders.
		vec3 wld_n = normalize(vec3(invT_modelMat*vec4(nx, ny, nz, 0.0f)));
		hits.push_back(Hit(wld_x, wld_n, t, this->material));
	}
}	


// --

CSG::CSG() {}

CSG::~CSG() {}

void CSG::intersect(const Ray& ray, std::vector<Hit>& hits) {
	local_intersect(ray, hits);
}

bool cmp(const Hit& a, const Hit& b) { return a.t < b.t; }

void CSG::local_intersect(const Ray& ray, std::vector<Hit>& hits) {
	vector<Hit> leftHits, rightHits;
	this->left->intersect(ray, leftHits);
	this->right->intersect(ray, rightHits);
	float lt_min, lt_max, rt_min, rt_max;
	// Assume that only primitives are hit, and that 
	// each primitive returns either 0 or 2 intersections
	if (leftHits.size() < 2) {
		lt_min = 0.0f; lt_max = 0.0f;
	} else {
		lt_min = leftHits.at(0).t;
		lt_max = leftHits.at(1).t;
	}
	if (rightHits.size() < 2) {
		rt_min = 0.0f; rt_max = 0.0f;
	} else {
		rt_min = rightHits.at(0).t;
		rt_max = rightHits.at(1).t;
	}
	vector<Hit> nhits(std::move(leftHits));
	for (auto& it : rightHits) {
		// ensure that faces in difference csgs are properly lit
		if (operationType == OperationType::Difference) it.n = -it.n;
		nhits.push_back(std::move(it));
	}
	std::sort(nhits.begin(), nhits.end(), cmp);

	filter_intersections(lt_min, lt_max, rt_min, rt_max, nhits);
	hits = std::move(nhits);
}

bool intersection_allowed(OperationType op, bool inL, bool inR)
{
	if (op == OperationType::Union)
		return (inL && !inR) || (!inL && inR);
	else if (op == OperationType::Intersection)
		return (inL && inR);
	else if (op == OperationType::Difference)
		return (inL && !inR);
	return false;
}

const float EPSILION = 0.0000005f;

void CSG::filter_intersections(
	const float &lt_min, 
	const float &lt_max,
	const float &rt_min,
	const float &rt_max,   
	std::vector<Hit>& hits)
{
	bool inl = false, inr = false; 
	// unoptimal way to make difference show right shape's faces
	float s = this->operationType == OperationType::Difference ? -1.0f : 0.0f;
	// Note: I will not fall to premature optimization
	// Note: I will not fall to premature optimization
	// Note: I will not fall to premature optimization
	vector<Hit> new_hits;
	for (auto& hit : hits) {
		// Dark magic numbers for no speckles on reflected faces :D
		inl = (lt_min - EPSILION <= hit.t) && (hit.t <= lt_max + s*EPSILION);
		// this is specifically the way it is to have faces properly show
		// for difference
		inr = (rt_min - s*EPSILION <= hit.t) && (hit.t <= rt_max + s*EPSILION);
		// Assume that the given hits list is sorted in ascending order
		// of t; that is, the first intersection is outside both shapes
		if (intersection_allowed(this->operationType, inl, inr)) {
			new_hits.push_back(hit);
		}
	}
	hits = std::move(new_hits);
}
*/