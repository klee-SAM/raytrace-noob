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

void initializeTestScene0(unique_ptr<Scene>& target_scene) {
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

    // Hardcoded. Beware!
    shared_ptr<Texture> example = 
        make_shared<ImageTexture>(RESOURCE_DIR+"textures/exampleTexture.png");
    shared_ptr<Texture> earth = 
        make_shared<ImageTexture>(RESOURCE_DIR+"textures/earthmap.png");

    shared_ptr<Material> blueMat = make_shared<Material>(
        vec3(0.1f, 0.1f, 0.1f),
        vec3(0.0f, 0.0f, 1.0f),
        vec3(1.0f, 1.0f, 0.5f),
        100.0f
    );

    // hmm...
    blueMat->diffuse = example;
    greenMat->diffuse = earth;

    shared_ptr<Light> lighta = make_shared<Light>(vec3(-2.0f, 1.0f, 1.0f), 0.5);
    shared_ptr<Light> lightb = make_shared<Light>(vec3(2.0f, 4.0f, 4.0f), 0.5);
	target_scene->pushLight(lighta);
    target_scene->pushLight(lightb);
	
    MatrixStack MV;

    MV.push();
    MV.translate(vec3(-0.5f, -1.0f, 1.0f));
	shared_ptr<Sphere> red_sp = make_shared<Sphere>();
    red_sp->setModelMatrix(MV.top());
	red_sp->setMaterial(greenMat);
	target_scene->pushShape(red_sp);
    MV.pop();
	
    MV.push();
    MV.translate(vec3(0.5f, -1.0f, -1.0f));
	shared_ptr<Sphere> green_sp = make_shared<Sphere>();
    green_sp->setModelMatrix(MV.top());
	green_sp->setMaterial(redMat);
	target_scene->pushShape(green_sp);
    MV.pop();

    MV.push();
    MV.translate(vec3(0.0f, 1.0f, 0.0f));
	shared_ptr<Sphere> blue_sp = make_shared<Sphere>();
    blue_sp->setModelMatrix(MV.top());
	blue_sp->setMaterial(blueMat);
	target_scene->pushShape(blue_sp);
    MV.pop();

    MV.push();
    MV.translate(vec3(0.0f, 0.0f, -5.0f));
    MV.rotate(45.0f, vec3(1.0f, 0.0f, 0.0f));
    MV.rotate(60.0f, vec3(0.0f, 0.0f, 1.0f));
	shared_ptr<Plane> wall = make_shared<Plane>();
    wall->setModelMatrix(MV.top());
	wall->setMaterial(blueMat);
	target_scene->pushShape(wall);
    MV.pop();
}

