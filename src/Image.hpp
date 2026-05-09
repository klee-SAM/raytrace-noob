#pragma once
#include "stn.hpp"



class Image {
public:
    Image(uint w, uint h) : width(w), height(h), comp(3), data(w*h*comp, 0) {}
    ~Image() { data.clear(); };

    void setFilename(const std::string &name) { filename = name; }
    uint getWidth() const { return width; }
    uint getHeight() const { return height; }

    // The origin of the image starts at the lower left corner.
    void setPixel(uint x, uint y, u_char r, u_char g, u_char b);

    // Automatically clamps the color components from [0.0, 1.0].
    void setPixel(uint x, uint y, const glm::vec3& clr);

    void write();
    
private:
    std::string filename;
    uint width, height, comp;
    std::vector<u_char> data;
};