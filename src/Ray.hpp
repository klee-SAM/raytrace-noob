#pragma once
#ifndef RAY_H
#define RAY_H

#include "stn.hpp"
#include "Material.hpp"

#include <array>
#include <vector>

class Ray {    
public:
    glm::vec4 pos;
    glm::vec4 dir;
    // glm::vec4 clr; // Stores the color from a previous computation
    float time;

    // Ensures ray invariants
    Ray(const glm::vec4 &pos, const glm::vec4 &dir, float tm) noexcept
    : pos{pos}, dir{dir}, time(tm) {
        this->pos.w = 1.0f; this->dir.w = 0.0f; }

    Ray(const glm::vec3 &pos, const glm::vec3 &dir, float tm = 0.f) noexcept
    : Ray(glm::vec4(pos, 1.f), glm::vec4(dir, 0.f), tm) {}

    Ray() noexcept : Ray(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(0.0f), 0.f) {}

    virtual ~Ray() = default;

    // Getter functions when no homogenous coordinate required
    constexpr glm::vec3 getPos() const { return glm::vec3(pos); }
    constexpr glm::vec3 getDir() const { return glm::vec3(dir); }

    // Setter function for vec3 to vec4
    inline void setPos(const glm::vec3& p) { pos = glm::vec4(p, 1.0f); }
    inline void setDir(const glm::vec3& d) { dir = glm::vec4(d, 0.0f); }  
};

class Hit {
public:
    glm::vec3 x;            // hit location
    glm::vec3 n;            // hit normal
    glm::vec2 uv;           // texture uv coord
    Material* m = nullptr;  // material of hit surface
    float t;                // dist from origin to hit

    // Define getters for lighting components here instead of 
    // in the material class to avoid needing extra parameters
    // However, these can segfault if m isn't checked for nullptr
    inline glm::vec3 ambient() const { return m != nullptr ? m->ambient->value(uv, x) : glm::vec3(0.f); }
    inline glm::vec3 diffuse() const { return m != nullptr ? m->diffuse->value(uv, x) : glm::vec3(0.f); }
    inline glm::vec3 specular() const { return m != nullptr ? m->specular->value(uv, x) : glm::vec3(0.f); }
    inline glm::vec3 emissive() const { return m != nullptr ? m->emissive->value(uv, x) : glm::vec3(0.f); }
    inline glm::vec3 absorb() const { return m != nullptr ? m->absorb->value(uv, x) : glm::vec3(0.f); }
};

// i have been induced with micro-optimization madness
class HitArray {
private:
    static constexpr size_t N = 32UL; // hardcoded capacity
    std::array<Hit, N> arr;           // "raw" data
    size_t indEnd = 0UL;              // size; index to last non-default hit
public:
    constexpr const Hit& at(size_t i) const noexcept { return arr[i % N]; }
    constexpr const Hit& operator[](size_t i) const { return arr[i]; }

    constexpr Hit* begin() noexcept { return arr.begin(); }
    constexpr Hit* end() noexcept { return arr.begin() + indEnd; }
    constexpr size_t max_size() const noexcept { return N; }
    constexpr size_t size() const noexcept { return indEnd; }
    constexpr bool empty() const noexcept {return indEnd == 0; }
    constexpr void clear() noexcept { indEnd = 0; }

    // Does nothing if the array is already full.
    constexpr void push_back(const Hit& a) { 
        if (indEnd >= N) return;
        arr.at(indEnd++) = a;
    }

    inline void sort() {
        static constexpr auto cmp = [](const Hit& a, const Hit& b) { return a.t < b.t; };
        std::sort(begin(), end(), cmp); 
    }
};

#endif