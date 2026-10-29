#pragma once

extern "C" {
    #include <GL/gl.h>
}
#include <cmath>
#include <type_traits>

template<typename T>
struct vec3_t {
    T x, y, z;

    constexpr vec3_t(T x, T y, T z);
    constexpr vec3_t(T v);
    constexpr vec3_t();

    constexpr static vec3_t<T> zero();
    constexpr static vec3_t<T> one();

    template<typename K> constexpr auto operator+(const vec3_t<K>& rhs) const;
    template<typename K> constexpr auto operator-(const vec3_t<K>& rhs) const;
    template<typename K> constexpr auto operator*(const vec3_t<K>& rhs) const;
    template<typename K> constexpr auto operator/(const vec3_t<K>& rhs) const;

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
    constexpr vec3_t<T> normalize(const vec3_t<T>& a);

    template<typename T>
    constexpr vec3_t<T> abs(const vec3_t<T>& a);
}

template<typename T, typename K> requires std::is_arithmetic_v<K>
constexpr auto operator*(K scale, const vec3_t<T>& vec);

// column-major: m[col*4 + row], upload with transpose = GL_FALSE
struct mat4_t {
    GLfloat m[16];

    constexpr GLfloat& at(size_t i, size_t j);
    constexpr const GLfloat& at(size_t i, size_t j) const;

    constexpr void insert_row(size_t i, vec3_t<GLfloat> row, size_t shift = 0);

    constexpr static mat4_t identity();
    constexpr static mat4_t look_at(vec3_t<GLfloat> eye, vec3_t<GLfloat> center, vec3_t<GLfloat> up);
    constexpr static mat4_t perspective(GLfloat fovy, GLfloat aspect, GLfloat n, GLfloat f);
    constexpr static mat4_t ortho(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f);

    constexpr mat4_t operator*(const mat4_t& rhs) const;

    // M * (p, 1), no perspective divide
    constexpr vec3_t<GLfloat> transform_point(const vec3_t<GLfloat>& p) const;
};

// cylindrical coordinates
struct cyl_t {
    GLfloat angle, radius, height;

    constexpr vec3_t<GLfloat> to_cart_z_up() const;
    constexpr vec3_t<GLfloat> to_cart_y_up() const;
};

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
constexpr vec3_t<T> vec3::normalize(const vec3_t<T>& a)
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

constexpr GLfloat& mat4_t::at(size_t i, size_t j)
{
    return m[4*j + i];
}
constexpr const GLfloat& mat4_t::at(size_t i, size_t j) const
{
    return m[4*j + i];
}

constexpr void mat4_t::insert_row(size_t i, vec3_t<GLfloat> row, size_t shift)
{
    assert(shift <= 1);
    at(i, shift+0) = row.x;
    at(i, shift+1) = row.y;
    at(i, shift+2) = row.z;
}

constexpr mat4_t mat4_t::identity()
{
    mat4_t r{};
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}

constexpr mat4_t mat4_t::look_at(vec3_t<GLfloat> eye, vec3_t<GLfloat> center, vec3_t<GLfloat> up)
{
    vec3_t<GLfloat> f = vec3::normalize(center - eye);
    vec3_t<GLfloat> s = vec3::normalize(vec3::cross(f, up));
    vec3_t<GLfloat> u = vec3::cross(s, f);
    vec3_t<GLfloat> b = -f;

    mat4_t r = identity();
    r.insert_row(0, s);
    r.insert_row(1, u);
    r.insert_row(2, b);
    r.at(0,3) = -vec3::dot(s, eye);
    r.at(1,3) = -vec3::dot(u, eye);
    r.at(2,3) = -vec3::dot(b, eye);
    return r;
}

constexpr mat4_t mat4_t::perspective(GLfloat fovy, GLfloat aspect, GLfloat n, GLfloat f)
{
    GLfloat t = std::tan(fovy / 2.0f);

    mat4_t r{};
    r.at(0,0) = 1.0f / (aspect * t);
    r.at(1,1) = 1.0f / t;
    r.at(2,2) = -(f + n) / (f - n);
    r.at(2,3) = -2.0f * f * n / (f - n);
    r.at(3,2) = -1.0f;
    return r;
}

constexpr mat4_t mat4_t::ortho(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f)
{
    mat4_t res = identity();
    res.at(0,0) =  2.0f / (r - l);
    res.at(1,1) =  2.0f / (t - b);
    res.at(2,2) = -2.0f / (f - n);
    res.at(0,3) = -(r + l) / (r - l);
    res.at(1,3) = -(t + b) / (t - b);
    res.at(2,3) = -(f + n) / (f - n);
    return res;
}

constexpr mat4_t mat4_t::operator*(const mat4_t& rhs) const
{
    mat4_t c{};
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            GLfloat sum = 0;
            for (int k = 0; k < 4; ++k) sum += m[k*4 + row] * rhs.m[col*4 + k];
            c.m[col*4 + row] = sum;
        }
    }
    return c;
}

constexpr vec3_t<GLfloat> mat4_t::transform_point(const vec3_t<GLfloat>& p) const
{
    return vec3_t<GLfloat>(
        at(0,0)*p.x + at(0,1)*p.y + at(0,2)*p.z + at(0,2),
        at(1,0)*p.x + at(1,1)*p.y + at(1,2)*p.z + at(1,2),
        at(2,0)*p.x + at(2,1)*p.y + at(2,2)*p.z + at(2,2)
    );
}

constexpr vec3_t<GLfloat> cyl_t::to_cart_z_up() const
{
    GLfloat a = radius * std::cos(angle);
    GLfloat b = radius * std::sin(angle);
    return vec3_t<GLfloat>(a, b, height);
}

constexpr vec3_t<GLfloat> cyl_t::to_cart_y_up() const
{
    GLfloat a = radius * std::cos(angle);
    GLfloat b = radius * std::sin(angle);
    return vec3_t<GLfloat>(a, height, b);
}
