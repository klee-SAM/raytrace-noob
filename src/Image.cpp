#include "Image.hpp"

#include "umath.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_FAILURE_STRINGS
#define STBI_FAILURE_USERMSG
#include "stb_image.h"

const float COLOR_SCALE = 1.0f / 255.0f;

// Filename (aka path) must be set before calling this
bool Image::loadFile(bool suppressNotFoundError) {
    // Load texture
	int w, h, ncomps;
    // get_index() flips the v coordinate, keep this false
	stbi_set_flip_vertically_on_load(false);
	u_char* raw_data = stbi_load(filename.c_str(), &w, &h, &ncomps, 3);
	if (!raw_data) {
        if (!suppressNotFoundError) {
            std::cerr << filename << " not found;\n" << stbi_failure_reason() << '\n';
        }
		return false;
	}
	if (ncomps != 3) {
		std::cerr << filename << " must have exactly 3 components (RGB).\n"
                  << "Image data not loaded.\n";
        return false;
	}

    // // Arbitrary for this raytracer, any reasonable non-zero lengths are fine 
	// if ((w & (w - 1)) != 0 || (h & (h - 1)) != 0) {
	// 	std::cerr << filename << " must be a power of 2; "
    //               << "only square images with power of 2 lengths are supported.\n"
    //               << "Image data not loaded.\n";
    //     return;
	// }

	this->width = w;
	this->height = h;
	this->comp = ncomps;

	#ifndef NDEBUG
	std::clog << "Loaded image with "
		 << w << " width, " << h << " height, and "
		 << ncomps << " channels\n"; 
	#endif
	
	// Copy the image data to a texture buffer
    size_t texBufSize = w * h * ncomps;
	this->data = std::vector<u_char>(w * h * ncomps, 0);
	for (size_t i = 0; i < texBufSize; i += ncomps) {
		for (int c = 0; c < ncomps; ++c) {
			this->data.at(i + c) = *(raw_data + i + c);
		}
	}

	// Free image, since the data is copied elsewhere
	stbi_image_free(raw_data);

    return true;
}

size_t Image::get_index(uint x, uint y) const {
    if (data.size() < 1) {
        std::cerr << "Image data not initialized\n";
        return 0;
    }
    if (x < 0 || x >= width) { 
        std::cerr << "Column " << x << " out of bounds\n"; 
        throw std::logic_error("");
        return 0;
    }
    if (y < 0 || y >= height) { 
        std::cerr << "Row " << y << " out of bounds\n"; 
        throw std::logic_error("");
        return 0;
    }

    y = height - y - 1;
    size_t index = y*width + x;
    assert(index >= 0);
    assert(3*index + 2 < data.size());

    return index;
}

void Image::getPixel(uint x, uint y, u_char& r, u_char& g, u_char& b) const {
    size_t index = get_index(x, y);
    r = data.at(3*index + 0);
    g = data.at(3*index + 1);
    b = data.at(3*index + 2);
}

void Image::getPixel(float u, float v, glm::vec3& clr) const {
    // u = std::fmod(sgn(u)*u, 1.0f);
    // v = std::fmod(sgn(u)*u, 1.0f);
    u = glm::fract(u);
    v = glm::fract(v);

    // u and v should be in [0, 1] before this
    assert(u >= 0.0f && v >= 0.0f);
    assert(u <= 1.0f && v <= 1.0f);
    // projected images are flipped across the y-axis 
    // and i can't be bothered, just "unflip" 
    u = 1.0f - u;

    u_int i = u_int(u*(width-1));
    u_int j = u_int(v*(height-1));

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