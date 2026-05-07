#include "stn.h"

#include "Image.h"
#include "MatrixStack.h"
#include "Scene.h"

class Camera {
public:
    Camera() : fovy{glm::radians(90.0)}, width{1}, height{1} {
        this->aspectRatio = 1.0;
    }
    // fov is in degrees
    Camera(uint w, uint h, double fov) : fovy{glm::radians(fov)}, width{w}, height{h} {
        this->aspectRatio = (double)width/(double)height;
    }
    Camera(uint resolution, double aspect) : Camera() {
        assert(aspect > 0.0);

        this->aspectRatio = aspect;
        this->width = std::sqrt(resolution*aspect);
        this->height = std::sqrt(resolution*(1.0/aspect));
    }

    void applyProjection(shared_ptr<MatrixStack>);
    void applyView(shared_ptr<MatrixStack>);

    std::shared_ptr<Image> render(std::shared_ptr<Scene>, const glm::mat4&, const glm::mat4&);

private:
    glm::vec3 translation;
    glm::vec3 rotation;

    double aspectRatio;
    double fovy; // radians
    double znear, zfar;
    uint width, height;
};