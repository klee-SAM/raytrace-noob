#pragma once
#include "stn.hpp"

#include <fstream>
#include <sstream>

#include "Camera.hpp"

#define JSMN_STRICT
#include "jsmn.h" 
// if I had more time I should probably write my own json parser instead
// but thank you zserge

class SceneLoader {
public:
    SceneLoader() : location{""} {}
    SceneLoader(const std::string& loc) : location{loc} {}

    void setFileLocation(const std::string& loc) { location = loc; }

    void loadSceneFile(std::shared_ptr<Camera>& cam, std::shared_ptr<Scene>& scene) {
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

        // All tokens are parsed in order
        for (int i = 1; i < r; ++i) {
            if (jsonstreq(&tokens[i], "camera")) {
                // i now refers to the object-value of the "camera" key
                if (tokens[++i].type != JSMN_OBJECT) continue;
                i += parseCameraProperties(&tokens[i], cam);
                
            } else if (jsonstreq(&tokens[i], "materials")) {
            //     printf("\"materials\": %.*s\n", tokens[i + 1].end - tokens[i + 1].start, 
            //            jsonData.c_str() + tokens[i + 1].start);

            } else if (jsonstreq(&tokens[i], "lights")) {
                if (tokens[++i].type != JSMN_ARRAY) continue;
                i += parseLights(&tokens[i], scene);
                
            } else if (jsonstreq(&tokens[i], "shapes")) {
            //     printf("\"shapes\": %.*s\n", tokens[i + 1].end - tokens[i + 1].start, 
            //            jsonData.c_str() + tokens[i + 1].start);
            } else {
                printf("\"Unrecognized property\": %.*s\n", 
                       tokens[i].end - tokens[i].start, 
                       jsonData.c_str() + tokens[i].start);
            }
        }
    }

private:
    std::string location;
    std::ifstream file;

    std::string jsonData;
    

    int offsetToNextKey(const jsmntok_t* tok) { return 1 + tok->size; }

    // returns the offset from the provided token to the next
    // token not nested in the aforementioned
    int parseCameraProperties(jsmntok_t* obj_tok, std::shared_ptr<Camera>& cam) {
        int j = 1; // the offset to the next key token
        // this pointer arithmetic is not very safe
        for (int key = 0; key < obj_tok->size; ++key) {
            if (jsonstreq(obj_tok+j, "distance")) {
                double initDist = doubleFromToken(obj_tok+j+1);
                cam->setInitDistance(initDist);  
            } else if (jsonstreq(obj_tok+j, "antialias")) {
                int samples = intFromToken(obj_tok+j+1);
                samples = samples < 0 ? 0 : samples;
                cam->setAntialiasSamples((uint)samples);
            } else if (jsonstreq(obj_tok+j, "fov")) {
                float fov = doubleFromToken(obj_tok+j+1);
                cam->setFOV(fov);
            }

            // if the value-token of distance is not a primitive,
            // increment the current token pointer past the object's
            // or array's tokens
            j += offsetToNextKey(obj_tok + j);
        }
        // offset to next object (camera, materials, etc.)
        return j;
    }

    int parseLights(const jsmntok_t* arr_tok, std::shared_ptr<Scene>& scene) {
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
                    std::clog << light->pos.x << '\n';
                } else if (jsonstreq(key, "intensity") && 
                           value->type == JSMN_PRIMITIVE) {
                    light->intensity = doubleFromToken(value);
                    std::clog << light->intensity << '\n';
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

    int parseMaterials(const jsmntok_t* obj_tok, std::shared_ptr<Scene>& scene) {
        int j = 1;
        for (int item = 0;;) {}

        return 0;
    }

    // if shapes are declared before materials, then
    // reference to materials must be emplaced in the map
    // this means that parseMaterials modifies the
    // material associated with a name if it alr exists,
    // and inserts it if it does not
    // or should storing material references be the 
    // scene's responsibility? prob scene 
    int parseShapes() {
        return 0;
    }

    bool jsonstreq(const jsmntok_t* tok, const std::string& str) 
    {
        size_t tok_len = tok->end - tok->start;
        if (tok->type == JSMN_STRING && str.length() == tok_len && 
            strncmp(&jsonData.at(tok->start), str.c_str(), tok_len) == 0) {
            return true;
        }
        return false;
    }

    bool charIsNumeric(int offset) {
        // Relies on ASCII orderings of characters
        for (char c = '0'; c <= '9'; c++) {
            if (jsonData.at(offset) == c) return true;
        }
        if (jsonData.at(offset) == '-') return true;
        return false;
    }

    int intFromToken(const jsmntok_t* tok) {
        if (tok->type == JSMN_PRIMITIVE && charIsNumeric(tok->start)) {
            return (int) std::strtol(&jsonData.at(tok->start), nullptr, 10);
        }
        return 0;
    }

    double doubleFromToken(const jsmntok_t* tok) {
        if (tok->type == JSMN_PRIMITIVE && charIsNumeric(tok->start)) {
            return std::strtod(&jsonData.at(tok->start), nullptr);
        }
        return 0.0; // bogus
    }

    glm::vec3 float3FromToken(const jsmntok_t* tok) {
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
};