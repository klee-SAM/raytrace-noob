#pragma once
#include "stn.hpp"
#include "Material.hpp"

#include <vector>

typedef std::shared_ptr<Material> pMaterial; 

class Ray {
public:
    vec4 pos;
    vec4 dir;
    vec4 clr; // Stores the color from a previous computation
    float time;

    // Ensures ray invariants
    Ray(const vec4 &pos, const vec4 &dir, float tm) 
     : pos{pos}, dir{dir}, time(tm) {
        this->pos.w = 1.0f; this->dir.w = 0.0f; }

    Ray(const vec3 &pos, const vec3 &dir, float tm = 0.f)
     : Ray(vec4(pos, 1.f), vec4(dir, 0.f), tm) {}

    Ray(const vec3 &pos, const vec3 &dir) : Ray(pos, dir, 0.f) {}
    Ray() : Ray(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(0.0f), 0.f) {}

    virtual ~Ray() = default;

    // Getter functions when no homogenous coordinate required
    inline vec3 getPos() const { return vec3(pos); }
    inline vec3 getDir() const { return vec3(dir); }

    // Setter function for vec3 to vec4
    inline void setPos(const vec3& p) { pos = vec4(p, 1.0f); }
    inline void setDir(const vec3& d) { dir = vec4(d, 0.0f); }  
};

class Hit {
public:
    vec3 x;      // hit location
    vec3 n;      // hit normal
    float t;     // dist from origin to hit
    vec2 uv;     // texture uv coord
    pMaterial m; // material of hit surface

    // Define getters for lighting components here instead of 
    // in the material class to avoid needing extra parameters
    // However, these can segfault if m isn't checked for nullptr
    inline vec3 ambient() const { return m ? m->ambient->value(uv, x) : glm::vec3(0.f); }
    inline vec3 diffuse() const { return m ? m->diffuse->value(uv, x) : glm::vec3(0.f); }
    inline vec3 specular() const { return m ? m->specular->value(uv, x) : glm::vec3(0.f); }
    inline vec3 emissive() const { return m ? m->emissive->value(uv, x) : glm::vec3(0.f); }
    inline vec3 absorb() const { return m ? m->absorb->value(uv, x) : glm::vec3(0.f); }

    static inline void sortHits(std::vector<Hit>& hits) { 
        constexpr auto cmp = [](const Hit& a, const Hit& b) { return a.t < b.t; };
        std::sort(hits.begin(), hits.end(), cmp); 
    }
};