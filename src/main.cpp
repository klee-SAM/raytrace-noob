#include "stn.h"

#include "Camera.h"
#include "Image.h"
#include "Material.h"
#include "Scene.h"
#include "Texture.h"

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