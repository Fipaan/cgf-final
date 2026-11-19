/*
 * 3d/vec.hpp -
 *     3D vectors related logic.
 */
#pragma once

#include <algorithm>
#include <cmath>
#include <type_traits>

namespace math {

template<typename T>
struct vec3_t {
    union {
        struct {
            T x, y, z;
        };
        T v[3];
    };

    constexpr vec3_t(T x, T y, T z);
    constexpr vec3_t(T v);
    constexpr vec3_t();

    constexpr static vec3_t<T> zero();
    constexpr static vec3_t<T> one();

    template<typename K> constexpr auto operator+(const vec3_t<K>& rhs) const;
    template<typename K> constexpr auto operator-(const vec3_t<K>& rhs) const;
    template<typename K> constexpr auto operator*(const vec3_t<K>& rhs) const;
    template<typename K> constexpr auto operator/(const vec3_t<K>& rhs) const;
    template<typename K> constexpr auto operator==(const vec3_t<K>& rhs) const;
    template<typename K> constexpr auto operator!=(const vec3_t<K>& rhs) const;

    template<typename K> constexpr vec3_t<T>& operator=(const vec3_t<K>& rhs);
    template<typename K> constexpr vec3_t<T>& operator+=(const vec3_t<K>& rhs);
    template<typename K> constexpr vec3_t<T>& operator-=(const vec3_t<K>& rhs);
    template<typename K> constexpr vec3_t<T>& operator*=(const vec3_t<K>& rhs);
    template<typename K> constexpr vec3_t<T>& operator/=(const vec3_t<K>& rhs);

    constexpr vec3_t<T> operator-() const;

    template<typename K> requires std::is_arithmetic_v<K>
    constexpr auto operator*(K scale) const;
    template<typename K> requires std::is_arithmetic_v<K>
    constexpr auto operator/(K scale) const;

    template<typename K>
    constexpr vec3_t<K> cast() const;

    constexpr vec3_t<T> clone() const;
};

namespace vec3 {
    template<typename T, typename K>
    constexpr auto dot(const vec3_t<T>& a, const vec3_t<K>& b);

    template<typename T, typename K>
    constexpr auto cross(const vec3_t<T>& a, const vec3_t<K>& b);

    template<typename T>
    constexpr T len(const vec3_t<T>& a);

    // zero vector stays zero (degenerate triangles)
    template<typename T>
    constexpr vec3_t<T> norm(const vec3_t<T>& a);

