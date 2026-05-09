#pragma once
#include "stn.hpp"

#include "Camera.hpp"

#include <fstream>

using namespace std;
using namespace glm;

class Parser {
public:
    Parser() : location{""} {}
    Parser(const string& loc) : location{loc} {}

    void setFileLocation(const string& loc) { location = loc; }

    void parseSceneFile(shared_ptr<Camera>& cam, shared_ptr<Scene>& scene) {
        if (location.empty()) { 
            cerr << "No input file given\n"; 
            return; 
        }

        file.open(location);

        if (!file.is_open()) {
            cerr << "Couldn't open file " << location << '\n';
            return;
        }

        

        // while (!file.eof()) {
        //     if (file.fail()) {
        //         cerr << "Error while reading file " << location << '\n';
        //         return;
        //     }
        //     try {
        //         file >> type;
        //         if (type == "camera") {
                    
        //         } else if (type == "material") {

        //         } else if (type == "light") {
                    
        //         } else if (typeIsShape(type)) {

        //         } else {
        //             cerr << "Unknown type: " << type << '\n';
        //         }
        //     } catch(exception& e) {
        //         cerr << e.what() << '\n';
        //         return;
        //     }
            
        // }
    }

private:
    string location;
    ifstream file;

    // string type;
    // string word;

    // bool extractForNextChar(char c) {
    //     char blockStart;
    //     file >> blockStart;
    //     if (blockStart != c) {
    //         return false;
    //     }
    //     return true;
    // }
    // bool peekForChar(char c) {
    //     int nextChar = file.peek();
    //     if (nextChar == EOF || nextChar == c) return true;
    //     else return false;
    // }

    // bool findBraceStart() {
    //     return extractForNextChar('{');
    // }

    // bool findBraceEnd() {
    //     return peekForChar('}');
    // }

    // void parseCameraProperties(shared_ptr<Camera>& cam) {
    //     if (!findBraceStart()) throw runtime_error("{ not found");
    //     while (!file.eof()) {
    //         if (findBraceEnd) {}
    //         file >> word;
    //         if (word == "distance") {
    //             float dist;
    //             file >> dist;
    //             cam->setInitDistance(dist);
    //         }

    //     }
    // }

    // vector<string> shapes{"sphere", "plane"};
    // bool typeIsShape(const string& type) {
    //     for (auto& cmd : shapes) if (cmd == type) return true;
    //     return false; 
    // }
};