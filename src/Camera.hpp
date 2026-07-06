#pragma once
#include "stn.hpp"

#include "Texture.hpp"
#include "Scene.hpp"
#include "Ray.hpp"

#include "util/MatrixStack.hpp"
#include "util/RowQueue.hpp"

class Camera {
public:
    // I could make these modifible via json files
    static constexpr float EPSILION = 5E-3f; // having a low epsilion value does not bode well with meshes
    static constexpr float MAX_DIST = std::numeric_limits<float>::max();
    static constexpr uint MAX_RECURSIONS = 7;

    static constexpr float MINIMUM_COEFF = 0.005f;
    static constexpr bool FULL_SHADOWS = false;

    Camera() : translation(0.f), rotation(0.f),
               position(0.f), lookAtPos(0.f), camUpVec{0.f, 1.f, 0.f},
               aspectRatio(1.0), fovy(glm::radians(45.0)), 
               width(1), height(1) { }
    Camera(uint w, uint h) : Camera() {
        this->width = w; this->height = h;
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

    virtual ~Camera() = default;

    void applyProjection(MatrixStack&);
    void applyView(MatrixStack&);

    void setAspect(double a) { aspectRatio = a; }
    void setFOV(double FOVdeg) { fovy = glm::radians(FOVdeg); }

    // Provided for compatibility; don't use these
    void setInitDistance(double dist) { translation.z = -std::abs(dist); }
    void setTranslation(const glm::vec3 &pos) { translation = pos; }
    void setRotation(const glm::vec3 &rot) { rotation = rot; }

    void setCameraPos(const glm::vec3 &pos) { position = pos; }
    void setLookAtPos(const glm::vec3 &pos) { lookAtPos = pos; }
    // As a failsafe, make the up vector face in the +y axis
    // if the magnitude of upVec is 0.
    void setUpVector(const glm::vec3 &upVec) { 
        if (glm::length(upVec) < CONSTANTS::EPSILION) {
            std::cerr << "Provided up vector has a length near zero\n";
            camUpVec = glm::normalize(upVec+vec3(0.f, EPSILION, 0.f)); 
        } else { camUpVec = glm::normalize(upVec); }
    }

    // Setting samples below 2 disables antialiasing.
    void setAntialiasSamples(uint count) { AAsamples = count > 1 ? count : 1; }
    // Ambient occlusion samples are taken for every color ray, including
    // anti-aliasing rays; recommended to reduce AO samples if increasing AA rays
    void setAmbientOcclusionSamples(uint count) { occlusionSamples = count > 0 ? count : 0; }
    void setGlobalAmbientColor(const glm::vec3 &clr) { globalAmbient = clr; }
    void setAmbientOccludingRadius(float r) { occludingRadius = r; }

    enum class SkyType {Void, Haze, SphereMap, Ambient};
    void setSky(SkyType s) { sky = s; }
    void setSkyTexture(std::unique_ptr<ImageTexture>&& texture) { skyTexture = std::move(texture); }

    std::unique_ptr<Image> render(std::unique_ptr<Scene>&, const glm::mat4&, const glm::mat4&);
    void setRow(const std::unique_ptr<Scene> &scene, std::unique_ptr<Image> &image, uint y);
    void processRows(const std::unique_ptr<Scene> &scene, std::unique_ptr<Image> &image);

private:
    RowQueue r_queue;

    // Relative translation, which is indirectly used in computing cameraPos
    glm::vec3 translation;
    // Rotate the scene around the origin by angles specified in each axis in radians.
    glm::vec3 rotation; 

    glm::vec3 position;    // Position of the camera in world-space
    glm::vec3 lookAtPos;   // position the camera looks at in world-space      
    glm::vec3 camUpVec;    // Up vector of the camera. 

    double aspectRatio;
    double fovy;                // radians; determines how much camera sees
    double znear = 1.0f;        // focal length
    double zfar = 1000.0f;
    double focalAngle = 0.f;    // variation angle in radians; 0 means no DoF
    uint width, height;

    uint AAsamples = 1;                        // must be at least 1
    uint occlusionSamples = 0;                 // actual count is divided by AA samples
    float occludingRadius = 0.25f;             // radius of occluding hemisphere
    glm::vec3 globalAmbient = glm::vec3(0.0f); // ambient color added to all objects.
    SkyType sky = SkyType::Ambient;
    std::unique_ptr<ImageTexture> skyTexture;

    // variables computed in render()
    glm::vec4 cameraPos; // contains world-space position of camera
    glm::mat4 C;         // Camera Matrix
    glm::mat4 invP;      // inverse of projection mat
    glm::vec3 dof_u;
    glm::vec3 dof_v;
    float sample_scale;

    // Contains common info used for BRDF calculations
    struct IntParams {
        const std::unique_ptr<Scene> &scene;
        const Interval &interval;
        const Hit &rec;
    };

    glm::vec3 getRayColor(const std::unique_ptr<Scene> &scene, const Ray &ray, 
                          const Interval &interval = Interval(EPSILION, MAX_DIST), 
                          uint recursiveDepth = 0) const;
    
    glm::vec3 getReflectionColor(const Ray &ray, IntParams args, uint recursions) const;
    
    glm::vec3 getRefractedColor(const Ray &ray, IntParams args, uint recursions, 
                                bool back_face) const;

    glm::vec3 getSkyColor(const Ray &ray) const;
    
    Ray castPrimaryRay(uint idx, uint idy, float offsetx = 0.5f, float offsety = 0.5f) const;

    float occlusionDiffuseFactor(IntParams args, glm::vec3 &diffuseAtten, float time) const;

    static glm::vec3 getShadowContrib(std::vector<Hit> &srecs, const Ray &sray,
                                      const std::unique_ptr<Scene> &scene, 
                                      const Interval &t_int);
    static glm::vec3 lightingFactor(const Ray &ray, IntParams args, 
                                    const std::shared_ptr<Light> &light, 
                                    const glm::vec3 &diffuseAtt = glm::vec3(1.f),
                                    bool sampleArea = true);
};