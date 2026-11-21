/*
 * gl/flat.hpp -
 *     Convenient handler for flatening data.
 */
#pragma once

#include <core/gl/first.hpp>
#include <vector>
#include <span>

namespace gl {

template<typename T>
struct flat_t {
    std::vector<T> vec;

    void reset();
    void add(T value);
    void add(std::span<T> values);
    void add(const T *values, size_t count);

    T *data();
    size_t count();
};

} // namespace gl