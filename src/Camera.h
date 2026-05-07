#include "stn.h"

#include "Scene.h"

class Camera {
public:
    Camera() : FOVY{90.0}, width{1}, height{1} {
        this->aspectRatio = (double)width/(double)height;
    }
    Camera(uint w, uint h, double fov) : FOVY{fov}, width{w}, height{h} {
        this->aspectRatio = (double)width/(double)height;
    }
    Camera(uint resolution, double aspect) : Camera() {
        assert(aspect > 0.0);

        this->aspectRatio = aspect;
        this->width = std::sqrt(resolution*aspect);
        this->height = std::sqrt(resolution*(1.0/aspect));
    }

    void applyCameraMatrix() {

    }

private:
    glm::vec3 translation;
    glm::vec3 rotation;

    double aspectRatio;
    double FOVY;
    uint width, height;
};