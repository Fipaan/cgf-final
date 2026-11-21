/*
 * 3d/camera.hpp -
 *     Free look camera (yaw/pitch). Y is up, yaw = -pi/2 looks down -Z.
 *     Movement is left to the user, so the same camera works for free-fly and first-person.
 */
#pragma once

#include <core/gl/const.hpp>
#include <3d/vec.hpp>
#include <3d/math.hpp>

namespace math {

struct camera_t {
    static constexpr gl::f32 UP_ANGLE_LIMIT_IN_DEGREES = 89.99f;
    static constexpr gl::f32 UP_ANGLE_LIMIT = UP_ANGLE_LIMIT_IN_DEGREES * gl::PI / 180.0f;

    vec3_t<gl::f32> pos;
    gl::f32 yaw   = -gl::PI / 2.0f;
    gl::f32 pitch = 0.0f;

    gl::f32 fovy   = 45.0f * gl::PI / 180.0f;
    gl::f32 z_near = 0.1f;
    gl::f32 z_far  = 200.0f;

    // radians per pixel
    gl::f32 sensitivity = 0.0025f;

    void look(gl::f32 dx, gl::f32 dy);

    // full view direction
    vec3_t<gl::f32> forward() const;
    // view direction projected on XZ, unit length (ignoring pitch)
    vec3_t<gl::f32> flat_forward() const;
    vec3_t<gl::f32> right() const;

    // matrix to convert world coordinates into camera coordinates
    mat4_t view() const;
    mat4_t proj(gl::f32 aspect) const;
};

} // namespace math
