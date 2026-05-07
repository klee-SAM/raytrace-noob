#include "Camera.h"
#include "Ray.h"

using namespace std;
using namespace glm;

void Camera::applyProjection(shared_ptr<MatrixStack> MS) {
    MS->mult(perspective(fovy, aspectRatio, znear, zfar));
}
void Camera::applyView(shared_ptr<MatrixStack> MS) {
    MS->translate(translation);
    MS->rotate(rotation.y, vec3(1.0f, 0.0f, 0.0f));
    MS->rotate(rotation.x, vec3(0.0f, 1.0f, 0.0f));
}

shared_ptr<Image> Camera::render(
    shared_ptr<Scene> scene, 
    const mat4& P,
    const mat4& V) 
{
    // Precompute as much as possible before loops
    mat4 C = inverse(V);
    auto invP = glm::inverse(P);
    vec4 cameraPos = C[3]; cameraPos.w = 1.0f;
    double dy = 1.0f/((double)height);
    double dx = 1.0f/((double)width);

    // TODO create image

    for (int y = 0; y < height; ++y) {
        double ndc_y = 2*((double)y)/((double)height) - 1.0;
        ndc_y += dy;
        for (int x = 0; x < width; ++x) {
            double ndc_x = 2*((double)x)/((double)width) - 1.0;
            ndc_x += dx;
            glm::vec4 coord((float)ndc_x, (float)ndc_y, -1.0f, 1.0f); // px coord
            coord = invP*coord; // eye coord
			coord.w = 1.0f;
			coord = glm::normalize(C*coord - cameraPos); // n_pw

			Ray cray(vec3(cameraPos), vec3(coord));
            // TODO compute primary color
        }
    }
}




// vector<Hit> primary_hits;
// // the hit object should contain all of the information of the closest object
// bool Camera::hit(vector<shared_ptr<Shape>>& shapes, const Ray& ray, float min, float max, Hit& closestHit) {
//     float minDist = max;
//     bool intersected = false;
//     for (shared_ptr<Shape>& shape : shapes) {
//         // TODO: maybe rework the intersect functions to 
//         // only return at most one hit object 
//         shape->intersect(ray, primary_hits);
//         if (primary_hits.empty()) continue;
//         for (Hit& hit : primary_hits) {
//             if (hit.t >= min && hit.t <= max) {
//                 intersected = true;
//             } else {
//                 continue;
//             }
//             if (minDist > hit.t) {
//                 minDist = hit.t;
//                 closestHit = hit;
//             }
//         }
//         primary_hits.clear();
//     }
//     return intersected;
// }

// const int MAX_RECURSIONS = 7;
// const bool SHOW_NORMALS = false;

// const float REFL_EPISILION = 0.005f;

// vec3 Camera::getRayColor(Scene& scene, const Ray& ray, float min, float max, int recursiveDepth) {
//     Hit rec;
//     if (hit(scene.shapes, ray, min, max, rec)) {
//         vec3 clr = vec3(0.0);
//         if (SHOW_NORMALS) {
//             clr = rec.n;
//             clr.r = .5f*clr.r+.5f;
//             clr.g = .5f*clr.g+.5f;
//             clr.b = .5f*clr.b+.5f;
//             return clr;
//         } else if (rec.mat->reflCoeff > REFL_EPISILION) { 
//             if (recursiveDepth < MAX_RECURSIONS) {
//                 vec3 refl = reflect(ray.direction, rec.n);
//                 clr += rec.mat->reflCoeff*this->getRayColor(scene, Ray(rec.x + rec.n*min, refl), min, max, recursiveDepth + 1);
//             } else {
//                 return clr;
//             }
//         }

//         vec3 ev;
//         if (recursiveDepth > 0) {
//             ev = normalize(-ray.direction);
//         } else {
//             ev = -normalize(rec.x - vec3(cameraPos));
//         }

//         vec3 bp_clr = rec.mat->ambient;
//         for (auto& light : scene.lights) {
//             // Construct a shadow ray for each light, using
//             // world coordinates.
//             vec3 ld = light->position - rec.x;
//             vec3 lv = normalize(ld);
//             float tl = length(ld);
//             Ray sray = Ray(rec.x + min*rec.n, lv);

//             Hit srec;
//             if (hit(scene.shapes, sray, min, tl, srec)) {
//                 continue;
//             }

//             float Li = light->intensity;
//             vec3 hv = normalize(lv + ev);
//             float md = glm::max(0.0f, dot(rec.n, lv));
//             float ms = glm::pow(glm::max(0.0f, dot(rec.n, hv)), rec.mat->exponent);
//             bp_clr += Li * (rec.getDiffuse()*md + rec.mat->specular*ms);
//         }
//         clr += (1.0f - rec.mat->reflCoeff) * bp_clr;
//         // Clamp the pixel colors to prevent strange results on specular.
//         clr.r = glm::clamp(clr.r, 0.0f, 1.0f);
//         clr.g = glm::clamp(clr.g, 0.0f, 1.0f);
//         clr.b = glm::clamp(clr.b, 0.0f, 1.0f);
//         return clr;
//     } else {
//         return vec3(0.0f);
//     }
// }
