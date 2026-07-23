#include "MeshBuffer.hpp"

#include <iostream>
#include "util/umath.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "external/tiny_obj_loader.h"

using glm::vec3;
using glm::vec4;

MeshBuffer::MeshBuffer(const std::string& objName, const std::string& directory)  
{
    loadMesh(objName, directory);
}

void MeshBuffer::loadMesh(const std::string& meshName, 
                          const std::string& directoryPath, 
                          bool printVerticesCount) 
{
    using namespace fastVecOp;
    using glm::vec3;

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

    posBuf.clear();
    norBuf.clear();
    texBuf.clear();

	for (auto& shape : shapes) {
		size_t index_offset = 0;
		// Loop over faces. 
		for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
			u_char fv = shape.mesh.num_face_vertices.at(f);
			// Loop over vertices in face.
			// Because of triangulation, fv is always 3.
			for (u_char v = 0; v < fv; ++v) {
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

			// Only guaranteed to work if triangulate is enabled,
			// and if vertices are duplicated (not shared)
			// Do flat shading by default
			if (attrib.normals.empty() && fv >= 3) {
				// at least 9 vertices must be added per face
				size_t end = posBuf.size();
				assert(end >= 9 + fv*f);
				float *vt0 = &posBuf.at(end - 9); 
				float *vt1 = &posBuf.at(end - 6); 
				float *vt2 = &posBuf.at(end - 3);
				vec3 edge1 = SUB(vt1, vt0);
				vec3 edge2 = SUB(vt2, vt0);
				vec3 norm = CROSS(&edge1.x, &edge2.x);
				norm = glm::normalize(norm);
				// Make sure that the size of the 
				for (int i = 0; i < fv; ++i) {
					// Fairly wasteful, but oh well
					norBuf.push_back(norm.x);
					norBuf.push_back(norm.y);
					norBuf.push_back(norm.z);
				}
			} else if (attrib.normals.empty()) {
				// Push some bogus instead
				for (int i = 0; i < fv; ++i) {
					vec3 norm = vec3(0.577);
					norBuf.push_back(norm.x);
					norBuf.push_back(norm.y);
					norBuf.push_back(norm.z);
				}
			}

			// Smooth shading would require passing through all
			// the faces to get the vertices that are shared, incrementing
			// the normal based on the cross product from the triangle,
			// then doing a second pass to normalize the normals.
			// Maybe in the future.

			assert(norBuf.size() == posBuf.size());

			index_offset += fv;
			// shape.mesh.material_ids[f];
		}
	}

	setBoundingRadius();

	if (posBuf.size() % 9 != 0) {
		// Otherwise, a segfault may occur from invalid
		// memory access (2:57 AM brain thinking).
		std::cerr << "posBuf.size() == " << posBuf.size() 
			      << "; not a multiple of 9\n";
		return;
	} 
	if (texBuf.size() % 6 != 0) {
		std::cerr << "texBuf.size() == " << texBuf.size()
			      << "; not a multiple of 6\n";
		return;
	}

    if (printVerticesCount)
	    std::clog << "Number of vertices: " << posBuf.size()/3 << '\n';
}

void MeshBuffer::setBoundingRadius() 
{
    using CONSTANTS::INF;

	struct {
		Interval x;
		Interval y;
		Interval z;
	} bounds;

	if (posBuf.size() < 3) {
		this->boundingRadius = 0.0f;
		std::cerr << "Expected posBuf.size() > 3, got " << posBuf.size() << '\n';
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

	// center the sphere wrt mesh's vertices
	float cx = (bounds.x.max + bounds.x.min) * 0.5;
	float cy = (bounds.y.max + bounds.y.min) * 0.5;
	float cz = (bounds.z.max + bounds.z.min) * 0.5;
	this->meshCenter = vec4(cx, cy, cz, 1.f);

	vec3 minCoord = vec3(bounds.x.min, bounds.y.min, bounds.z.min);
	vec3 maxCoord = vec3(bounds.x.max, bounds.y.max, bounds.z.max);

	// vec3 lengths = vec3(bounds.x.max - bounds.x.min,
						// bounds.y.max - bounds.y.min,
						// bounds.z.max - bounds.z.min);

	this->boundingRadius = glm::max(
		glm::distance(minCoord, vec3(this->meshCenter)),
		glm::distance(maxCoord, vec3(this->meshCenter))
	);

	// construct new matrix just for the uniform sphere test
	this->boundMat = glm::translate(glm::mat4(1.f), vec3(this->meshCenter));
	this->boundMat = glm::scale(this->boundMat, vec3(this->boundingRadius));
	this->inv_boundMat = inverse(this->boundMat);
}