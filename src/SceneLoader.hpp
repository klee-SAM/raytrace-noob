#pragma once
#ifndef SCENELOADER_H
#define SCENELOADER_H

#include "stn.hpp"

#include <iosfwd>
#include <string>

#include "Camera.hpp"

/* Changed JSMN_STATIC to use inline instead of static; this is a
dirty attempt at hiding linker errors and compiler warnings */
#define JSMN_INLINE 
#define JSMN_STRICT 
#include "external/jsmn.h" 

// if I had more time I should probably write my own json parser instead
// but thank you zserge

class SceneLoader {
public:
    SceneLoader() : location{""} {}

    // Initialize with the resource directory containing
    // .json, .obj, and .png files.
    SceneLoader(const std::string& loc) : srcDir{loc} {}

    // for scene.json files. set the name of the scene file to load from.
    void setSceneFile(const std::string& name) { location = name; }

    // for all files, must be called before loading scene with meshes or image textures
    void setResourceDirectory(const std::string& loc) { srcDir = loc; }

    void loadSceneFile(std::unique_ptr<Camera>& cam, std::unique_ptr<Scene>& scene);

private:
    std::string location;
    std::string srcDir;

    std::string sceneDir;
    std::string textureDir;
    std::string modelDir;

    std::string jsonData;
    
    // To get to the next token of the same level, add the number of nested
    // tokens plus 1 to account for itself (tok)
    int offsetToNextKey(const jsmntok_t* tok) { return 1 + tok->size; }
    std::string print_token(const jsmntok_t* tok) {
        return jsonData.substr(tok->start, tok->end - tok->start);
    }

    int parseCameraProperties(const jsmntok_t* obj_tok, std::unique_ptr<Camera>& cam);
    int parseLights(const jsmntok_t* arr_tok, std::unique_ptr<Scene>& scene);
    int parseMaterials(const jsmntok_t* obj_tok, std::unique_ptr<Scene>& scene);
    int parseShapes(const jsmntok_t* arr_tok, std::unique_ptr<Scene>& scene);
    int parseShape(const jsmntok_t* obj_tok, std::unique_ptr<Scene>& scene, 
        std::shared_ptr<Shape>& parentShape);
    int parseTexture(const jsmntok_t* obj_tok, std::shared_ptr<Texture>& text);

    enum class SHAPE_TYPE {none, cylinder, sphere, box, plane, mesh, csg, torus};

    class ShapeProperties {
    public:
        SceneLoader& loader;

        SHAPE_TYPE type;
        glm::vec3 pos, scl, rot;
        glm::vec3 npos, nscl, nrot;
         // Should be set to True whenever moving transforms are set
        bool movePos, moveScl, moveRot;
        std::shared_ptr<Material> smat;

        // shape sub-class properties

        // mesh
        std::string mesh_filename;
        std::shared_ptr<MeshBuffer> mesh_buf;

        // csg
        OperationType operationType;
        std::shared_ptr<Shape> left;
        std::shared_ptr<Shape> right;

        ShapeProperties(SceneLoader& l) : loader(l), 
            pos(0.0f), scl(1.0f), rot(0.0f), 
            npos(0.0f), nscl(1.0f), nrot(0.0f),
            movePos(false), moveScl(false), moveRot(false),
            smat(defaultMaterial) {}

        void applyProperties(std::shared_ptr<Shape>& shape);
    };

    std::shared_ptr<Shape> createShape(SHAPE_TYPE type);
    void loadMeshBuffer(const std::string& mesh_filename, 
                        std::unique_ptr<Scene>& scene);

    bool jsonstreq(const jsmntok_t* tok, const std::string& str);

    enum class PRIMITIVE_TYPE {BOOL, NUL, NUM, NONE};
    using PRIM = SceneLoader::PRIMITIVE_TYPE;
    PRIM typeOfPrimitiveAt(int offset);
    bool charIsNumeric(int offset);
    std::pair<bool, int> tryLoadMaterialComps(
        std::shared_ptr<Material>& mat, 
        const jsmntok_t *key, 
        const jsmntok_t *value);

    int intFromToken(const jsmntok_t* tok);
    bool boolFromToken(const jsmntok_t* tok);
    double doubleFromToken(const jsmntok_t* tok);
    glm::vec3 float3FromToken(const jsmntok_t* tok);
    std::string stringFromToken(const jsmntok_t* tok);
    SHAPE_TYPE shapeTypeFromToken(const jsmntok_t* tok);
};

#endif