#include "SceneLoader.hpp"

#include "umath.hpp"

using namespace std;
using namespace glm;

/*
 * TODO: documentation
 */

const bool VERBOSE = false;

std::shared_ptr<Shape> shapeFromString(const std::string& type);

void SceneLoader::loadSceneFile(std::unique_ptr<Camera>& cam, std::unique_ptr<Scene>& scene) 
{
    if (scene == nullptr || cam == nullptr) {
        throw std::logic_error("loadSceneFile: One or more arguments are null pointers");
    }
    if (location.empty()) { 
        std::cerr << "No input file given\n"; 
        return; 
    }

    file.open(location);

    if (!file.is_open()) {
        std::cerr << "Couldn't open file " << location << '\n';
        return;
    } else {
        // not great, this is a hacky way to access the entire
        // json data as a string, which is not great for large files
        jsonData = (std::stringstream() << file.rdbuf()).str();
    }
    file.close();

    const uint tok_size = 2048U;

    jsmn_parser parser;
    jsmntok_t tokens[tok_size];
    jsmn_init(&parser);

    // r is either an error code or the number of actual tokens used
    int r = jsmn_parse(&parser, jsonData.c_str(), jsonData.length(), tokens, tok_size);

    /*
    JSMN_ERROR_INVAL - bad token, JSON string is corrupted
    JSMN_ERROR_NOMEM - not enough tokens, JSON string is too large
    JSMN_ERROR_PART - JSON string is too short, expecting more JSON data
    */
    if (r < 0) {
        std::cerr << "Failed to parse JSON: " << r << '\n';
        switch(r) {
        case JSMN_ERROR_INVAL:
            std::cerr << "corrupted string\n";
            break;
        case JSMN_ERROR_NOMEM:
            std::cerr << "string is too large\n";
            break;
        case JSMN_ERROR_PART:
            std::cerr << "expected more data\n";
            break;
        default:
            // gcc might complain w/o this
            break;
        }
        return;
    }

    // Assume the top-level element is an object
    if (r < 1 || tokens[0].type != JSMN_OBJECT) {
        std::cerr << "Object expected\n";
        return;
    }

    int ignored_tokens = 0;

    // All tokens are parsed in order
    for (int i = 1, prev_i = 1; i < r; prev_i = i) {
        if (jsonstreq(&tokens[i], "camera")) {
            // i now refers to the object-value of the "camera" key
            if (tokens[++i].type != JSMN_OBJECT) continue;
            i += parseCameraProperties(&tokens[i], cam);
            
        } else if (jsonstreq(&tokens[i], "materials")) {
            if (tokens[++i].type != JSMN_OBJECT) continue;
            i += parseMaterials(&tokens[i], scene);

        } else if (jsonstreq(&tokens[i], "lights")) {
            if (tokens[++i].type != JSMN_ARRAY) continue;
            i += parseLights(&tokens[i], scene);
            
        } else if (jsonstreq(&tokens[i], "shapes")) {
            if (tokens[++i].type != JSMN_ARRAY) continue;
            i += parseShapes(&tokens[i], scene);

        } else {
            if (VERBOSE)
                printf("\"Unrecognized token\": %.*s\n", 
                    tokens[i].end - tokens[i].start, 
                    jsonData.c_str() + tokens[i].start);
            ++ignored_tokens;
            ++i;
        }

        // i should be incremented in all branches
        if (prev_i >= i) {
            throw std::logic_error("Infinite loop detected.");
        }
    }

    if (ignored_tokens > 0) {
        std::clog << ignored_tokens << " were unused.\n";
    }
}

// Section parsing
// these functions return the offset from the provided token to the next unnested token

// repeating logic here; could create new classes to abstract away the details
// of reading object and array tokens

int SceneLoader::parseCameraProperties(const jsmntok_t* obj_tok, std::unique_ptr<Camera>& cam)
{
    int j = 1; // the offset to the next key token
    // this pointer arithmetic is not very safe
    for (int prop = 0; prop < obj_tok->size; ++prop) {
        auto key = obj_tok+j, value = key+1;
        if (jsonstreq(key, "distance")) {
            double initDist = doubleFromToken(value);
            cam->setInitDistance(initDist);  
        } else if (jsonstreq(key, "antialias")) {
            int samples = intFromToken(value);
            samples = samples < 0 ? 0 : samples;
            cam->setAntialiasSamples((uint)samples);
        } else if (jsonstreq(key, "fov")) {
            float fov = doubleFromToken(value);
            cam->setFOV(fov);
        } else if (jsonstreq(key, "sky")) {
            if (jsonstreq(value, "haze")) {
                cam->setSky(Camera::SkyType::Haze);
            } else if (!jsonstreq(value, "void")) {
                std::cerr << "Unknown sky type: " << print_token(value) << '\n';  
            }
        } else if (jsonstreq(key, "rotation")) {
            // [yaw, pitch, roll]
            vec3 rot = float3FromToken(value);
            cam->setRotation(rot);
        }

        // if the value-token is not a primitive,
        // increment the current token pointer past the object's
        // or array's tokens
        j += 1 + offsetToNextKey(value);
    }
    // offset to next object (camera, materials, etc.)
    return j;
}

