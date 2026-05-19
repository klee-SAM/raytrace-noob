# raytrace-noob
Adding more features as a learning exercise

## todo:

### features

#### shapes/objects
- [x] Infinite cylinders
- [x] Boxes
- [ ] CSG
- [ ] Object/Regional Light Attentuation

#### light interactions
- [ ] Texture mapping
- [X] Refraction
- [ ] Surface roughness
- [ ] Ambient occulusion
- [ ] Area lights

#### other effects
- [x] Antialiasing
- [ ] Motion blur
- [ ] SDF and sphere tracing

#### optimizations
- [X] Multithreading
- [ ] BVH for meshes
- [ ] BVH for scene
- [ ] GPU acceleration

#### special
- [ ] Monte Carlo path tracing

### misc.
- [ ] scene format documentation (json)
- [ ] partial .mtl support (tiny_obj_loader)



## Dependencies
- GLM (OpenGL Mathematics): https://github.com/g-truc/glm
- CMake: (https://cmake.org/download/)

## Building
This requires that you set the environment variable `GLM_INCLUDE_DIR` to a copy of the
GLM library on your system. 

These instructions are for Linux. Other platforms may have slightly different build instructions.

1. Create a new `build` folder inside the project folder (which contains CMakeLists.txt)
2. Do `cmake ..` inside the build folder to create the Makefile.
3. Call `make` to build.

## Other libraries used
- stb_image.h and stb_image_write.h: https://github.com/nothings/stb
- jsmn.h (JSON parser): https://github.com/zserge/jsmn
- tiny_obj_loader.h: https://github.com/tinyobjloader/tinyobjloader

# Further reading:

- Raytracing in One Weekend series (https://raytracing.github.io/books/RayTracingInOneWeekend.html)
- The Ray Tracer Challenge: (https://pragprog.com/titles/jbtracer/the-ray-tracer-challenge/)
- cpp_multithreading: (https://github.com/wasimusu/cpp_multithreading)
- https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle
- https://graphics.stanford.edu/courses/cs148-10-summer/docs/2006--degreve--reflection_refraction.pdf