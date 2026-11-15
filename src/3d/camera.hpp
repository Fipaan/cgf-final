/*
 * 3d/camera.hpp -
 *     Free look camera (yaw/pitch). Y is up, yaw = -pi/2 looks down -Z.
 *     Movement is left to the user, so the same camera works for free-fly and first-person.
 */
#pragma once

#include <algorithm>
#include <cmath>
#include <numbers>

#include <3d/math.hpp>

struct camera_t {
    vec3_t<gl::f32> pos;
    gl::f32 yaw   = -std::numbers::pi_v<gl::f32> / 2.0f;
    gl::f32 pitch = 0.0f;

    gl::f32 fovy   = 45.0f * std::numbers::pi_v<gl::f32> / 180.0f;
    gl::f32 z_near = 0.1f;
    gl::f32 z_far  = 200.0f;

    // radians per pixel
    gl::f32 sensitivity = 0.0025f;

    void look(gl::f32 dx, gl::f32 dy);

    vec3_t<gl::f32> forward() const;      // full view direction
    vec3_t<gl::f32> flat_forward() const; // view direction projected on XZ, unit length
    vec3_t<gl::f32> right() const;

    mat4_t view() const;
    mat4_t proj(gl::f32 aspect) const;
};

///// IMPLEMENTATION ///////////////////////////////////////////////////////////

inline void camera_t::look(gl::f32 dx, gl::f32 dy)
{
    constexpr gl::f32 limit = 89.0f * std::numbers::pi_v<gl::f32> / 180.0f;
    yaw   += dx * sensitivity;
    pitch  = std::clamp(pitch - dy * sensitivity, -limit, limit);
}

inline vec3_t<gl::f32> camera_t::forward() const
{
    return vec3_t<gl::f32>(
        std::cos(pitch) * std::cos(yaw),
        std::sin(pitch),
        std::cos(pitch) * std::sin(yaw)
    );
}

inline vec3_t<gl::f32> camera_t::flat_forward() const
{
    return vec3_t<gl::f32>(std::cos(yaw), 0.0f, std::sin(yaw));
}

inline vec3_t<gl::f32> camera_t::right() const
{
    return vec3::norm(vec3::cross(forward(), vec3_t<gl::f32>(0.0f, 1.0f, 0.0f)));
}

inline mat4_t camera_t::view() const
{
    return mat4_t::look_at(pos, pos + forward(), vec3_t<gl::f32>(0.0f, 1.0f, 0.0f));
}

inline mat4_t camera_t::proj(gl::f32 aspect) const
{
    return mat4_t::perspective(fovy, aspect, z_near, z_far);
}
