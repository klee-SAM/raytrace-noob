#pragma once
#include "stn.hpp"
#include "Shape.hpp"
#include "Material.hpp"

class Light {
public:
    glm::vec3 pos;
    double intensity;
    Light() : pos{glm::vec3(0.0f)}, intensity(1.0) {}
    Light(glm::vec3 p, double i) : pos{p}, intensity(i) {} 
    virtual ~Light() = default;
};

// Should include functions for building acceleration structures
class Scene {
public:
    std::vector<std::shared_ptr<Shape>> shapes;
    std::vector<std::shared_ptr<Light>> lights;

    void pushShape(std::shared_ptr<Shape> s) { shapes.push_back(s); }
    void pushLight(std::shared_ptr<Light> l) { lights.push_back(l); }
};