int SceneLoader::parseLights(const jsmntok_t* arr_tok, std::unique_ptr<Scene>& scene)
{
    int j = 1;
    for (int item = 0; item < arr_tok->size; ++item) {
        // skip any items that do not contain light properties
        const jsmntok_t* l_tok = arr_tok+j; // this should be an object token
        if (l_tok->type != JSMN_OBJECT) {
            j += offsetToNextKey(l_tok);
            continue;
        }

        std::shared_ptr<Light> light = std::make_shared<Light>();
        int k = 1; // start after the object token
        for (int prop = 0; prop < (arr_tok+j)->size; ++prop) {
            const jsmntok_t *key = l_tok+k, *value = l_tok+k+1;
            if (jsonstreq(key, "position") && value->type == JSMN_ARRAY) {
                light->pos = float3FromToken(value);
            } else if (jsonstreq(key, "intensity") && 
                        value->type == JSMN_PRIMITIVE) {
                light->intensity = doubleFromToken(value);
            } else {
                std::cerr << "invalid property when parsing lights\n";
            }
            // Add another 1 to account for the key
            k += 1 + offsetToNextKey(value);
        }
        j += k;    
        scene->pushLight(light);
    }



    return j;
}

int SceneLoader::parseMaterials(const jsmntok_t* obj_tok, std::unique_ptr<Scene>& scene) 
{
    assert(obj_tok->type == JSMN_OBJECT);

    int j = 1;
    for (int item = 0; item < obj_tok->size; ++item) {
        const jsmntok_t* mat_name_tok = obj_tok + j;
        const jsmntok_t* mat_prop_tok = mat_name_tok + 1;

        if (mat_name_tok->type != JSMN_STRING) {
            j += 1 + offsetToNextKey(mat_prop_tok);
            continue;
        }

        std::shared_ptr<Material> material = std::make_shared<Material>();

        int prop_ind = 1;
        for (int prop = 0; prop < mat_prop_tok->size; ++prop) {
            auto key = mat_prop_tok+prop_ind, value = key + 1;

            // alternate names for the same property
            auto isReflectProp = [&key, this]() {
                return jsonstreq(key, "reflCoeff") ||
                       jsonstreq(key, "reflectance");
            };

            auto isRefractProp = [&key, this]() {
                return jsonstreq(key, "refrIndex") ||
                       jsonstreq(key, "refractionIndex");
            };

            if (jsonstreq(key, "ambient")) {
                material->ambient = float3FromToken(value);
            } else if (jsonstreq(key, "diffuse")) {
                material->diffuse = float3FromToken(value);
            } else if (jsonstreq(key, "specular")) {
                material->specular = float3FromToken(value);
            } else if (jsonstreq(key, "exponent")) {
                material->exponent = doubleFromToken(value);
            } else if (isReflectProp()) {
                material->reflCoeff = doubleFromToken(value);
            } else if (isRefractProp()) {
                material->refrIndex = doubleFromToken(value);
            } else if (jsonstreq(key, "transparency")) {
                material->transparency = doubleFromToken(value);
            }
            
            prop_ind += 1 + offsetToNextKey(value); 
        }
        scene->writeMaterial(stringFromToken(mat_name_tok), material);
        j += prop_ind + 1; // add 1 to account for mat name 
    }
    return j;
}

int SceneLoader::parseShapes(const jsmntok_t* arr_tok, std::unique_ptr<Scene>& scene) 
{
    ModelMatConstr modelMat;

    int j = 1;
    for (int item = 0; item < arr_tok->size; ++item) {
        const jsmntok_t* s_tok = arr_tok+j; // should be an object token
        if (s_tok->type != JSMN_OBJECT) {
            j += s_tok->size;
            continue;
        }

        ShapeProperties prop;
        std::shared_ptr<Shape> shape;

        int prop_ind = 1;
        for (int p = 0; p < s_tok->size; ++p) {
            auto key = s_tok + prop_ind, value = key+1;

            if (jsonstreq(key, "shape")) {
                shape = shapeFromString(stringFromToken(value));
            } else if (jsonstreq(key, "position")) {
                prop.pos = float3FromToken(value);
            } else if (jsonstreq(key, "scale")) {
                prop.scl = float3FromToken(value);
            } else if (jsonstreq(key, "rotation")) {
                prop.rot = float3FromToken(value);
                prop.rot.x = glm::radians(prop.rot.x);
                prop.rot.y = glm::radians(prop.rot.y);
                prop.rot.z = glm::radians(prop.rot.z);
            } else if (jsonstreq(key, "material")) {
                prop.smat = scene->getMaterial(stringFromToken(value));
            } else if (jsonstreq(key, "file")) {
                prop.mesh_filename = stringFromToken(value);
            } else {
                std::cerr << "parseShapes: invalid property: "
                          << print_token(key) << '\n';
            }

            prop_ind += 1 + offsetToNextKey(value);
        }

        if (shape != nullptr) {
            prop.applyProperties(shape, modelMat, this->srcDir);
            scene->pushShape(shape);
        } else {
            std::cerr << "Shape not created.\n";
        }

        j += prop_ind;
    }
    return j;
}


