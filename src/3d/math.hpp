/*
 * 3d/math.hpp -
 *     Math related to 3D objects, such as matrices, and etc.
 */
#pragma once

#include <core/gl/first.hpp>
#include <3d/vec.hpp>
#include <cassert>
#include <cmath>
#include <type_traits>

namespace math {

// column-major: m[col*4 + row], upload with transpose = GL_FALSE
struct mat4_t {
    gl::f32 m[16];

    constexpr gl::f32& at(size_t i, size_t j);
    constexpr const gl::f32& at(size_t i, size_t j) const;

    constexpr void insert_row(size_t i, vec3_t<gl::f32> row, size_t shift = 0);

    constexpr static mat4_t id();
    // matrix to convert world coordinates into camera coordinates
    //     eye    - camera position
    //     center - point where camera is looking at
    //     up     - world up vector
    constexpr static mat4_t look_at(vec3_t<gl::f32> eye, vec3_t<gl::f32> center, vec3_t<gl::f32> up);
    // matrix to convert camera-space into clip-space coordinates from perspective view
    //     fovy   - vertical field of view
    //     aspect - width:height ratio
    //     near - near clip plane
    //     far  - far clip plane
    constexpr static mat4_t perspective(gl::f32 fovy, gl::f32 aspect, gl::f32 near, gl::f32 far);
    // matrix to convert camera-space into clip-space coordinates from orthographic view
    //     all parameters define bounding box of the world that's going to be rendered
    constexpr static mat4_t ortho(
        gl::f32 left, gl::f32 right, gl::f32 bottom, gl::f32 top,
        gl::f32 near, gl::f32 far
    );

    constexpr static mat4_t translate(vec3_t<gl::f32> t);
    constexpr static mat4_t scale(vec3_t<gl::f32> s);
    constexpr static mat4_t rotate_x(gl::f32 angle);
    constexpr static mat4_t rotate_y(gl::f32 angle);
    constexpr static mat4_t rotate_z(gl::f32 angle);
    // T * Ry * Rx * Rz * S, `rot` is euler angles in radians
    constexpr static mat4_t trs(vec3_t<gl::f32> pos, vec3_t<gl::f32> rot, vec3_t<gl::f32> scale);

    constexpr mat4_t operator*(const mat4_t& rhs) const;

    // M * (p, 1), no perspective divide
    constexpr vec3_t<gl::f32> transform_point(const vec3_t<gl::f32>& p) const;
};

// cylindrical coordinates
struct cyl_t {
    gl::f32 angle, radius, height;

