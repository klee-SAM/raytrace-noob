#pragma once
#include "stn.hpp"

#include <fstream>
#include <sstream>

#include "Camera.hpp"
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
                int j = 1; // the offset to the next key token
                for (int keys = 0; keys < tokens[i].size; ++keys) {
                    if (jsonstreq(&tokens[i+j], "distance")) {
                        float initDist = floatFromToken(&tokens[i+j+1]);
                        std::clog << initDist << '\n';
                        cam->setInitDistance(initDist);  
                    } else if (jsonstreq(&tokens[i+j], "antialias")) {
                        
                    } else if (jsonstreq(&tokens[i+j], "fov")) {
                        float fov = floatFromToken(&tokens[i+j+1]);
                        std::clog << fov << '\n';
                    }

                    // if the value-token of distance is not a primitive,
                    // increment the current token pointer past the object's
                    // or array's tokens
                    j += 1 + tokens[i+j].size;
                }
            } else if (jsonstreq(&tokens[i], "materials")) {
            //     printf("\"materials\": %.*s\n", tokens[i + 1].end - tokens[i + 1].start, 
            //            jsonData.c_str() + tokens[i + 1].start);
            } else if (jsonstreq(&tokens[i], "lights")) {
            //     printf("\"lights\": %.*s\n", tokens[i + 1].end - tokens[i + 1].start, 
            //            jsonData.c_str() + tokens[i + 1].start);
            } else if (jsonstreq(&tokens[i], "shapes")) {
            //     printf("\"shapes\": %.*s\n", tokens[i + 1].end - tokens[i + 1].start, 
            //            jsonData.c_str() + tokens[i + 1].start);
            } else {
                // printf("\"man\": %.*s\n", tokens[i].end - tokens[i].start, 
                    //    jsonData.c_str() + tokens[i].start);
            }
        }
    }

private:
    std::string location;
    std::ifstream file;

    std::string jsonData;

    void skipNestedTokens() {

    }

    bool jsonstreq(jsmntok_t* tok, const std::string& str) 
    {
        size_t tok_len = tok->end - tok->start;
        if (tok->type == JSMN_STRING && str.length() == tok_len && 
            strncmp(&jsonData.at(tok->start), str.c_str(), tok_len) == 0) {
            return true;
        }
        return false;
    }

    float floatFromToken(jsmntok_t* tok) {
        if (tok->type == JSMN_PRIMITIVE && charIsNumeric(tok->start)) {
            return strtod(&jsonData.at(tok->start), nullptr);
        }
        return 0.0f; // bogus
    }

    bool charIsNumeric(int offset) {
        for (char c = '0'; c != '9'+1; c++) {
            if (jsonData.at(offset) == c) return true;
        }
        if (jsonData.at(offset) == '-') return true;
        return false;
    }
};