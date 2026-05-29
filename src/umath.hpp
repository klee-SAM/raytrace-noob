#pragma once
#include "stn.hpp"

typedef glm::vec2 vec2;

// Utility math functions and classes

namespace CONSTANTS {
    constexpr float PI = 3.14159265358979323846f;
    constexpr float R_PI = 1.0f / PI;

    constexpr float INF = std::numeric_limits<float>::infinity();
    constexpr float EPSILION = glm::epsilon<float>();
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
constexpr float decimal(float x) { return x - static_cast<int>(x); }
// Like decimal, but returns a positive number instead
constexpr float fract(float x) { return x - std::floor(x); }
// https://stackoverflow.com/questions/1903954/
constexpr int sgn(float val) { return (0.0f < val) - (val < 0.0f); }

// Pseudo-random generation using lookup tables. 
namespace prand {
    // Much faster than randomly generating for
    // each ray, and less noise. However, this 
    // means that sampling beyond 18 per instance
    // doesn't lead to better results
    constexpr size_t N = 18;
    // Thank you professor sueda
    static constexpr vec2 poissonDiskData[N] = {
        vec2(-0.220147, 0.976896),
        vec2(-0.735514, 0.693436),
        vec2(-0.200476, 0.310353),
        vec2( 0.180822, 0.454146),
        vec2( 0.292754, 0.937414),
        vec2( 0.564255, 0.207879),
        vec2( 0.178031, 0.024583),
        vec2( 0.613912,-0.205936),
        vec2(-0.385540,-0.070092),
        vec2( 0.962838, 0.378319),
        vec2(-0.886362, 0.032122),
        vec2(-0.466531,-0.741458),
        vec2( 0.006773,-0.574796),
        vec2(-0.739828,-0.410584),
        vec2( 0.590785,-0.697557),
        vec2(-0.081436,-0.963262),
        vec2( 1.000000,-0.100160),
        vec2( 0.622430, 0.680868)
    };

    constexpr inline vec2 poissonDisk(size_t index) { return poissonDiskData[index % N]; }
    
    static constexpr float r_RAND_MAX = 1.0f / static_cast<float>(RAND_MAX);
    inline float rand() {
        return static_cast<float>(std::rand()) * r_RAND_MAX;
    }   
}