    constexpr vec3_t<gl::f32> to_cart_z_up() const;
    constexpr vec3_t<gl::f32> to_cart_y_up() const;
};

///// IMPLEMENTATION ///////////////////////////////////////////////////////////

constexpr gl::f32& mat4_t::at(size_t i, size_t j)
{
    return m[4*j + i];
}
constexpr const gl::f32& mat4_t::at(size_t i, size_t j) const
{
    return m[4*j + i];
}

constexpr void mat4_t::insert_row(size_t i, vec3_t<gl::f32> row, size_t shift)
{
    assert(shift <= 1);
    at(i, shift+0) = row.x;
    at(i, shift+1) = row.y;
    at(i, shift+2) = row.z;
}

constexpr mat4_t mat4_t::id()
{
    mat4_t r{};
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}

constexpr mat4_t mat4_t::look_at(vec3_t<gl::f32> eye, vec3_t<gl::f32> center, vec3_t<gl::f32> up)
{
    vec3_t<gl::f32> f = vec3::norm(center - eye);
    vec3_t<gl::f32> s = vec3::norm(vec3::cross(f, up));
    vec3_t<gl::f32> u = vec3::cross(s, f);
    vec3_t<gl::f32> b = -f;

    mat4_t r = id();
    r.insert_row(0, s);
    r.insert_row(1, u);
    r.insert_row(2, b);
    r.at(0,3) = -vec3::dot(s, eye);
    r.at(1,3) = -vec3::dot(u, eye);
    r.at(2,3) = -vec3::dot(b, eye);
    return r;
}

constexpr mat4_t mat4_t::perspective(gl::f32 fovy, gl::f32 aspect, gl::f32 near, gl::f32 far)
{
    gl::f32 t = std::tan(fovy / 2.0f);

    mat4_t r{};
    r.at(0,0) = 1.0f / (aspect * t);
    r.at(1,1) = 1.0f / t;
    r.at(2,2) = -(far + near) / (far - near);
    r.at(2,3) = -2.0f * far * near / (far - near);
    r.at(3,2) = -1.0f;
    return r;
}

constexpr mat4_t mat4_t::ortho(
    gl::f32 left, gl::f32 right, gl::f32 bottom, gl::f32 top,
    gl::f32 near, gl::f32 far
) {
    mat4_t res = id();
    res.at(0,0) =  2.0f / (right - left);
    res.at(1,1) =  2.0f / (top - bottom);
    res.at(2,2) = -2.0f / (far - near);
    res.at(0,3) = -(right + left) / (right - left);
    res.at(1,3) = -(top + bottom) / (top - bottom);
    res.at(2,3) = -(far + near) / (far - near);
    return res;
}

constexpr mat4_t mat4_t::translate(vec3_t<gl::f32> t)
{
    mat4_t r = id();
    r.at(0,3) = t.x;
    r.at(1,3) = t.y;
    r.at(2,3) = t.z;
    return r;
}

constexpr mat4_t mat4_t::scale(vec3_t<gl::f32> s)
{
    mat4_t r = id();
    r.at(0,0) = s.x;
    r.at(1,1) = s.y;
    r.at(2,2) = s.z;
    return r;
}

constexpr mat4_t mat4_t::rotate_x(gl::f32 angle)
{
    gl::f32 c = std::cos(angle), s = std::sin(angle);
    mat4_t r = id();
    r.at(1,1) =  c; r.at(1,2) = -s;
    r.at(2,1) =  s; r.at(2,2) =  c;
    return r;
}

constexpr mat4_t mat4_t::rotate_y(gl::f32 angle)
{
    gl::f32 c = std::cos(angle), s = std::sin(angle);
    mat4_t r = id();
    r.at(0,0) =  c; r.at(0,2) =  s;
    r.at(2,0) = -s; r.at(2,2) =  c;
    return r;
}

constexpr mat4_t mat4_t::rotate_z(gl::f32 angle)
{
    gl::f32 c = std::cos(angle), s = std::sin(angle);
    mat4_t r = id();
    r.at(0,0) =  c; r.at(0,1) = -s;
    r.at(1,0) =  s; r.at(1,1) =  c;
    return r;
}

constexpr mat4_t mat4_t::trs(vec3_t<gl::f32> pos, vec3_t<gl::f32> rot, vec3_t<gl::f32> scale_)
{
    return translate(pos) * rotate_y(rot.y) * rotate_x(rot.x) * rotate_z(rot.z) * scale(scale_);
}

constexpr mat4_t mat4_t::operator*(const mat4_t& rhs) const
{
    mat4_t c{};
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            gl::f32 sum = 0;
            for (int k = 0; k < 4; ++k) {
                sum += at(row,k) * rhs.at(k,col);
            }
            c.at(row,col) = sum;
        }
    }
    return c;
}

constexpr vec3_t<gl::f32> mat4_t::transform_point(const vec3_t<gl::f32>& p) const
{
    return vec3_t<gl::f32>(
        at(0,0)*p.x + at(0,1)*p.y + at(0,2)*p.z + at(0,3),
        at(1,0)*p.x + at(1,1)*p.y + at(1,2)*p.z + at(1,3),
        at(2,0)*p.x + at(2,1)*p.y + at(2,2)*p.z + at(2,3)
    );
}

constexpr vec3_t<gl::f32> cyl_t::to_cart_z_up() const
{
    gl::f32 a = radius * std::cos(angle);
    gl::f32 b = radius * std::sin(angle);
    return vec3_t<gl::f32>(a, b, height);
}

constexpr vec3_t<gl::f32> cyl_t::to_cart_y_up() const
{
    gl::f32 a = radius * std::cos(angle);
    gl::f32 b = radius * std::sin(angle);
    return vec3_t<gl::f32>(a, height, b);
}

} // namespace math