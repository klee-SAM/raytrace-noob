#include "stn.h"
#include "Shape.h"
#include "Material.h"

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