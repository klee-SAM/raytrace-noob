#pragma once
#include "stn.h"

// Utility math functions and classes

class ModelMatConstr {
public:
    void setScale(const glm::vec3& s) { scale = s; }
    void setScale(const float r) { scale = glm::vec3(r); }
	void setPosition(const glm::vec3& p) { position = p; }
	void setRotation(const glm::vec3& r) { rotation = r; }
    glm::mat4 getMatrix() { /* evil */
        glm::mat4 modMat(1.0f); modMat[3] = glm::vec4(position, 1.0f);
        modMat *= glm::rotate(glm::mat4(1.0f), rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        modMat *= glm::rotate(glm::mat4(1.0f), rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        modMat *= glm::rotate(glm::mat4(1.0f), rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        modMat[0][0] *= scale.x; modMat[1][1] *= scale.y; modMat[2][2] *= scale.z;
    }
    void clear();
private:
    glm::vec3 position; // position in world coordinates
	glm::vec3 scale; // scales for each axis
	glm::vec3 rotation; // contains the Euler angles for each axis
};

const double PI = 3.14159265358979323846;
const double R_PI = 1.0/PI;

// TODO: interval class from raytracing one weekend