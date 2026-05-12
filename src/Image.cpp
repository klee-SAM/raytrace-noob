#include "Image.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void Image::setPixel(uint x, uint y, u_char r, u_char g, u_char b) {
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

// Automatically clamps the color components from [0.0, 1.0].
void Image::setPixel(uint x, uint y, const glm::vec3& clr) {
    // multiply by under 256 so that truncation ensures that 
    // the comp does not exceed 256 but also does not need to
    // be exactly 1.0 to get to 255
    u_char r = (u_char)(std::clamp(clr.r, 0.0f, 1.0f)*255.99f);
    u_char g = (u_char)(std::clamp(clr.g, 0.0f, 1.0f)*255.99f);
    u_char b = (u_char)(std::clamp(clr.b, 0.0f, 1.0f)*255.99f);
    setPixel(x, y, r, g, b);
}

void Image::write() {
    // The distance in bytes from the first byte of a row of pixels to the
    // first byte of the next row of pixels
    int stride_in_bytes = width*comp*sizeof(unsigned char);
    int ret = stbi_write_png(filename.c_str(), width, height, comp, &data[0], stride_in_bytes);
    if (ret) std::clog << "Wrote to " << filename << '\n';
    else std::cerr << "Failed write to " << filename << '\n';
}