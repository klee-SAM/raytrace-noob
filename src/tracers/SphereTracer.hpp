#pragma once
#ifndef SPHERE_TRACER_H
#define SPHERE_TRACER_H

#include "../stn.hpp"
#include "Raytracer.hpp"

struct SphereTracer : public Raytracer<SphereTracer>
{
    glm::vec3 getRayColorImpl(const Ray&) const;
};

#endif