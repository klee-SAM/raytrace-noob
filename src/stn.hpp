#pragma once

#define _USE_MATH_DEFINES
#include <algorithm>
#include <cassert>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#ifdef __AVX2__
#define GLM_FORCE_SIMD_AVX2
#endif
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/common.hpp>
#include <glm/matrix.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>


typedef glm::vec2 vec2;
typedef glm::vec3 vec3;
typedef glm::vec4 vec4;
typedef glm::mat3 mat3;
typedef glm::mat4 mat4;