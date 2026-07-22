#pragma once
#ifndef UTIL_MODELMAT_H
#define UTIL_MODELMAT_H

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

class ModelMatConstr {
public:
    void setScale(const glm::vec3& s) { scale = s; }
    void setScale(const float r) { scale = glm::vec3(r); }
	void setPosition(const glm::vec3& p) { position = p; }
	void setRotation(const glm::vec3& r) { rotation = r; }
    glm::mat4 getMatrix() {
        glm::mat4 modMat(1.0f); 
        modMat[3] = glm::vec4(position, 1.0f);
        modMat = glm::rotate(modMat, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        modMat = glm::rotate(modMat, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        modMat = glm::rotate(modMat, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        
        modMat = glm::scale(modMat, scale);
        return modMat;
    }
    void clear() {
        position = glm::vec3(0.0f);
        scale    = glm::vec3(1.0f);
        rotation = glm::vec3(0.0f);
    };
private:
    glm::vec3 position = glm::vec3(0.0f); // position in world coordinates
	glm::vec3 scale    = glm::vec3(1.0f); // scales for each axis
	glm::vec3 rotation = glm::vec3(0.0f); // contains the Euler angles for each axis
};

#endif 