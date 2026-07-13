#pragma once

#include <glm/gtc/random.hpp>

#include "../stn.hpp"
#include "umath.hpp"

// Could be adjusted to balance memory use and "randomness"
constexpr size_t RAND_GEN_SIZE = 4'096U; 

// Pseudo-random generation using lookup tables. Must be static or heap-allocated.
namespace prand 
{
    template <typename T>
    class genRand 
    {
    protected:
        // Because of multithreading, this is very random order
        std::atomic<size_t> i;
        T dataPoints[RAND_GEN_SIZE];
        virtual T generateRand() = 0;

    public:
        genRand() : i(0U) {}
        inline T rand() { i++; return dataPoints[i*(i+2673457) % RAND_GEN_SIZE]; }
        // Slightly faster than not providing an index
        inline T rand(size_t j) { return dataPoints[j % RAND_GEN_SIZE]; }
    };

    class uniformRand : public genRand<float> 
    {
    private:
        static constexpr float r_RAND_MAX = 1.0f / static_cast<float>(RAND_MAX);
        float generateRand() override {
            return static_cast<float>(std::rand()) * r_RAND_MAX;
        }
    public:
        uniformRand() {
            for (size_t i = 0; i < RAND_GEN_SIZE; ++i) {
                dataPoints[i] = generateRand();
            }
        }
    };

    class diskRand : public genRand<glm::vec2> 
    {
    private:
        glm::vec2 generateRand() override {
            return glm::diskRand(1.f);
        }
    public:
        diskRand() {
            for (size_t i = 0; i < RAND_GEN_SIZE; ++i) {
                dataPoints[i] = generateRand();
            }
        }
    };

    // class pmj02 {
    // private:

    // public:
    // };
}