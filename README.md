# raytrace-noob
Adding more features on a simple raytracer as a learning exercise

## todo:

### features

#### shapes/objects
- [x] Infinite cylinders
- [x] Boxes
- [ ] Tori
- [ ] Object/Regional Light Attentuation
    - "fog!"
- [x] CSG

#### light interactions
- [X] Partial BP/Reflect
- [X] Texture mapping
- [X] Refraction
- [X] Reflection roughness
- [X] Area lights
- [X] Ambient occulusion

#### camera effects
- [x] Antialiasing
- [ ] Motion blur

#### optimizations
- [X] Multithreading
- [ ] BVH for scenes and meshes
    - use a kd-tree for meshes
- [ ] GPU acceleration (triangles)

#### special
- [ ] SDF and sphere tracing
    - create a fractal
- [ ] Monte Carlo path tracing

### misc.
- [ ] scene format documentation (json)
- [ ] description format for textures/patterns 
- [x] ambient occlusion settings
- [ ] partial .mtl support (tiny_obj_loader)



## Dependencies
- GLM (OpenGL Mathematics): https://github.com/g-truc/glm
- CMake: (https://cmake.org/download/)

## Building
This requires that you set the environment variable `GLM_INCLUDE_DIR` to a copy of the GLM library on your system. 

Other platforms may have slightly different build instructions.

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
- Physically Based Rendering: (https://pbr-book.org/4ed/contents)
- Scratchapixel lessons: (https://www.scratchapixel.com/index.html) 

## Other links:

### Multithreading
- cpp_multithreading: (https://github.com/wasimusu/cpp_multithreading)
- https://stackoverflow.com/questions/15278343/c11-thread-safe-queue

### Reflections/Refractions
- https://graphics.stanford.edu/courses/cs148-10-summer/docs/2006--degreve--reflection_refraction.pdf

### Ambient Occlusion
- https://computergraphics.stackexchange.com/questions/7985/
- https://research.pixar.com/docs/2017.Others.DBCHKLV.pdf
- https://jcgt.org/published/0006/01/02/paper.pdf
- https://www.rorydriscoll.com/2009/01/07/better-sampling/

## Area lighting
- https://jcgt.org/published/0008/01/04/paper.pdf
- https://github.com/Andrew-Helmer/pmj-cpp/tree/master/sample_generation
- https://abau.io/blog/sample_patterns/
- https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-lighting/introduction-to-lighting-spherical-light-cone-sampling.html
