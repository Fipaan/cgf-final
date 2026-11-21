#include "flat.hpp"

using namespace gl;

template<typename T>
void flat_t<T>::reset()
{
    vec.clear();
}

template<typename T>
void flat_t<T>::add(T value)
{
    vec.push_back(value);
}

template<typename T>
void flat_t<T>::add(std::span<T> values)
{
    for (const auto& value : values) {
        vec.push_back(value);
    }
}

template<typename T>
void flat_t<T>::add(const T *values, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        vec.push_back(values[i]);
    }
}

template<typename T>
T *flat_t<T>::data()
{
    return vec.data();
}

template<typename T>
size_t flat_t<T>::count()
{
    return vec.size();
}

template struct gl::flat_t<gl::f32>;
