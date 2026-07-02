#pragma once
#include "../stn.hpp"

namespace CounterCmps {
    // I like when VSCode freaks out on templates, lol
    static constexpr auto vec3_cmp = [](const vec3& a, const vec3& b) {
        return a.x < b.x && a.y < b.y && a.z < b.z;
    };
};

// For early-exit termination; accumulates a running average
// and variance. https://stackoverflow.com/questions/5147378/
template <typename T>
class VarianceCounter {
private:
    T mean, m2;
    float samplesDone;
public:
    static constexpr T EPSI2 = T(0.05f);

    VarianceCounter() : mean(T(0)), m2(T(0)), samplesDone(0.f) {}
    inline T getMean() const { return mean; }
    inline float getSamplesDone() const {return samplesDone; }
    // Adds contrib to the mean counter and returns whether the
    // variance is less than the threshold EPSI2.
    template <typename Func = std::function<bool(const T&, const T&)> >
    inline bool add(T contrib, Func cmp = [](const T& a, const T& b) { return a < b; }) {
        samplesDone++;
        const float r_sampDone = 1.f / samplesDone;
        const T prev_mean = mean;
        mean += (contrib - prev_mean) * r_sampDone;
        m2 += (contrib - prev_mean) * (contrib - mean);

        const T vari = m2 * r_sampDone;
        return cmp(vari, EPSI2);
    }
};