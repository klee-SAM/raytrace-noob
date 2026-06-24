#pragma once

#include "../stn.hpp"
#include "umath.hpp"

// Could be adjusted to balance memory use and randomness
constexpr size_t RAND_GEN_SIZE = 50'000U; 

// Pseudo-random generation using lookup tables. 
namespace prand 
{
    template <typename T>
    class genRand 
    {
    protected:
        // Because of multithreading, this is very random order
        std::atomic<size_t> i;
        std::vector<T> dataPoints;
        virtual T generateRand() = 0;

    public:
        genRand() : i(0U) {}
        genRand(size_t size) : genRand() { dataPoints.reserve(size); }
        inline T rand() { i++; return dataPoints.at(i*(i+2673457) % dataPoints.size()); }
        // Slightly faster than not providing an index
        inline T rand(size_t j) { return dataPoints.at(j % dataPoints.size()); }
    };

    class uniformRand : public genRand<float> 
    {
    private:
        static constexpr float r_RAND_MAX = 1.0f / static_cast<float>(RAND_MAX);
        float generateRand() override {
            return static_cast<float>(std::rand()) * r_RAND_MAX;
        }
    public:
        uniformRand(size_t N) {
            for (size_t i = 0; i < N; ++i) {
                dataPoints.push_back(generateRand());
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
        diskRand(size_t N) {
            for (size_t i = 0; i < N; ++i) {
                dataPoints.push_back(generateRand());
            }
        }
    };

    // class pmj02 {
    // private:

    // public:
    // };
}