void SceneLoader::ShapeProperties::applyProperties(
    shared_ptr<Shape>& shape,
    ModelMatConstr& modelMat, 
    const std::string& srcDir) // not nice
{
    modelMat.setPosition(pos);
    modelMat.setRotation(rot);
    modelMat.setScale(scl);
    
    // Hack to initialize a mesh object
    bool isMesh = dynamic_cast<Mesh*>(shape.get()) != nullptr;
    if (isMesh) { shape = make_shared<Mesh>(mesh_filename, srcDir); }

    shape->setModelMatrix(modelMat.getMatrix());
    shape->setMaterial(smat);
};

// For use in recursively parsing shapes
int SceneLoader::parseShape(const jsmntok_t* obj_tok, unique_ptr<Shape>& parentShape) 
{
    if (obj_tok->type != JSMN_OBJECT) {
        std::cerr << "parseShape(): token is of type "
                  << obj_tok->type << '\n'; 
        return 0;
    }
    // TODO
    return 0;
}

// Misc. helper functions

SceneLoader::PRIM SceneLoader::typeOfPrimitiveAt(int offset) 
{
    // to be determined
    return PRIM::NONE;
}

bool SceneLoader::charIsNumeric(int offset)
{
    // Relies on ASCII orderings of characters
    if (jsonData.at(offset) == '-') return true;
    for (char c = '0'; c <= '9'; c++) {
        if (jsonData.at(offset) == c) return true;
    }
    return false;
}

bool SceneLoader::jsonstreq(const jsmntok_t* tok, const std::string& str) 
{
    size_t tok_len = tok->end - tok->start;
    if (tok->type == JSMN_STRING && str.length() == tok_len && 
        strncmp(&jsonData.at(tok->start), str.c_str(), tok_len) == 0) {
        return true;
    }
    return false;
}

std::shared_ptr<Shape> shapeFromString(const std::string& type) 
{
    if (type == "sphere") {
        return make_shared<Sphere>(); 
    } else if (type == "plane") {
        return make_shared<Plane>();
    } else if (type == "mesh") { 
        return make_shared<Mesh>();
    } else if (type == "box") {
        return make_shared<Box>();
    } else if (type == "cylinder") {
        return make_shared<Cylinder>();
    } else {
        std::cerr << "shapeFromString: unrecognized shape type:"
                  << type << '\n';
        return nullptr;
    }
}

// Token conversion

glm::vec3 SceneLoader::float3FromToken(const jsmntok_t* tok)
{
    if (tok->type != JSMN_ARRAY) {
        std::cerr << "float3FromToken: token is not an array - "
                    << tok->type << '\n';
        return glm::vec3(0.0f);
    } else if (tok->size < 3) {
        std::cerr << "float3FromToken: Size of array must be >= 3\n";
        return glm::vec3(0.0f);
    };
    glm::vec3 v;
    v.x = (float) doubleFromToken(tok+1);
    v.y = (float) doubleFromToken(tok+2);
    v.z = (float) doubleFromToken(tok+3);
    return v;
}

double SceneLoader::doubleFromToken(const jsmntok_t* tok)
 {
    if (tok->type == JSMN_PRIMITIVE && charIsNumeric(tok->start)) {
        return std::strtod(&jsonData.at(tok->start), nullptr);
    }
    return 0.0; // bogus
}

int SceneLoader::intFromToken(const jsmntok_t* tok)
{
    if (tok->type == JSMN_PRIMITIVE && charIsNumeric(tok->start)) {
        return (int) std::strtol(&jsonData.at(tok->start), nullptr, 10);
    }
    return 0;
}

bool SceneLoader::boolFromToken(const jsmntok_t* tok)
{
    if (tok->type == JSMN_PRIMITIVE) {
        if (jsonData.at(tok->start) == 't') return true;
        if (jsonData.at(tok->start) == 'f') return false;
    }
    std::cerr << "boolFromToken: unrecognized value, returning false";
    return false;
}

std::string SceneLoader::stringFromToken(const jsmntok_t* tok)
{
    if (tok->type != JSMN_STRING) {
        std::cerr << "stringFromToken: expected token of type JSMN_STRING = 4, got"
                  << tok->type << "instead.\n";
        throw std::runtime_error("");
    }
    return print_token(tok);
}