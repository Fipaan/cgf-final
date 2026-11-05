/*
 * camera.hpp -
 *     Free look camera (yaw/pitch). Y is up, yaw = -pi/2 looks down -Z.
 *     Movement is left to the user, so the same camera works for free-fly and first-person.
 */
#pragma once

#include <algorithm>
#include <cmath>
#include <numbers>

#include <math/3d.hpp>

struct camera_t {
    vec3_t<GLfloat> pos;
    GLfloat yaw   = -std::numbers::pi_v<GLfloat> / 2.0f;
    GLfloat pitch = 0.0f;

    GLfloat fovy   = 45.0f * std::numbers::pi_v<GLfloat> / 180.0f;
    GLfloat z_near = 0.1f;
    GLfloat z_far  = 200.0f;

    // radians per pixel
    GLfloat sensitivity = 0.0025f;

    void look(GLfloat dx, GLfloat dy);

    vec3_t<GLfloat> forward() const;      // full view direction
    vec3_t<GLfloat> flat_forward() const; // view direction projected on XZ, unit length
    vec3_t<GLfloat> right() const;

    mat4_t view() const;
    mat4_t proj(GLfloat aspect) const;
};

///// IMPLEMENTATION ///////////////////////////////////////////////////////////

inline void camera_t::look(GLfloat dx, GLfloat dy)
{
    constexpr GLfloat limit = 89.0f * std::numbers::pi_v<GLfloat> / 180.0f;
    yaw   += dx * sensitivity;
    pitch  = std::clamp(pitch - dy * sensitivity, -limit, limit);
}

inline vec3_t<GLfloat> camera_t::forward() const
{
    return vec3_t<GLfloat>(
        std::cos(pitch) * std::cos(yaw),
        std::sin(pitch),
        std::cos(pitch) * std::sin(yaw)
    );
}

inline vec3_t<GLfloat> camera_t::flat_forward() const
{
    return vec3_t<GLfloat>(std::cos(yaw), 0.0f, std::sin(yaw));
}

inline vec3_t<GLfloat> camera_t::right() const
{
    return vec3::normalize(vec3::cross(forward(), vec3_t<GLfloat>(0.0f, 1.0f, 0.0f)));
}

inline mat4_t camera_t::view() const
{
    return mat4_t::look_at(pos, pos + forward(), vec3_t<GLfloat>(0.0f, 1.0f, 0.0f));
}

inline mat4_t camera_t::proj(GLfloat aspect) const
{
    return mat4_t::perspective(fovy, aspect, z_near, z_far);
}
