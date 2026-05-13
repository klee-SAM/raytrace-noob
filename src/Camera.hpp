#pragma once
#include "stn.hpp"

#include "Image.hpp"
#include "MatrixStack.hpp"
#include "Scene.hpp"
#include "Ray.hpp"
#include "umath.hpp"

class Camera {
public:
    static constexpr double EPSILION = 1E-3;
    static constexpr double MAX_DIST = std::numeric_limits<float>::max();
    static constexpr uint MAX_RECURSIONS = 7;
    static constexpr float MINIMUM_REFL_COEFF = 0.005f;

    Camera() : aspectRatio(1.0), fovy(glm::radians(45.0)), width(1), height(1) { }
    Camera(uint w, uint h) : fovy{glm::radians(45.0)}, width{w}, height{h} {
        this->aspectRatio = (double)width/(double)height;
    }
    // fov is in degrees
    Camera(uint w, uint h, double fov) : Camera(w, h) {
        this->fovy = glm::radians(fov);
        this->aspectRatio = (double)width/(double)height;
    }
    Camera(uint totalPixels, double aspect) : Camera() {
        assert(aspect > 0.0);

        this->aspectRatio = aspect;
        this->width = std::sqrt(totalPixels*aspect);
        this->height = std::sqrt(totalPixels*(1.0/aspect));
    }

    void applyProjection(MatrixStack&);
    void applyView(MatrixStack&);

    void setAspect(double a) { aspectRatio = a; }
    void setFOV(double FOVdeg) { fovy = glm::radians(FOVdeg); }
    void setInitDistance(double dist) { translation.z = -std::abs(dist); }
    void setTranslation(const glm::vec3& pos) { translation = pos; }
    void setRotation(const glm::vec3& rot) { rotation = rot; }

    // Setting samples below 2 disables antialiasing.
    void setAntialiasSamples(uint count) { samples = count > 1 ? count : 1; }

    std::shared_ptr<Image> render(std::shared_ptr<Scene>, const glm::mat4&, const glm::mat4&);

private:
    glm::vec3 translation; // Relative translation, which is indirectly used in computing cameraPos
    glm::vec3 rotation;    // Relative rotation

    double aspectRatio;
    double fovy; // radians
    double znear = 1.0f; 
    double zfar = 1000.0f;
    uint width, height;

    uint samples = 1;

    // variables computed in render()
    glm::vec4 cameraPos; // contains world-space position of camera
    glm::mat4 C;         // Camera Matrix
    glm::mat4 invP;      // inverse of projection mat

    glm::vec3 getRayColor(
        std::shared_ptr<Scene> scene, const Ray& ray, 
        const Interval& interval = Interval(EPSILION, MAX_DIST), 
        uint recursiveDepth = 0);
    
    Ray castPrimaryRay(uint idx, uint idy, double offsetx = 0.5, double offsety = 0.5);
};