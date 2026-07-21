#pragma once
#ifndef MESHBUFFER_H
#define MESHBUFFER_H

#include "stn.hpp"
#include <string>
#include <vector>

// custom functions for "speed," this can be ~50% faster 
// defining them here is also really funny
namespace fastVecOp {
    using glm::vec3;
    
    constexpr vec3 CROSS(const float *v1, const float *v2) {
        return vec3(v1[1]*v2[2]-v1[2]*v2[1],
                    v1[2]*v2[0]-v1[0]*v2[2],
                    v1[0]*v2[1]-v1[1]*v2[0]);
    }

    constexpr float DOT(const float *v1, const float *v2) {
        return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
    }

    constexpr vec3 SUB(const float *v1, const float *v2) {
        return vec3(v1[0]-v2[0], 
                    v1[1]-v2[1], 
                    v1[2]-v2[2]);
    }
}

class MeshBuffer {
public:
    MeshBuffer() noexcept = default;
	MeshBuffer(const std::string& objName, const std::string& directory);
	virtual ~MeshBuffer() = default;

    // Assume that the .obj file and .mtl files are in the same directory.
    // Overwrites any existing buffer if loading is successful.
    void loadMesh(const std::string &meshName, 
                  const std::string &directoryPath, 
                  bool printVerticesCount = true);
                  
    // All vertices get readjusted to fit in a unit box.
    void fitToUnitBox();

    // boilerplate getters

    inline const float* getPos(size_t i) { return &posBuf.at(i); }
    inline const float* getNor(size_t i) { return &norBuf.at(i); }
    inline const float* getTex(size_t i) { return &texBuf.at(i); }

    inline const std::vector<float>& getPosBuf() { return posBuf; }
    inline const std::vector<float>& getNorBuf() { return norBuf; }
    inline const std::vector<float>& getTexBuf() { return texBuf; }

    inline size_t getPosSize() { return posBuf.size(); }
    inline size_t getNorSize() { return norBuf.size(); }
    inline size_t getTexSize() { return texBuf.size(); }

    inline const glm::mat4& getBoundMat() { return boundMat; }
    inline const glm::mat4& getInvBoundMat() { return inv_boundMat; }
    
private:
    // These buffers are only populated when a mesh is loaded.
	std::vector<float> posBuf;
	std::vector<float> norBuf;
	std::vector<float> texBuf;

    glm::mat4 boundMat;      // matrix used for bounding sphere tests 
	glm::mat4 inv_boundMat;  // only useful for debugging
    glm::vec4 meshCenter; 	 // Defined in model/local space.
    float boundingRadius; 

    void setBoundingRadius();
};

#endif