#pragma once
#include "stn.hpp"

#include <string>
#include <vector>

class Image {
public:
    // For constructing output images
    Image(uint w, uint h) noexcept : width(w), height(h), comp(3), data(w*h*comp, 0) {}
    // For constructing texture images. 
    Image(const std::string& file) : filename(file) { loadFile(); }
    ~Image() { data.clear(); }

    bool loadFile(bool suppressNotFoundError = false);
    bool loadFile(const std::string& file) { filename = file; return loadFile(); } 

    // Returns true if the data buffer holds at least 1 pixel's worth of information
    bool isLoaded() { return data.size() > comp; }

    void setFilename(const std::string &name) { filename = name; }

    uint getWidth() const { return width; }
    uint getHeight() const { return height; }

    // The origin of the image starts at the lower left corner.
    void setPixel(uint x, uint y, u_char r, u_char g, u_char b);

    // Automatically clamps the color components from [0.0, 1.0].
    void setPixel(uint x, uint y, const glm::vec3& clr);

    // Writes the pixel components at the coordinates to the 3 reference args
    void getPixel(uint x, uint y, u_char& r, u_char& g, u_char& b) const;

    // Writes the pixel color at UV coords to the ref. arg. `clr`
    void getPixel(float u, float v, glm::vec3& clr) const;

    void write();
    
private:
    // So it turns out that this specific ordering of these member variables are 
    // needed so that the cost of constructing 
    std::string filename;
    uint width, height, comp;
    std::vector<u_char> data;

    size_t get_index(uint x, uint y) const;
};

const std::shared_ptr<Image> blankImage = std::make_shared<Image>(1U, 1U);