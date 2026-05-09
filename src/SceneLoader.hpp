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

        std::string jsonData;

        if (!file.is_open()) {
            std::cerr << "Couldn't open file " << location << '\n';
            return;
        } else {
            // not great, this is a hacky way to access the entire
            // json data as a string, which is not great for large files
            jsonData = (std::stringstream() << file.rdbuf()).str();
        }
        file.close();

        jsmn_parser parser;
        jsmntok_t tokens[1024];
        jsmn_init(&parser);

        int r = jsmn_parse(&parser, jsonData.c_str(), jsonData.length(), tokens, 1024);

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
        
        // start after the root object
        for (int i = 1; i < r; ++i) {
            
        }
    }

private:
    std::string location;
    std::ifstream file;
};