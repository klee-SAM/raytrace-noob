#include "stn.hpp"
#include "umath.hpp"

#include "Camera.hpp"
#include "Image.hpp"
#include "Material.hpp"
#include "MatrixStack.hpp"
#include "Scene.hpp"
#include "SceneLoader.hpp"
#include "Texture.hpp"

using namespace std;
using namespace glm;

const string RESOURCE_DIR = "../data/";

shared_ptr<Scene> createTestScene0() {
    shared_ptr<Material> redMat = make_shared<Material>(
        vec3(0.1f, 0.1f, 0.1f),
        vec3(1.0f, 0.0f, 0.0f),
        vec3(1.0f, 1.0f, 0.5f),
        100.0f
    );

    shared_ptr<Material> greenMat = make_shared<Material>(
        vec3(0.1f, 0.1f, 0.1f),
        vec3(0.0f, 1.0f, 0.0f),
        vec3(1.0f, 1.0f, 0.5f),
        100.0f
    );

    shared_ptr<Material> blueMat = make_shared<Material>(
        vec3(0.1f, 0.1f, 0.1f),
        vec3(0.0f, 0.0f, 1.0f),
        vec3(1.0f, 1.0f, 0.5f),
        100.0f
    );

    shared_ptr<Scene> target_scene = make_shared<Scene>();

    shared_ptr<Light> lighta = make_shared<Light>(vec3(-2.0f, 1.0f, 1.0f), 1.0f);
	target_scene->lights.push_back(lighta);
	
    MatrixStack MV;

    MV.push();
    MV.translate(vec3(-0.5f, -1.0f, 1.0f));
	shared_ptr<Sphere> red_sp = make_shared<Sphere>();
    red_sp->setModelMatrix(MV.top());
	red_sp->setMaterial(redMat);
	target_scene->shapes.push_back(red_sp);
    MV.pop();
	
    MV.push();
    MV.translate(vec3(0.5f, -1.0f, -1.0f));
	shared_ptr<Sphere> green_sp = make_shared<Sphere>();
    green_sp->setModelMatrix(MV.top());
	green_sp->setMaterial(greenMat);
	target_scene->shapes.push_back(green_sp);
    MV.pop();

    MV.push();
    MV.translate(vec3(0.0f, 1.0f, 0.0f));
	shared_ptr<Sphere> blue_sp = make_shared<Sphere>();
    blue_sp->setModelMatrix(MV.top());
	blue_sp->setMaterial(blueMat);
	target_scene->shapes.push_back(blue_sp);
    MV.pop();

    return target_scene;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        clog << "Usage: ./prog sceneFile outputFile\n";
        return 0;
    }

    // argument handling should be rewritten to be more flexible
    // (using flags like -width, for example)

    string filename = argv[1];
    string outputname = argv[2];

    // fix the width and height for debugging, for now
    uint width = 64U;
    uint height = width;

    shared_ptr<Scene> target_scene = make_shared<Scene>();

    MatrixStack P = MatrixStack();
    MatrixStack MV = MatrixStack();
    shared_ptr<Camera> camera = make_shared<Camera>(width, height, 45.0f);

    SceneLoader sl(RESOURCE_DIR+filename);
    sl.setResourceDirectory(RESOURCE_DIR);
    sl.loadSceneFile(camera, target_scene);
    
    camera->applyProjection(P);
    camera->applyView(MV);

    clock_t start = clock();
    shared_ptr<Image> image = camera->render(target_scene, P.top(), MV.top());
    clog << "Seconds used by render(): " << (double)(clock()-start)/CLOCKS_PER_SEC << '\n';
    image->setFilename(outputname);
    image->write();

    return 0;
}