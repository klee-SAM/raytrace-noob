#pragma once

#include <stdexcept>

// Naive helper class for "backporting"
// std::optional to C++14. Defaults constructs
// class T anyways, but sets is_init to false
// only when default constructed

// this is very useless

template <class T>
class optional {
public:
    optional() noexcept : is_init{false} {}
    optional(T obj) : val{obj}, is_init{true} {}
    virtual ~optional() = default;

    optional<T>& operator=(const T& val) { 
        this->val = val; 
        this->is_init = true; 
        return *this;    
    }

    constexpr T value_or(const T& oth) const { return is_init ? val : oth; }
    constexpr T& value() & {
        if (!is_init) throw std::logic_error(".value() : value not set");
        else return val;
    }
    constexpr const T& value() const & { return value(); }
    constexpr bool has_value() const { return is_init; }

private:
    T val;
    bool is_init;
};