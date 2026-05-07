// Custom vector and matrix mathematics for masochism reasons
// TODO: b/c opencl, it would make sense for me to use glm instead
// so might delete this later
#pragma once

template <typename T>
class vecN {
public:
    virtual vecN<T>& operator+=(const vecN<T>&) = 0; 
    friend virtual vecN<T> operator+(vecN<T>, const vecN<T>&) = 0;
};

