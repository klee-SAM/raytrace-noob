#pragma once
#include "stn.hpp"
#include "Shape.hpp"
#include "Material.hpp"

class Light {
public:
    glm::vec3 pos;
    float intensity;
    Light() : pos{glm::vec3(0.0f)}, intensity(1.0f), radius(0.0f) {}
    Light(glm::vec3 p, float i) : pos{p}, intensity(i), radius(0.0f) {} 
    Light(glm::vec3 p, float i, float r) : Light(p, i) { setRadius(r); }
    virtual ~Light() = default;

    // arbitrary dynamic formula for area light sampling; max samples 
    // are done when light has a radius of 0.25 or more
    inline void setRadius(float r) {
        radius = r; 
        constexpr int MAX_SAMPLES = 64;
        int calcSamp = MAX_SAMPLES*sqrt(r);
        samples = std::clamp(calcSamp, 1, MAX_SAMPLES);
    }
    inline const float& getRadius() const { return radius; }
    inline const int& getSamples() const { return samples; }
private:
    float radius;
    int samples = 1;
};

// Should include functions for building acceleration structures
class Scene {
public:
    inline const std::vector<std::shared_ptr<Shape>>& getShapes() { return shapes; }

    inline const std::vector<std::shared_ptr<Light>>& getLights() { return lights; }

    // When shapes are pushed, they are finalized (precomputation from transforms)
    inline void pushShape(std::shared_ptr<Shape> s) { 
        s->initialize();
        shapes.push_back(s);
    }

    inline void pushLight(std::shared_ptr<Light> l) { lights.push_back(l); }

    void writeMaterial(const std::string& name, std::shared_ptr<Material> m) {
        // have to modify the object pointed to, rather than the pointer itself
        auto placed = materials.emplace(name, m);
        if (!placed.second) {
            // this is bad. need to emulate a copy constructor somehow
            std::shared_ptr<Material> oth = placed.first->second;
            oth->copy(*m);
        }
    } 
    // Creates a new default material with the given name if it does not 
    // exist beforehand (i.e, when parsing shapes data before material data)
    std::shared_ptr<Material>& getMaterial(const std::string& name) {
        auto placed = materials.try_emplace(name, std::make_shared<Material>());
        return placed.first->second; // return the material from the key-value pair
    }
    
private:
    std::vector<std::shared_ptr<Shape>> shapes;
    std::vector<std::shared_ptr<Light>> lights;
    std::unordered_map<std::string, std::shared_ptr<Material>> materials;
};