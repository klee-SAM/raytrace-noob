#include "../stn.hpp"
#include "Raytracer.hpp"

#include "../Ray.hpp"
#include "../util/umath.hpp"

#include <glm/gtc/noise.hpp>

using glm::vec3, glm::vec4, glm::mat4;

using CONSTANTS::EPSILION;
constexpr float MIN_DIST = 0.005f;
constexpr float MAX_DIST = 1000.f;

using std::unique_ptr;
using std::vector;

// good to do
// Multiple shapes
// CSG operations (unions, intersections, subtractions, etc.)
// Fractal SDFs
// Shadows and/or ambient occlusion

// https://raw.githubusercontent.com/pedrotrschneider/shader-fractals/refs/heads/main/3D/Mandelbox.glsl
// Some of the following SDF code was ripped from the above source, with some
// modifications. I really liked this.

// Converts a color from the HSV colorspace to RGB
vec3 hsv2rgb (vec3 c) {
  vec4 K = vec4 (1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
  vec3 hue_comp = vec3(c.x);
  vec3 p = glm::abs((glm::fract(hue_comp + vec3(K)) * 6.f) - K.w);
  return c.z * glm::mix(vec3(K.x), glm::clamp(p - vec3(K.x), 0.f, 1.f), c.y);
}

// SDF FUNCTIONS //
vec4 sphere(vec4 z) {
  float r2 = glm::dot(vec3(z), vec3(z));
  if (r2 < 2.0)
    z *= (1.0 / r2);
  else z *= 0.5;

  return z;
}

vec3 box(vec3 z) {
  return glm::clamp(z, vec3(-1.0), vec3(1.0)) * 2.f - z;
}

// mandelbox
float DE2 (vec3 pos) {
  vec3 params = vec3(0.5f);
  vec4 scale = vec4(-20.f * 0.272321);
  vec4 p = vec4(pos, 1.f);
  vec4 c = vec4(params, 0.5f) - 0.5f; // param = 0..1

  for (float i = 0.0; i < 10.0; i++) {
    vec3 b = box(vec3(p));
    p.x = b.x;
    p.y = b.y;
    p.z = b.z;
    p = sphere(p);
    p = p * scale + c;
  }

  return glm::length(vec3(p)) / p.w;
}

float sceneSDF(vec3 p) {
    return DE2(p);
}

vec3 sceneNormal(vec3 p) {
    // Observe how the SDF output changes to get the normal;
    // also known as calculating the gradient

    constexpr float SMALL_STEP = 0.01f;
    constexpr vec3 stepX = vec3(SMALL_STEP, 0.f, 0.f);
    constexpr vec3 stepY = vec3(0.f, SMALL_STEP, 0.f);
    constexpr vec3 stepZ = vec3(0.f, 0.f, SMALL_STEP);

    float gX = sceneSDF(p + stepX) - sceneSDF(p - stepX);
    float gY = sceneSDF(p + stepY) - sceneSDF(p - stepY);
    float gZ = sceneSDF(p + stepZ) - sceneSDF(p - stepZ);

    return glm::normalize(vec3(gX, gY, gZ));
}

const Interval interval(MIN_DIST, MAX_DIST);

vec3 Raytracer::rayMarch(const Ray &ray) const
{
    float total_dist = 0.0f;
    const int MAX_STEPS = 128;
    const float MIN_HIT_DIST = interval.min;
    const float MAXIMUM_TRACE_DIST = interval.max;

    float min_dist_to_sdf = CONSTANTS::INF;
    vec3 min_dist_to_sdf_pos = ray.getPos();

    for (int i = 0; i < MAX_STEPS; ++i) 
    {
        vec3 curr_pos = ray.getPos() + total_dist * ray.getDir();

        float dist_to_sdf = sceneSDF(curr_pos);

        if (min_dist_to_sdf > dist_to_sdf) {
            min_dist_to_sdf = dist_to_sdf;
            min_dist_to_sdf_pos = curr_pos;
        }

        if (dist_to_sdf < MIN_HIT_DIST) 
        {
            // inside
            vec3 normal = sceneNormal(curr_pos);

            // handedness is consistent actually; +x goes to the right
            vec3 light_position = vec3(2.0, 5.0, 3.0);

            // Calculate the unit direction vector that points from
            // the point of intersection to the light source
            vec3 direction_to_light = glm::normalize(light_position - curr_pos);

            const vec3 h = normalize(direction_to_light - ray.getDir());
            float dI = std::max(0.f, glm::dot(normal, direction_to_light));
            float sI = std::pow(std::max(0.0f, glm::dot(normal, h)), 100.f);

            vec3 Kd = vec3(0.0078, 0.6745, 0.8392);

            const float hF = 10.0, sat = 1.0, val = 0.8;
            const vec3 hsvClr = vec3(0.8 + (glm::length(curr_pos) / hF), sat, val);
            Kd = glm::mix(Kd, hsv2rgb(hsvClr), .35f);

            vec3 clr = vec3(0.1) + Kd*dI + vec3(1.0f)*sI;

            clr /= i * 0.08; // Ambient occlusion
            clr /= glm::distance(ray.getPos(), min_dist_to_sdf_pos);
            clr *= 2.0;            

            return clr;
        }
        if (total_dist > MAXIMUM_TRACE_DIST) 
        {
            // miss
            break;
        }

        total_dist += dist_to_sdf;
    }

    return vec3(0.3f, 0.4f, 0.6f) * 0.5f;
}