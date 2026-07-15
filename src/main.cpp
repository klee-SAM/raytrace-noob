#include "stn.hpp"

#include "util/umath.hpp"
#include "util/MatrixStack.hpp"

#include "Camera.hpp"
#include "Image.hpp"
#include "Material.hpp"
#include "Scene.hpp"
#include "SceneLoader.hpp"
#include "Texture.hpp"

#include <chrono>
#include <iostream>

// #include <array>

using std::string;
using std::unique_ptr, std::shared_ptr;
using std::make_shared, std::make_unique;
using std::cerr;

using std::vector;

const string RESOURCE_DIR = "../data/";

int main(int argc, char** argv) {
    string filename, outputname;
    uint width = 256U, height = 256U;

    // {
    // using namespace std::chrono;
    // auto start = steady_clock::now();

    // float sumT = 0.f;

    // for (int i = 0; i < 100'000; ++i) {
    //     // HitArray hs;
    //     // vector<Hit> hs;
    //     std::array<Hit, 32> hs;
    //     // hs.reserve(32);

    //     float sum = 0.f;
    //     for(size_t i = 0; i < 32; ++i) {
    //         Hit h;
    //         h.t = std::rand() * (1.f / RAND_MAX);
    //         hs.at(i) = h;
    //         // hs.push_back(h);
    //     }
    //     // hs.sort();
    //     // Hit::sortHits(hs);
    //     constexpr auto cmp = [](const Hit& a, const Hit& b) { return a.t < b.t; };
    //     std::sort(hs.begin(), hs.end(), cmp); 

    //     std::for_each(hs.begin(), hs.end(), [&sum](Hit& h){ sum += h.t; });
    //     sumT += sum;
    // }

    // auto interval = duration_cast<nanoseconds>(steady_clock::now() - start);
    // std::clog << "Seconds used : " << sumT << " : " << interval.count() / 1e9 << '\n'; 

    // return 0;
    // }

    if (argc < 3) {
        std::clog << "Usage: ./prog sceneFile outputFile\n";
        return 0;
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

    return 0;
}