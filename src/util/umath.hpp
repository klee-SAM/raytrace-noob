#pragma once
#include "../stn.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <limits>

// Utility math functions and classes

namespace CONSTANTS {
    constexpr float PI = 3.14159265358979323846f;
    constexpr float R_PI = 1.0f / PI;
    static const float R_SQRT3 = 1.f / std::sqrt(3.f);

    constexpr float INF = std::numeric_limits<float>::infinity();
    constexpr float EPSILION = std::numeric_limits<float>::epsilon();
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

class Interval {
public:
    float min, max;

    Interval() : min(CONSTANTS::INF), max(-CONSTANTS::INF) {} // empty
    Interval(float min, float max) : min(min), max(max) {}

    float size() const { return max - min; }
    bool contains(float x) const { return (min <= x) && (x <= max); }
    bool surrounds(float x) const { return (x < min) && (max < x);}

    static inline Interval empty() { return Interval(CONSTANTS::INF, -CONSTANTS::INF); }
    static inline Interval world() { return Interval(-CONSTANTS::INF, CONSTANTS::INF); }
    static inline Interval signif() { return Interval(CONSTANTS::EPSILION, 1.f - CONSTANTS::EPSILION); }
};

// Returns the number without the integer part (-2.6 -> -0.6)
constexpr float decimal(float x) { return x - static_cast<int>(x); }
// Like decimal, but returns a positive number instead
constexpr float fract(float x) { return x - std::floor(x); }
// https://stackoverflow.com/questions/1903954/: -1, 0, or 1
constexpr int sgn(float val) { return (0.0f < val) - (val < 0.0f); }
constexpr bool is_pow2(int n) { return n == 0 || (n & (n - 1)) == 0; }

// orthonormal basis (TBN matrix)
inline void assignONBvec3s(const glm::vec3& n, glm::vec3& b1, glm::vec3& b2) 
{   // thank you Duff et al
    const float sign = copysignf(1.0f, n.z); // sign should be 1 even when n.z == 0
    const float a = -1.0f / (sign + n.z);
    const float b = n.x * n.y * a;
    b1 = glm::vec3(1.0f + sign * n.x * n.x * a, sign * b, -sign * n.x);
    b2 = glm::vec3(b, sign + n.y * n.y * a, -n.y);
}

// This implements importance sampling.
// u1 and u2 are random uniform variables w/ range [0, 1)
inline glm::vec3 cosineSampleHemisphere(float u1, float u2) 
{   // thank you rory
    const float r = sqrt(u1);
    const float theta = 2 * CONSTANTS::PI * u2;
    const float x = r*cos(theta);
    const float y = r*sin(theta);
    // Modification: assume u1 can never go above 1.0f
    return glm::vec3(x, y, sqrt(1.0f - u1));
}

// https://stackoverflow.com/questions/42537957/fast-accurate-atan-arctan-approximation-algorithm
static constexpr float atan_jw(float x) {
  return 8 * x / (3 + std::sqrt(25 + 80.0f / 3.0f * x * x));
}

// https://mazzo.li/posts/vectorized-atan2.html (auto 4)
constexpr float f_atan2(float y, float x) {
    using namespace CONSTANTS;
    const bool swap = std::fabs(x) < std::fabs(y);
    const float atan_inp = (swap ? x : y) / (swap ? y : x);
    float res = atan_jw(atan_inp);
    res = swap ? sgn(atan_inp)*.5f*PI - res : res;
    if (x < 0.f) res += sgn(y)*PI; // adjust res based on input quadrant
    return res;
}

// Uses an identity of atan
constexpr float f_asin(float x) {
    return atan_jw(x / std::sqrt(1.f - x*x));
}

// https://en.wikipedia.org/wiki/UV_mapping#Finding_UV_on_a_sphere
constexpr glm::vec2 sphereMap(const glm::vec3 &p) {
    using namespace CONSTANTS;
    const float u = 0.5f + f_atan2(p.z, p.x)*R_PI*0.5f;
    const float v = 0.5f + f_asin(p.y)*R_PI;
    return vec2(u, v);
}