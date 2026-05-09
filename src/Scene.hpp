#include "stn.hpp"
#include "Shape.hpp"
#include "Material.hpp"

struct Light {
    glm::vec3 pos;
    double intensity;
};

// Should include functions for building acceleration structures
class Scene {
public:
    std::vector<std::shared_ptr<Shape>> shapes;
    std::vector<std::shared_ptr<Light>> lights;
};