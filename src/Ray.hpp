#pragma once
#include "stn.hpp"
#include "Material.hpp"

// TODO: rewrite ray as a class that uses vec4s 
// instead of vec3s, avoid need to convert
// to and from vec4 for intersect functions

typedef glm::vec3 vec3;
typedef glm::vec4 vec4;
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
    float u;     // texture u coord
    float v;     // texture v coord
    pMaterial m; // material of hit surface

    // Define getters for lighting components here instead of 
    // in the material class to avoid needing extra parameters
    // However, these can segfault if m isn't checked for nullptr
    inline vec3 ambient() const { return m->ambient->value(u, v, x); }
    inline vec3 diffuse() const { return m->diffuse->value(u, v, x); }
    inline vec3 specular() const { return m->specular->value(u, v, x); }
    inline vec3 emissive() const { return m->emissive->value(u, v, x); }
};