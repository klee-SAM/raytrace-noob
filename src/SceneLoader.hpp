#pragma once
#include "stn.hpp"

#include <fstream>
#include <sstream>

#include "Camera.hpp"

/* Changed JSMN_STATIC to use inline instead of static; this is a
dirty attempt at hiding linker errors and compiler warnings */
#define JSMN_INLINE 
#define JSMN_STRICT 
#include "jsmn.h" 

// if I had more time I should probably write my own json parser instead
// but thank you zserge

class SceneLoader {
public:
    SceneLoader() : location{""} {}
    SceneLoader(const std::string& loc) : location{loc} {}

    void setFileLocation(const std::string& loc) { location = loc; }

    void loadSceneFile(std::shared_ptr<Camera>& cam, std::shared_ptr<Scene>& scene);

private:
    std::string location;
    std::ifstream file;

    std::string jsonData;
    
    // To get to the next token of the same level, add the number of nested
    // tokens plus 1 to account for itself (tok)
    int offsetToNextKey(const jsmntok_t* tok) { return 1 + tok->size; }
    std::string print_token(const jsmntok_t* tok) {
        return jsonData.substr(tok->start, tok->end - tok->start);
    }

    int parseCameraProperties(const jsmntok_t* obj_tok, std::shared_ptr<Camera>& cam);
    int parseLights(const jsmntok_t* arr_tok, std::shared_ptr<Scene>& scene);
    int parseMaterials(const jsmntok_t* obj_tok, std::shared_ptr<Scene>& scene);
    int parseShapes(const jsmntok_t* arr_tok, std::shared_ptr<Scene>& scene);

    bool jsonstreq(const jsmntok_t* tok, const std::string& str);

    enum class PRIMITIVE_TYPE {BOOL, NUL, NUM, NONE};
    using PRIM = SceneLoader::PRIMITIVE_TYPE;
    PRIMITIVE_TYPE typeOfPrimitiveAt(int offset);
    bool charIsNumeric(int offset);

    int intFromToken(const jsmntok_t* tok);
    bool boolFromToken(const jsmntok_t* tok);
    double doubleFromToken(const jsmntok_t* tok);
    glm::vec3 float3FromToken(const jsmntok_t* tok);
    std::string stringFromToken(const jsmntok_t* tok);
};