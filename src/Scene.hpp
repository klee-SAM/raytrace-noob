#pragma once
#ifndef SCENE_H
#define SCENE_H

#include "stn.hpp"
#include "Shape.hpp"
#include "Material.hpp"
#include "Light.hpp"
#include "MeshBuffer.hpp"

#include <string>
#include <unordered_map>
#include <vector>

// Should include functions for building acceleration structures
class Scene {
public:
    inline const std::vector<std::shared_ptr<Shape>>& getShapes() { return shapes; }

    inline const std::vector<std::shared_ptr<Light>>& getLights() { return lights; }

    // When shapes are pushed, they are finalized (precomputation from transforms)
    inline void pushShape(std::shared_ptr<Shape> s) { 
        s->initialize();
        shapes.push_back(s);
    }

    inline void pushLight(std::shared_ptr<Light> l) { lights.push_back(l); }

    void writeMaterial(const std::string& name, std::shared_ptr<Material> m) {
        // have to modify the object pointed to, rather than the pointer itself
        auto placed = materials.emplace(name, m);
        if (!placed.second) {
            // this is bad. need to emulate a copy constructor somehow
            std::shared_ptr<Material> oth = placed.first->second;
            oth->copy(*m);
        }
    } 
    // Creates a new default material with the given name if it does not 
    // exist beforehand (i.e, when parsing shapes data before material data)
    std::shared_ptr<Material>& getMaterial(const std::string& name) {
        auto placed = materials.try_emplace(name, std::make_shared<Material>());
        return placed.first->second; // return the material from the key-value pair
    }

    // add a new mesh if its name doesnt exist in map 
    void writeMeshBuf(const std::string& name, std::shared_ptr<MeshBuffer> mesh) {
        meshes.emplace(name, mesh); 
    }

    // may return an empty ptr if mesh not found
    std::shared_ptr<MeshBuffer> getMeshBuf(const std::string& name) {
        auto b = meshes.find(name);
        if (b != meshes.end()) return b->second;
        else return std::shared_ptr<MeshBuffer>(nullptr);
    }
    
private:
    std::vector<std::shared_ptr<Shape>> shapes;
    std::vector<std::shared_ptr<Light>> lights;
    std::unordered_map<std::string, std::shared_ptr<MeshBuffer>> meshes;
    std::unordered_map<std::string, std::shared_ptr<Material>> materials;
};


// new inner class Frame that holds vector of shapes and lights
// ofc, move the getters for shapes and lights to the frame class
/* Animated Scene JSON
{
// For defining common values to be used in transform and 
// material properties:
"defines" : { ... },    

// Defining materials for one or more shapes each.
"materials" : { ... },

// Camera settings that are shared across multiple frames.
"camera" : 
{ 
    "spp" : 0,
    "spr" : 0,
    "ao_spr" : 0,
    "ao_radius" : 0.0,
    "shd_spr" : 0,
    "sky" : "../filepath"
},
"lights" : [ ... ],
"shapes" : [ 
    ...
    {
        ...
        "transforms" : [ ... ],
        "blur_transforms" : [],     // Can explicitly specify transforms to use for
        "blur_next_frame" : true,   // motion blur, or just use next frame transforms
                                    // for motion blur, if it is present
        "id" : 1234,                // identifier for frame data
        ...
    }, 
    ...
],

// optional; when any property is specified here, it persists
// for subsequent frames until explicitly changed, and forces
// the program to render # of frames as scenes 
"frames" : [
    ...
    {
        "camera" : [ ... ], // frame-specific settings
        "next" : [ 
            ...
            {
                "id" : 1234,
                "transforms" : [ ... ], // new transforms for object of ID
                "visible" : true,       // whether or not this object is constructed
            },
            ... 
        ] // 
    },
    ...
]
}
*/

#endif