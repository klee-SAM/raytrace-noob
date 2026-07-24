#include "stn.hpp"

#include "util/umath.hpp"
#include "util/MatrixStack.hpp"

#include "Camera.hpp"
#include "Image.hpp"
#include "Material.hpp"
#include "Scene.hpp"
#include "SceneLoader.hpp"
#include "Texture.hpp"

#include <cstdlib>
#include <chrono>
#include <iostream>

#include "tracers/SphereTracer.hpp"

using std::string;
using std::unique_ptr, std::shared_ptr;
using std::make_shared, std::make_unique;
using std::cerr;

using std::vector;

const string RESOURCE_DIR = "../data/";

int main(int argc, char** argv) {
    string filename, outputname;
    uint width = 256U, height = 256U;

    if (argc < 2) {
        std::clog << "Usage: ./prog sceneFile outputFile\n";
        return 0;
    } else if (argc == 2) {
        filename = argv[1];
        if (filename == "strace") {
            // ... code to test spheretracer here pls
            // TODO: temporary get rid of scene arg for sphere tracer
            unique_ptr<Scene> tmp_scene = make_unique<Scene>();
            width = 1024U; height = 1024U;
            unique_ptr<Camera> camera = make_unique<Camera>(width, height, 45.f);
            camera->setInitDistance(5.f);
            unique_ptr<SphereTracer> stracer = make_unique<SphereTracer>(width, height);
            MatrixStack P = MatrixStack();
            MatrixStack MV = MatrixStack();
            camera->applyProjection(P);
            camera->applyView(MV);
            unique_ptr<Image> image = stracer->render(tmp_scene, P.top(), MV.top());
            image->setFilename("sphereTraceTest.png");
            image->write();
            return EXIT_SUCCESS;
        } else {
            std::clog << "Usage: ./prog sceneFile outputFile\n";
            return EXIT_FAILURE;
        }
    } else if (argc >= 3) {
        filename = argv[1];
        outputname = argv[2];
    }

    if (argc >= 4) {
        width = std::stoi(argv[3]);
        height = width;
    }
    if (argc >= 5) {
        height = std::stoi(argv[4]);
    } 

    // argument handling should be rewritten to be more flexible
    // (using flags like -width, for example)

    unique_ptr<Scene> target_scene = make_unique<Scene>();

    MatrixStack P = MatrixStack();
    MatrixStack MV = MatrixStack();
    unique_ptr<Camera> camera = make_unique<Camera>(width, height, 45.0f);

    {
        SceneLoader sl(RESOURCE_DIR+filename);
        sl.setSceneFile(filename);
        sl.setResourceDirectory(RESOURCE_DIR);
        sl.loadSceneFile(camera, target_scene);
    }
    
    camera->applyProjection(P);
    camera->applyView(MV);
    
    using namespace std::chrono;
    auto start = steady_clock::now();
    unique_ptr<Image> image = camera->render(target_scene, P.top(), MV.top());
    // Cast if the platform's steady clock doesn't use nanoseconds by default
    auto interval = duration_cast<nanoseconds>(steady_clock::now() - start);
    std::clog << "Seconds used by render(): " << interval.count() / 1e9 << '\n'; 
    image->setFilename(outputname);
    image->write();

    return EXIT_SUCCESS;
}