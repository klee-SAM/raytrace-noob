#pragma once
#ifndef LIGHT_H
#define LIGHT_H

#include "stn.hpp"

class Light {
public:
    static constexpr int MAX_SAMPLES = 256;
    static constexpr int DEFAULT_SAMPLES = 32;

    glm::vec3 pos;
    float intensity;
    Light() : pos{glm::vec3(0.0f)}, intensity(1.0f), radius(0.0f) {}
    Light(glm::vec3 p, float i) : pos{p}, intensity(i), radius(0.0f) {} 
    Light(glm::vec3 p, float i, float r) : Light(p, i) { setRadius(r); }
    virtual ~Light() = default;

    // if the sample count has not been set, set it to the DEFAULT_SAMPLES
    // count b/c it would otherwise be unexpected if setSamples() not called
    inline void setRadius(float r) { 
        radius = r; if (r > 0 && samples < 2) samples = DEFAULT_SAMPLES; 
    }
    inline void setSamples(int samp) { samples = std::clamp(samp, 1, MAX_SAMPLES); }

    inline float getRadius() const { return radius; }
    inline uint getSamples() const { return samples; }
private:
    float radius;
    uint samples = 1U;
};

#endif