    template<typename T>
    constexpr vec3_t<T> abs(const vec3_t<T>& a);
}

template<typename T, typename K> requires std::is_arithmetic_v<K>
constexpr auto operator*(K scale, const vec3_t<T>& vec);

///// IMPLEMENTATION ///////////////////////////////////////////////////////////

template<typename T>
constexpr vec3_t<T>::vec3_t(T x, T y, T z)
{
    this->x = x;
    this->y = y;
    this->z = z;
}

template<typename T>
constexpr vec3_t<T>::vec3_t(T v)
{
    x = y = z = v;
}

template<typename T>
constexpr vec3_t<T>::vec3_t()
{
    x = y = z = 0;
}

template<typename T>
constexpr vec3_t<T> vec3_t<T>::zero() { return vec3_t<T>(0); }
template<typename T>
constexpr vec3_t<T> vec3_t<T>::one()  { return vec3_t<T>(1); }

template<typename T>
template<typename K>
constexpr auto vec3_t<T>::operator+(const vec3_t<K>& rhs) const
{
    using R = std::common_type_t<T, K>;
    return vec3_t<R>(x + rhs.x, y + rhs.y, z + rhs.z);
}

template<typename T>
template<typename K>
constexpr auto vec3_t<T>::operator-(const vec3_t<K>& rhs) const
{
    using R = std::common_type_t<T, K>;
    return vec3_t<R>(x - rhs.x, y - rhs.y, z - rhs.z);
}

template<typename T>
constexpr vec3_t<T> vec3_t<T>::operator-() const {
    return vec3_t<T>(-x, -y, -z);
}

template<typename T>
template<typename K>
constexpr auto vec3_t<T>::operator*(const vec3_t<K>& rhs) const
{
    using R = std::common_type_t<T, K>;
    return vec3_t<R>(x * rhs.x, y * rhs.y, z * rhs.z);
}

template<typename T>
template<typename K>
constexpr auto vec3_t<T>::operator/(const vec3_t<K>& rhs) const
{
    using R = std::common_type_t<T, K>;
    return vec3_t<R>(x / rhs.x, y / rhs.y, z / rhs.z);
}

template<typename T>
template<typename K>
constexpr auto vec3_t<T>::operator==(const vec3_t<K>& rhs) const
{
    return x == rhs.x && y == rhs.y && z == rhs.z;
}

template<typename T>
template<typename K>
constexpr auto vec3_t<T>::operator!=(const vec3_t<K>& rhs) const
{
    return x != rhs.x || y != rhs.y || z != rhs.z;
}

template<typename T>
template<typename K> requires std::is_arithmetic_v<K>
constexpr auto vec3_t<T>::operator*(K scale) const
{
    using R = std::common_type_t<T, K>;
    return (*this) * vec3_t<R>(scale);
}

template<typename T>
template<typename K> requires std::is_arithmetic_v<K>
constexpr auto vec3_t<T>::operator/(K scale) const
{
    using R = std::common_type_t<T, K>;
    return (*this) / vec3_t<R>(scale);
}

template<typename T, typename K> requires std::is_arithmetic_v<K>
constexpr auto operator*(K scale, const vec3_t<T>& vec)
{
    return vec * scale;
}

template<typename T>
template<typename K>
constexpr vec3_t<T>& vec3_t<T>::operator=(const vec3_t<K>& rhs)
{
    x = rhs.x;
    y = rhs.y;
    z = rhs.z;
    return *this;
}

template<typename T>
template<typename K>
constexpr vec3_t<T>& vec3_t<T>::operator+=(const vec3_t<K>& rhs)
{
    x += rhs.x;
    y += rhs.y;
    z += rhs.z;
    return *this;
}

template<typename T>
template<typename K>
constexpr vec3_t<T>& vec3_t<T>::operator-=(const vec3_t<K>& rhs)
{
    x -= rhs.x;
    y -= rhs.y;
    z -= rhs.z;
    return *this;
}

template<typename T>
template<typename K>
constexpr vec3_t<T>& vec3_t<T>::operator*=(const vec3_t<K>& rhs)
{
    x *= rhs.x;
    y *= rhs.y;
    z *= rhs.z;
    return *this;
}

template<typename T>
template<typename K>
constexpr vec3_t<T>& vec3_t<T>::operator/=(const vec3_t<K>& rhs)
{
    x /= rhs.x;
    y /= rhs.y;
    z /= rhs.z;
    return *this;
}

template<typename T>
template<typename K>
constexpr vec3_t<K> vec3_t<T>::cast() const
{
    return vec3_t<K>((K)x, (K)y, (K)z);
}

template<typename T>
constexpr vec3_t<T> vec3_t<T>::clone() const
{
    return vec3_t(x, y, z);
}

template<typename T, typename K>
constexpr auto vec3::dot(const vec3_t<T>& a, const vec3_t<K>& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

template<typename T, typename K>
constexpr auto vec3::cross(const vec3_t<T>& a, const vec3_t<K>& b)
{
    using R = std::common_type_t<T, K>;
    return vec3_t<R>(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

template<typename T>
constexpr T vec3::len(const vec3_t<T>& a)
{
    return std::sqrt(vec3::dot(a, a));
}

template<typename T>
constexpr vec3_t<T> vec3::norm(const vec3_t<T>& a)
{
    T l = vec3::len(a);
    if (l < (T)1e-12) return vec3_t<T>::zero();
    return a / l;
}

template<typename T>
constexpr vec3_t<T> vec3::abs(const vec3_t<T>& a)
{
    return vec3_t<T>(std::fabs(a.x), std::fabs(a.y), std::fabs(a.z));
}

} // namespace math