void initializeCSGTestScene(unique_ptr<Scene>& target_scene) {
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
        vec3(0.0f, 0.25f, 1.0f),
        vec3(1.0f, 1.0f, 0.5f),
        100.0f
    );

    shared_ptr<Light> lighta = make_shared<Light>(vec3(-2.0f, 2.0f, 2.0f), 0.5f);
    shared_ptr<Light> lightb = make_shared<Light>(vec3(2.0f, -2.0f, 4.0f), 0.5f);
	target_scene->pushLight(lighta);
    target_scene->pushLight(lightb);

    MatrixStack MV;

    shared_ptr<Sphere> green_sp = make_shared<Sphere>();
    MV.push();
    MV.translate(vec3(0.0f, 0.0f, 0.0f));
    MV.scale(0.875f);
	green_sp->setModelMatrix(MV.top());
	green_sp->setMaterial(greenMat);
    MV.pop();

	shared_ptr<Sphere> blue_sp = make_shared<Sphere>();
    MV.push();
    MV.translate(vec3(0.0f, 0.0f, .5f));
    MV.scale(0.625f);
    blue_sp->setModelMatrix(MV.top());
	blue_sp->setMaterial(blueMat);
    MV.pop();

	shared_ptr<Sphere> red_sp = make_shared<Sphere>();
    MV.push();
	red_sp->setMaterial(redMat);
    red_sp->setModelMatrix(MV.top());
    MV.pop();

	shared_ptr<CSG> csgtest2 = make_shared<CSG>(
        OperationType::Intersection, red_sp, 
        make_shared<CSG>(
            OperationType::Union, green_sp, blue_sp)
    );
    
	target_scene->pushShape(csgtest2);

    MV.push();
	MV.translate(vec3(2.0f, 1.5f, -4.5f));
    MV.translate(0.0f, 0.0f, -5.0f);
	MV.rotate(-45.0f, vec3(0.0f,1.0f,0.0f));
	auto world = MV.top();
	// misnomer, getModelMatrix() is the object matrix
	blue_sp->setModelMatrix(world*blue_sp->getModelMatrix());
	red_sp->setModelMatrix(world*red_sp->getModelMatrix());
	green_sp->setModelMatrix(world*green_sp->getModelMatrix());
    MV.pop();

	vector<Shape> disk_comps;
	shared_ptr<Sphere> rs = make_shared<Sphere>();
    MV.push();
    MV.translate(vec3(0.0f, 0.0f, 0.0f));
    MV.scale(1.0f);
	rs->setModelMatrix(MV.top());
	rs->setMaterial(redMat);
    MV.pop();

	shared_ptr<Sphere> bs = make_shared<Sphere>();
    MV.push();
    MV.translate(vec3(0.0f, 0.0f, .5f));
	MV.scale(0.625f);
	bs->setModelMatrix(MV.top());
	bs->setMaterial(blueMat);
    MV.pop();

	shared_ptr<Sphere> gs = make_shared<Sphere>();
	MV.push();
    MV.translate(vec3(0.0f, 0.0f, -.5f));
	gs->setModelMatrix(MV.top());
	gs->setMaterial(greenMat);
    MV.pop();

	shared_ptr<CSG> hole_disk = make_shared<CSG>(
		OperationType::Difference, rs,
        make_shared<CSG>(OperationType::Union, gs, bs)
    );
	target_scene->pushShape(hole_disk);

	MV.reset();
	MV.translate(vec3(-1.0f, 1.5f, -5.5f));
	MV.rotate(225.0f, vec3(0.0f,1.0f,0.0f));
	world = MV.top();
	bs->setModelMatrix(world*bs->getModelMatrix());
	rs->setModelMatrix(world*rs->getModelMatrix());
	gs->setModelMatrix(world*gs->getModelMatrix());
}

int main(int argc, char** argv) {
    string filename, outputname;
    uint width = 256U, height = 256U;

    if (argc < 3) {
        clog << "Usage: ./prog sceneFile outputFile\n";
        return 0;
    } else if (argc >= 3) {
        filename = argv[1];
        outputname = argv[2];
    }

    if (argc >= 4) {
        width = stoi(argv[3]);
        height = width;
    }
    if (argc >= 5) {
        height = stoi(argv[4]);
    } 

    // argument handling should be rewritten to be more flexible
    // (using flags like -width, for example)

    unique_ptr<Scene> target_scene = make_unique<Scene>();

    MatrixStack P = MatrixStack();
    MatrixStack MV = MatrixStack();
    unique_ptr<Camera> camera = make_unique<Camera>(width, height, 45.0f);

    /// test 
    if (filename == "csgtest") {
        outputname = "csgtest.png";
        initializeCSGTestScene(target_scene);
    } else if (filename == "testscene0") {
        outputname = "testscene0.png";
        camera->setInitDistance(5.0f);
        initializeTestScene0(target_scene);
    } else {
        SceneLoader sl(RESOURCE_DIR+filename);
        sl.setSceneFile(filename);
        sl.setResourceDirectory(RESOURCE_DIR);
        sl.loadSceneFile(camera, target_scene);
    }
    
    camera->applyProjection(P);
    camera->applyView(MV);

    clock_t start = clock();
    // Each core increments the cycle count by 1, so attempt to account for that in the benchmark
    // by assuming all processors are used. This is still not totally accurate.
    auto processor_count = std::thread::hardware_concurrency();
    if (processor_count < 1) processor_count = 1;
    unique_ptr<Image> image = camera->render(target_scene, P.top(), MV.top());
    clog << "Seconds used by render(): " 
         << (double)(clock()-start)/(processor_count*CLOCKS_PER_SEC) << '\n';
    // commit rationing todo: use std::chrono steady clock or high res clock
    image->setFilename(outputname);
    image->write();

    return 0;
}