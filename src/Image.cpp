#include "Image.hpp"

#include "umath.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

const float COLOR_SCALE = 1.0f / 255.0f;

Image::Image(const std::string& file) : filename(file) {
    // TODO
}

size_t Image::get_index(uint x, uint y) const {
    if (x < 0 || x >= width) { 
        std::cerr << "Column " << x << " out of bounds\n"; 
        return;
    }
    if (y < 0 || y >= height) { 
        std::cerr << "Row " << y << " out of bounds\n"; 
        return;
    }

    y = height - y - 1;
    size_t index = y*width + x;
    assert(index >= 0);
    assert(3*index + 2 < data.size());
}

void Image::getPixel(uint x, uint y, u_char& r, u_char& g, u_char& b) const {
    size_t index = get_index(x, y);
    r = data.at(3*index + 0);
    g = data.at(3*index + 1);
    b = data.at(3*index + 2);
}

void Image::getPixel(float u, float v, glm::vec3& clr) const {
    u = std::fmod(sgn(u)*u, 1.0f);
    v = std::fmod(sgn(v)*v, 1.0f);

    u_int i = u_int(u*width);
    u_int j = u_int(v*height);

    u_char r, g, b;
    getPixel(i, j, r, g, b);
    clr = glm::vec3(r*COLOR_SCALE, g*COLOR_SCALE, b*COLOR_SCALE);
}

void Image::setPixel(uint x, uint y, u_char r, u_char g, u_char b) {
    size_t index = get_index(x, y);
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