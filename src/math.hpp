#pragma once

extern "C" {
    #include <GL/gl.h>
}
#include <vector>
#include <cmath>

template<typename T>
struct vec2_t {
    T x, y;

    constexpr vec2_t(T x, T y);
    constexpr vec2_t(T v);
    constexpr vec2_t();

    constexpr static vec2_t<T> zero();
    constexpr static vec2_t<T> one();

    template<typename K> constexpr auto operator+(const vec2_t<K>& rhs) const;
    template<typename K> constexpr auto operator-(const vec2_t<K>& rhs) const;
    template<typename K> constexpr auto operator*(const vec2_t<K>& rhs) const;
    template<typename K> constexpr auto operator/(const vec2_t<K>& rhs) const;

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
    constexpr auto compute_angle(const vec2_t<T>& v1, const vec2_t<K>& v2);
}

template<typename T, typename K> requires std::is_arithmetic_v<K>
constexpr auto operator*(K scale, const vec2_t<T>& vec);

template<typename T, typename K> requires std::is_arithmetic_v<K>
constexpr auto operator/(K scale, const vec2_t<T>& vec);

struct hsva_t {
    GLfloat h, s, v, a;
};

struct color_t {
    GLfloat r, g, b, a;

    constexpr color_t(GLfloat r, GLfloat g, GLfloat b,  GLfloat a);
    constexpr color_t(uint32_t rgba);

    constexpr static color_t from_hex(uint32_t rgba);
    constexpr static color_t from_hsva(hsva_t hsva);

    constexpr hsva_t to_hsva() const;
};

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
    // TODO: rewrite in vec2::dst, it doesn't exist yet
    return std::sqrt(x*x + y*y);
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
    return rot(angle, vec2_t<T>::zero());
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
constexpr auto vec2::compute_angle(const vec2_t<T>& v1, const vec2_t<K>& v2)
{
    using R = std::common_type_t<T, K>;
    R result = std::acos((v1.x * v2.x + v1.y * v2.y) / (v1.len() * v2.len()));
    return result;
}

constexpr color_t::color_t(GLfloat r, GLfloat g, GLfloat b,  GLfloat a)
{
    this->r = r;
    this->g = g;
    this->b = b;
    this->a = a;
}

constexpr color_t::color_t(uint32_t rgba)
{
    r = ((GLfloat)((rgba >> 24) & 0xFF)) / 0xFF;
    g = ((GLfloat)((rgba >> 16) & 0xFF)) / 0xFF;
    b = ((GLfloat)((rgba >> 8 ) & 0xFF)) / 0xFF;
    a = ((GLfloat)((rgba >> 0 ) & 0xFF)) / 0xFF;
}

constexpr color_t color_t::from_hex(uint32_t rgba)
{
    return color_t(rgba);
}

constexpr hsva_t color_t::to_hsva() const
{
    GLfloat max = std::max({r, g, b});
    GLfloat min = std::min({r, g, b});
    GLfloat delta = max - min;

    hsva_t hsva;
    auto& [h, s, v, a] = hsva;

    v = max;
    s = (max == 0.0f) ? 0.0f : delta / max;

    if (delta == 0.0f) h = 0.0f;
    else if (max == r) {
        h = 60.0f * std::fmod( (g - b) / delta, 6.0f );
    } else if (max == g) {
        h = 60.0f * ( (b - r) / delta + 2.0f );
    } else {
        h = 60.0f * ( (r - g) / delta + 4.0f );
    }
    if (h < 0.0f) h += 360.0f;

    a = this->a;

    return hsva;
}

constexpr color_t color_t::from_hsva(hsva_t hsva)
{
    auto& [h, s, v, a] = hsva;
    h = std::fmod(h, 360.0f);
    if (h < 0.0f) h += 360.0f;

    s = std::clamp(s, 0.0f, 1.0f);
    v = std::clamp(v, 0.0f, 1.0f);

    GLfloat c = v * s;
    GLfloat x = c * (1.0f - std::fabs(std::fmod(h/60.0f, 2.0f) - 1.0f));
    GLfloat m = v - c;

    GLfloat r, g, b;
    if (h < 60.0f) {
        r = c;
        g = x;
        b = 0.0f;
    } else if (h < 120.0f) {
        r = x;
        g = c;
        b = 0.0f;
    } else if (h < 180.0f) {
        r = 0.0f;
        g = c;
        b = x;
    } else if (h < 240.0f) {
        r = 0.0f;
        g = x;
        b = c;
    } else if (h < 300.0f) {
        r = x;
        g = 0.0f;
        b = c;
    } else {
        r = c;
        g = 0.0f;
        b = x;
    }

    return color_t(r + m, g + m, b + m, a);
}
