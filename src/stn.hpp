#pragma once
#ifndef STN_H
#define STN_H

#define _USE_MATH_DEFINES
#include <algorithm>
#include <cassert>
#include <cfloat>
#include <cmath>
#include <memory>
#include <utility>

#if defined(__AVX2__)
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_SIMD_AVX2
#endif

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>

// Just in case uint isn't defined on some platforms
using uint = unsigned int;

#endif