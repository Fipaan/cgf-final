#include "camera.hpp"

#include <algorithm>
#include <cmath>

using namespace math;

void camera_t::look(gl::f32 dx, gl::f32 dy)
{
    yaw   += dx * sensitivity;
    pitch  = std::clamp(pitch - dy * sensitivity, -UP_ANGLE_LIMIT, UP_ANGLE_LIMIT);
}

vec3_t<gl::f32> camera_t::forward() const
{
    return vec3_t<gl::f32>(
        std::cos(pitch) * std::cos(yaw),
        std::sin(pitch),
        std::cos(pitch) * std::sin(yaw)
    );
}

vec3_t<gl::f32> camera_t::flat_forward() const
{
    return vec3_t<gl::f32>(std::cos(yaw), 0.0f, std::sin(yaw));
}

vec3_t<gl::f32> camera_t::right() const
{
    return vec3::norm(vec3::cross(forward(), vec3_t<gl::f32>(0.0f, 1.0f, 0.0f)));
}

mat4_t camera_t::view() const
{
    return mat4_t::look_at(pos, pos + forward(), vec3_t<gl::f32>(0.0f, 1.0f, 0.0f));
}

mat4_t camera_t::proj(gl::f32 aspect) const
{
    return mat4_t::perspective(fovy, aspect, z_near, z_far);
}
