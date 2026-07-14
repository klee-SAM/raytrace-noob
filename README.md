# raytrace-noob
Adding more features on a simple raytracer as a learning exercise

## todo:

### features

#### shapes/objects
- [x] Infinite cylinders
- [x] Boxes
- [x] Tori
- [x] CSG

#### light interactions
- [X] Partial BP/Reflect
- [X] Texture mapping
- [X] Refraction
- [X] Reflection roughness
- [X] Environment mapping
- [X] Object/Regional Light Attentuation
- [X] Area lights
- [X] Ambient occlusion

#### camera effects
- [x] Antialiasing
- [x] Motion blur
- [x] Depth of field

#### optimizations
- [X] Multithreading
- [ ] BVH for scenes and meshes
    - use a kd-tree for meshes
- [ ] GPU acceleration (triangles)

#### special
- [ ] SDF and sphere tracing
    - create a fractal
- [ ] Monte Carlo path tracing

#### misc.
- [ ] get rid of stn.hpp, use proper #include practices
    - use forward declarations
    - include headers only where they are actually used
- [ ] support for blurred rotations
- [ ] scene format documentation (json)
- [x] description format for textures/patterns 
- [x] Spatial textures
- [x] phong reflection for area lights
- [ ] pmj02 random number generation
- [ ] partial .mtl support (tiny_obj_loader)
- [ ] move mesh buffers to another object
    - allows for sharing of mesh data between objects
- [x] organize src files into folders
- [ ] mesh motion blur support, whenever BVH is done
- [ ] Rewrite sceneloader to change scene after read all
    - requires intermediate data structure for holding
- [ ] support .mtl conventions for scene files
- [x] reimplement refraction with better practices

### things to consider
- creating a custom math library instead of using GLM
- Using SoA for better cache locality
    - w/o SOA, performance decreases by linearly significant
     amount if i add more data to hold b/c cache misses
    - Requires moving common data to single SoA
    - Needs rework to inserting scene shapes
    - Need to rework how intersect functions access data
    - Takes substanial effort in reworking the entire raytracer
    - a BVH would yield good results for less effort
    - could be promising, but this is a maybe
- option for noiseless (but aliased) motion blur
    - move transforms options into a group for json files
    - option to specify "frames" 
    - option to "merge" with the previous frame (render results
    of both frames are averaged together)
    - produce a sequence of png images w/ suffixed numbers


