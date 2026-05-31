#pragma once

#include "stn.hpp"
#include "umath.hpp"

// Could be adjusted to balance memory use and randomness
constexpr size_t RAND_GEN_SIZE = 50'000U; 

// Pseudo-random generation using lookup tables. 
namespace prand 
{
    // Much faster than randomly generating for
    // each ray, and less noise. However, this 
    // means that sampling beyond 18 per instance
    // doesn't lead to better results
    constexpr size_t N = 18;
    // Thank you professor sueda
    static constexpr vec2 poissonDiskData[N] = {
        vec2(-0.220147, 0.976896),
        vec2(-0.735514, 0.693436),
        vec2(-0.200476, 0.310353),
        vec2( 0.180822, 0.454146),
        vec2( 0.292754, 0.937414),
        vec2( 0.564255, 0.207879),
        vec2( 0.178031, 0.024583),
        vec2( 0.613912,-0.205936),
        vec2(-0.385540,-0.070092),
        vec2( 0.962838, 0.378319),
        vec2(-0.886362, 0.032122),
        vec2(-0.466531,-0.741458),
        vec2( 0.006773,-0.574796),
        vec2(-0.739828,-0.410584),
        vec2( 0.590785,-0.697557),
        vec2(-0.081436,-0.963262),
        vec2( 1.000000,-0.100160),
        vec2( 0.622430, 0.680868)
    };
    constexpr inline vec2 poissonDisk(size_t index) { return poissonDiskData[index % N]; }

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
        genRand(size_t size) : genRand() { dataPoints.reserve(N); }
        inline T rand() { return dataPoints.at(i++ % dataPoints.size()); }
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

// Arbitrary size, but computing these numbers
// beforehand saves actual seconds
prand::uniformRand unifRandGen(RAND_GEN_SIZE);
prand::diskRand diskRandGen(RAND_GEN_SIZE);