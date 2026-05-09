#include "stn.hpp"
#include "umath.hpp"

#include "Camera.hpp"
#include "Image.hpp"
#include "Material.hpp"
#include "MatrixStack.hpp"
#include "Scene.hpp"
#include "Texture.hpp"

using namespace std;
using namespace glm;

const string RESOURCE_DIR = "../data/";

void parseSceneFile(const string& inputFile) {

}

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
    if (argc < 5) {
        clog << "Usage: ./prog sceneFile width height outputFile\n";
        return 0;
    }

    shared_ptr<Scene> target_scene = createTestScene0();

    MatrixStack P = MatrixStack();
    MatrixStack MV = MatrixStack();
    shared_ptr<Camera> camera = make_shared<Camera>(256U, 256U, 45.0f);
    camera->setInitDistance(5.0);
    camera->applyProjection(P);
    camera->applyView(MV);

    shared_ptr<Image> image = camera->render(target_scene, P.top(), MV.top());
    image->setFilename("output.png");
    image->write();

    return 0;
}