## Dependencies
- GLM (OpenGL Mathematics): https://github.com/g-truc/glm
- CMake: (https://cmake.org/download/)

## Building
This requires that you set the environment variable `GLM_INCLUDE_DIR` to a copy of the GLM library on your system. 

Other platforms may have slightly different build instructions.

1. Create a new `build` folder inside the project folder (which contains CMakeLists.txt)
2. Do `cmake ..` inside the build folder to create the Makefile for debug mode.
    - Do `cmake -DCMAKE_BUILD_TYPE=Release` for release mode instead.
    - Do `cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo` if using a tool like Callgrind.
3. Call `make` to build.

## Other libraries used
- stb_image.h and stb_image_write.h: https://github.com/nothings/stb
- jsmn.h (JSON parser): https://github.com/zserge/jsmn
- tiny_obj_loader.h: https://github.com/tinyobjloader/tinyobjloader

## Further reading:
- Raytracing in One Weekend series (https://raytracing.github.io/books/RayTracingInOneWeekend.html)
- The Ray Tracer Challenge: (https://pragprog.com/titles/jbtracer/the-ray-tracer-challenge/)
- Physically Based Rendering: (https://pbr-book.org/4ed/contents)
- Scratchapixel lessons: (https://www.scratchapixel.com/index.html) 

## Other links:

These are the resources I consulted while developing this raytracer.
See [Texture Mapping](#texture-mapping) for (some) image sources.

### Multithreading
- cpp_multithreading: (https://github.com/wasimusu/cpp_multithreading)
- https://stackoverflow.com/questions/15278343/c11-thread-safe-queue
- https://stackoverflow.com/questions/868568/what-do-the-terms-cpu-bound-and-i-o-bound-mean
- https://stackoverflow.com/questions/43482448/multithreading-raytracer
- https://stackoverflow.com/questions/5645592/large-number-of-threads-in-c-and-efficiency?rq=3

### Reflections/Refractions
- https://graphics.stanford.edu/courses/cs148-10-summer/docs/2006--degreve--reflection_refraction.pdf
- https://computergraphics.stackexchange.com/questions/7789/
- https://stackoverflow.com/questions/53549615/
- https://computergraphics.stackexchange.com/questions/4573/

### Ambient Occlusion (and Color Bleeding)
- https://computergraphics.stackexchange.com/questions/7985/
- https://research.pixar.com/docs/2017.Others.DBCHKLV.pdf
- https://jcgt.org/published/0006/01/02/paper.pdf
- https://www.rorydriscoll.com/2009/01/07/better-sampling/
- https://www.researchgate.net/publication/234787391_Real-time_obscurances_with_color_bleeding
- https://raytracing2012.wordpress.com/first-bounce-diffuse-interreflections/

### Area lighting
- https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-lighting/introduction-to-lighting-spherical-light-cone-sampling.html

### Motion blur
- https://stackoverflow.com/questions/46156903/how-to-lerp-between-two-quaternions

### Depth of field
- https://stackoverflow.com/questions/13532947/references-for-depth-of-field-implementation-in-a-raytracer
- https://cg.skeelogy.com/depth-of-field-using-raytracing/
- https://pathtracing.home.blog/depth-of-field/

### Object Light Attenuation
- https://blog.demofox.org/2017/01/09/raytracing-reflection-refraction-fresnel-total-internal-reflection-and-beers-law/
- https://blog.demofox.org/2014/06/22/analytic-fog-density/
- https://www.cs.rpi.edu/~cutler/classes/advancedgraphics/S07/final_projects/fischc/fog_simulation.html
- https://iquilezles.org/articles/fog/
- https://sberkun.github.io/184-final-proj/report/
- https://justgood.dev/docs/740-paper.pdf

### CSG
- The Ray Tracer Challenge Chapter 16: CSG
- https://groups.csail.mit.edu/graphics/classes/6.838/F01/lectures/SmoothSurfaces/0the_s040.html
- https://github.com/jtsiomb/csgray

### Tori
- http://cosinekitty.com/raytrace/chapter13_torus.html (Unsecure)
- https://www.shadertoy.com/view/3XdyRj
- https://iquilezles.org/articles/intersectors/

### Texture Mapping
- http://raytracerchallenge.com/bonus/texture-mapping.html (Not secure)
- Images 1: https://texturify.com/stock-photo/above-the-city-in-winter-10588.html
- Images 2: https://polyhaven.com/hdris

### BVH and GPU Acceleration
- https://github.com/AdaptiveCpp/AdaptiveCpp
- https://umm-csci.github.io/senior-seminar/seminars/fall2011/Martin.pdf
- https://stackoverflow.com/questions/61222620/optimizing-bvh-traversal-in-ray-tracer
- https://computergraphics.stackexchange.com/questions/10098/is-bvh-faster-than-the-octree-kd-tree-for-raytracing-the-objects-on-a-gpu

### Sampling Methods
- https://github.com/Andrew-Helmer/pmj-cpp/tree/master/sample_generation
- https://jcgt.org/published/0008/01/04/paper.pdf
- https://abau.io/blog/sample_patterns/
- https://stackoverflow.com/questions/5147378/

### Enabling SIMD for GLM
- https://gamedev.stackexchange.com/questions/132549/how-to-use-glm-simd-using-glm-version-0-9-8-2 

### Other Interesting Links
- https://stackoverflow.com/questions/11227809
- https://www.cs.umd.edu/users/mount/Indep/Alisa_Chen/caustics.html
- https://github.com/catchorg/Catch2/blob/devel/docs/cmake-integration.md