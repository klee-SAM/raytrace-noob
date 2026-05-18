#pragma once
#include "stn.hpp"

// Utility math functions and classes

namespace CONSTANTS {
    constexpr double PI = 3.14159265358979323846;
    constexpr double R_PI = 1.0/PI;

    constexpr double INF = std::numeric_limits<double>::infinity();
    constexpr double EPSILION = glm::epsilon<float>();
}

class ModelMatConstr {
public:
    void setScale(const glm::vec3& s) { scale = s; }
    void setScale(const float r) { scale = glm::vec3(r); }
	void setPosition(const glm::vec3& p) { position = p; }
	void setRotation(const glm::vec3& r) { rotation = r; }
    glm::mat4 getMatrix() {
        glm::mat4 modMat(1.0f); 
        modMat[3] = glm::vec4(position, 1.0f);
        modMat *= glm::rotate(glm::mat4(1.0f), rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        modMat *= glm::rotate(glm::mat4(1.0f), rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        modMat *= glm::rotate(glm::mat4(1.0f), rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        
        modMat *= glm::scale(glm::mat4(1.0f), scale);
        return modMat;
    }
    void clear();
private:
    glm::vec3 position = glm::vec3(0.0f); // position in world coordinates
	glm::vec3 scale    = glm::vec3(1.0f); // scales for each axis
	glm::vec3 rotation = glm::vec3(0.0f); // contains the Euler angles for each axis
};

// Create an anonymous namespace so the linker doesn't complain
// about multiple definitions
namespace {
    using namespace CONSTANTS;
    class Interval {
    public:
        double min, max;

        Interval() : min(INF), max(-INF) {} // empty
        Interval(double min, double max) : min(min), max(max) {}

        double size() const { return max - min; }
        bool contains(double x) const { return (min <= x) && (x <= max); }
        bool surrounds(double x) const { return (x < min) && (max < x);}    

        static const Interval empty, world;
    };

    const Interval Interval::empty = Interval(INF, -INF);
    const Interval Interval::world = Interval(-INF, INF); 
}

// Returns the number without the integer part (-2.6 -> -0.6)
constexpr float decimal(float x) { return x - (int)x; }