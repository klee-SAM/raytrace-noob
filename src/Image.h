#pragma once
#include "stn.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

class Image {
public:
    Image(uint w, uint h) : width(w), height(h), comp(3), data(w*h*comp, 0) {}
    ~Image() { data.clear(); };

    void setFilename(const std::string &name) { filename = name; }
    uint getWidth() const { return width; }
    uint getHeight() const { return height; }

    // The origin of the image starts at the lower left corner.
    void setPixel(uint x, uint y, u_char r, u_char g, u_char b) {
        if (x < 0 || x >= width) { 
            std::cerr << "Column " << x << " out of bounds\n"; 
            return;
        }
        if (y < 0 || y >= height) { 
            std::cerr << "Row " << y << " out of bounds\n"; 
            return;
        }

        // the origin (0, 0) of the image is the upper left corner, so 
        // flip the row to make origin be lower left to be consistent
        y = height - y - 1;
        size_t index = y*width + x;
        assert(index >= 0);
	    assert(3*index + 2 < data.size());
        data.at(3*index + 0) = r;
        data.at(3*index + 1) = g;
        data.at(3*index + 2) = b;
    }

    void setPixel(uint x, uint y, const glm::vec3& clr) {
        u_char r = ((u_char)std::clamp(clr.r, 0.0f, 1.0f))*255;
        u_char g = ((u_char)std::clamp(clr.g, 0.0f, 1.0f))*255;
        u_char b = ((u_char)std::clamp(clr.b, 0.0f, 1.0f))*255;
        setPixel(x, y, r, g, b);
    }

    void write() {
        // The distance in bytes from the first byte of a row of pixels to the
        // first byte of the next row of pixels
        int stride_in_bytes = width*comp*sizeof(unsigned char);
        int ret = stbi_write_png(filename.c_str(), width, height, comp, &data[0], stride_in_bytes);
        if(ret) {
            std::clog << "Wrote to " << filename << '\n';
        } else {
            std::cerr << "Failed write to " << filename << '\n';
        }

    }
    
private:
    std::string filename;
    uint width, height, comp;
    std::vector<u_char> data;
};