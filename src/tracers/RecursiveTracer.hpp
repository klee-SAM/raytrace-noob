#pragma once
#ifndef WHITTED_TRACER_H
#define WHITTED_TRACER_H

#include "../stn.hpp"
#include "Raytracer.hpp"

struct DistriTracer : public Raytracer<DistriTracer>
{
public:
    glm::vec3 getRayColorImpl(const Ray&) const;

private:
    class sampleCone;

    glm::vec3 raytrace(const Ray &ray, const Interval&, uint bounces) const;

    glm::vec3 getReflectedColor(const Ray &ray, IntParams args, uint recursions) const;

    glm::vec3 getRefractedColor(const Ray &ray, IntParams args,
                                uint recursions, bool back_face) const;

    float occlusionDiffuseFactor(IntParams args, glm::vec3 &diffuseFac, float time) const;

    glm::vec3 getShadowContrib(const Ray &sray, const Interval &t_int) const;
    glm::vec3 lightingFactor(const Ray &ray, IntParams args,
                             const std::shared_ptr<Light> &light,
                             const glm::vec3 &diffuseAtt,
                             bool sampleArea = true) const;
};

#endif