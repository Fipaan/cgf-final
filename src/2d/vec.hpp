/*
 * 2d/vec2.hpp -
 *     2D vectors related logic.
 */
#pragma once

#include <vector>
#include <cmath>
#include <type_traits>
#include <span>
#include <algorithm>

namespace math {

template<typename T>
struct vec2_t {
    union {
        struct {
            T x, y;
        };
        T v[2];
    };

    constexpr vec2_t(T x, T y);
    constexpr vec2_t(T v);
    constexpr vec2_t();

    constexpr static vec2_t<T> zero();
    constexpr static vec2_t<T> one();

    template<typename K> constexpr auto operator+(const vec2_t<K>& rhs) const;
    template<typename K> constexpr auto operator-(const vec2_t<K>& rhs) const;
    template<typename K> constexpr auto operator*(const vec2_t<K>& rhs) const;
    template<typename K> constexpr auto operator/(const vec2_t<K>& rhs) const;
    template<typename K> constexpr bool operator==(const vec2_t<K>& rhs) const;
    template<typename K> constexpr bool operator!=(const vec2_t<K>& rhs) const;

    template<typename K> constexpr vec2_t<T>& operator=(const vec2_t<K>& rhs);
    template<typename K> constexpr vec2_t<T>& operator+=(const vec2_t<K>& rhs);
    template<typename K> constexpr vec2_t<T>& operator-=(const vec2_t<K>& rhs);
    template<typename K> constexpr vec2_t<T>& operator*=(const vec2_t<K>& rhs);
    template<typename K> constexpr vec2_t<T>& operator/=(const vec2_t<K>& rhs);

    template<typename K> constexpr auto operator*(K scale) const;
    template<typename K> constexpr auto operator/(K scale) const;

    template<typename K>
    constexpr vec2_t<K> cast() const;

    template<typename K>
    constexpr vec2_t<T>& rot(T angle, const vec2_t<K> origin);
    constexpr vec2_t<T>& rot(T angle);

    constexpr T len() const;

    constexpr vec2_t<T> clone() const;
    static void spread(std::vector<T>& storage, std::span<vec2_t<T>> vecs);
    constexpr static vec2_t<T> make_angle(T angle);
};

namespace vec2 {
    template<typename T, typename K>
    constexpr auto dst_sqr(const vec2_t<T>& a, const vec2_t<K>& b);

    template<typename T, typename K>
    constexpr auto dst(const vec2_t<T>& a, const vec2_t<K>& b);

