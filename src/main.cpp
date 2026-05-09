#include "stn.hpp"

#include "Camera.hpp"
#include "Image.hpp"
#include "Material.hpp"
#include "Scene.hpp"
#include "Texture.hpp"

using namespace std;

const string RESOURCE_DIR = "../data/";

void parseSceneFile(const string& inputFile) {

}

void createTestScene() {

}

int main(int argc, char** argv) {
    if (argc < 5) {
        clog << "Usage: ./prog sceneFile width height outputFile";
    }

    
    return 0;
}