    template<typename T, typename K>
    constexpr auto angle(const vec2_t<T>& v1, const vec2_t<K>& v2);
}

template<typename T, typename K> requires std::is_arithmetic_v<K>
constexpr auto operator*(K scale, const vec2_t<T>& vec);

template<typename T, typename K> requires std::is_arithmetic_v<K>
constexpr auto operator/(K scale, const vec2_t<T>& vec);

///// IMPLEMENTATION ///////////////////////////////////////////////////////////

template<typename T>
constexpr vec2_t<T>::vec2_t(T x, T y)
{
    this->x = x;
    this->y = y;
}

template<typename T>
constexpr vec2_t<T>::vec2_t(T v)
{
    x = y = v;
}

template<typename T>
constexpr vec2_t<T>::vec2_t()
{
    x = y = 0;
}

template<typename T>
constexpr vec2_t<T> vec2_t<T>::zero() { return vec2_t<T>(0); }
template<typename T>
constexpr vec2_t<T> vec2_t<T>::one()  { return vec2_t<T>(1); }

template<typename T>
template<typename K>
constexpr auto vec2_t<T>::operator+(const vec2_t<K>& rhs) const
{
    using R = std::common_type_t<T, K>;
    return vec2_t<R>(x + rhs.x, y + rhs.y);
}

template<typename T>
template<typename K>
constexpr auto vec2_t<T>::operator-(const vec2_t<K>& rhs) const
{
    using R = std::common_type_t<T, K>;
    return vec2_t<R>(x - rhs.x, y - rhs.y);
}

template<typename T, typename K> requires std::is_arithmetic_v<K>
constexpr auto operator*(K scale, const vec2_t<T>& vec)
{
    return vec2_t<K>(scale)*vec;
}

template<typename T>
template<typename K>
constexpr auto vec2_t<T>::operator*(K scale) const
{
    auto vec = *this;
    return vec2_t<K>(scale)*vec;
}

template<typename T>
template<typename K>
constexpr auto vec2_t<T>::operator*(const vec2_t<K>& rhs) const
{
    using R = std::common_type_t<T, K>;
    return vec2_t<R>(x * rhs.x, y * rhs.y);
}

template<typename T, typename K> requires std::is_arithmetic_v<K>
constexpr auto operator/(K scale, const vec2_t<T>& vec)
{
    return vec2_t<K>(scale)/vec;
}

template<typename T>
template<typename K>
constexpr auto vec2_t<T>::operator/(K scale) const
{
    using R = std::common_type_t<T, K>;
    return (*this)/vec2_t<R>(scale);
}

template<typename T>
template<typename K>
constexpr auto vec2_t<T>::operator/(const vec2_t<K>& rhs) const
{
    using R = std::common_type_t<T, K>;
    return vec2_t<R>(x / rhs.x, y / rhs.y);
}

template<typename T>
template<typename K>
constexpr bool vec2_t<T>::operator==(const vec2_t<K>& rhs) const
{
    return x == rhs.x && y == rhs.y;
}
template<typename T>
template<typename K>
constexpr bool vec2_t<T>::operator!=(const vec2_t<K>& rhs) const
{
    return x != rhs.x || y != rhs.y;
}

template<typename T>
template<typename K>
constexpr vec2_t<T>& vec2_t<T>::operator=(const vec2_t<K>& rhs)
{
    x = rhs.x;
    y = rhs.y;
    return *this;
}

template<typename T>
template<typename K>
constexpr vec2_t<T>& vec2_t<T>::operator+=(const vec2_t<K>& rhs)
{
    x += rhs.x;
    y += rhs.y;
    return *this;
}

template<typename T>
template<typename K>
constexpr vec2_t<T>& vec2_t<T>::operator-=(const vec2_t<K>& rhs)
{
    x -= rhs.x;
    y -= rhs.y;
    return *this;
}

template<typename T>
template<typename K>
constexpr vec2_t<T>& vec2_t<T>::operator*=(const vec2_t<K>& rhs)
{
    x *= rhs.x;
    y *= rhs.y;
    return *this;
}

template<typename T>
template<typename K>
constexpr vec2_t<T>& vec2_t<T>::operator/=(const vec2_t<K>& rhs)
{
    x /= rhs.x;
    y /= rhs.y;
    return *this;
}

template<typename T>
constexpr T vec2_t<T>::len() const
{
    return vec2::dst(*this, zero());
}

template<typename T>
constexpr vec2_t<T> vec2_t<T>::clone() const
{
    return vec2_t(x, y);
}

template<typename T>
template<typename K>
constexpr vec2_t<K> vec2_t<T>::cast() const
{
    return vec2_t<K>((K)x, (K)y);
}

template<typename T>
template<typename K>
constexpr vec2_t<T>& vec2_t<T>::rot(T angle, const vec2_t<K> origin)
{
    auto dv = *this - origin;
    auto cos_a = std::cos(angle);
    auto sin_a = std::sin(angle);

    auto dv_x = dv.x*cos_a - dv.y*sin_a;
    auto dv_y = dv.x*sin_a + dv.y*cos_a;

    *this = origin;
    this->x += dv_x;
    this->y += dv_y;

    return *this;
}

template<typename T>
constexpr vec2_t<T>& vec2_t<T>::rot(T angle)
{
    return rot(angle, zero());
}

template<typename T>
void vec2_t<T>::spread(std::vector<T>& storage, std::span<vec2_t<T>> vecs)
{
    storage.resize(vecs.size() * 2);

    for (size_t i = 0; i < vecs.size(); ++i) {
        storage[2*i + 0] = vecs[i].x;
        storage[2*i + 1] = vecs[i].y;
    }
}

template<typename T>
constexpr vec2_t<T> vec2_t<T>::make_angle(T angle)
{
    return vec2_t<T>(std::cos(angle), std::sin(angle));
}

template<typename T, typename K>
constexpr auto vec2::dst_sqr(const vec2_t<T>& a, const vec2_t<K>& b)
{
    auto dx = a.x - b.x;
    auto dy = a.y - b.y;

    return dx * dx + dy * dy;
}

template<typename T, typename K>
constexpr auto vec2::dst(const vec2_t<T>& a, const vec2_t<K>& b)
{
    return std::sqrt(dst_sqr(a, b));
}

template<typename T, typename K>
constexpr auto vec2::angle(const vec2_t<T>& v1, const vec2_t<K>& v2)
{
    using R = std::common_type_t<T, K>;
    R result = std::acos((v1.x * v2.x + v1.y * v2.y) / (v1.len() * v2.len()));
    return result;
}

} // namespace math