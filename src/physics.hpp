/*
 * physics.hpp -
 *     Minimal rigid body physics: gravity + AABB collisions. Header only.
 */
#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

#include <math/3d.hpp>

/* Usage:
```cpp
phys::world_t world;
size_t ground = world.add({.pos = {0, -5, 0}, .half = {50, 5, 50}, .is_static = true});
size_t crate  = world.add({.pos = {0,  3, 0}, .half = {0.5f, 0.5f, 0.5f}});

// every frame
world.update(dt);
vec3_t<GLfloat> p = world.bodies[crate].pos;
```
*/
namespace phys {
    constexpr GLfloat TERMINAL_SPEED = 50.0f;
    // contacts closer than this are not counted as overlap (resting contact stays stable)
    constexpr GLfloat EPS = 1e-4f;

    struct body_t {
        vec3_t<GLfloat> pos;  // centre
        vec3_t<GLfloat> half; // half extents
        vec3_t<GLfloat> vel;
        GLfloat restitution = 0.0f; // 0 = no bounce, 1 = perfect bounce
        GLfloat friction    = 4.0f; // horizontal drag while on ground, 1/s
        bool is_static  = false;    // never moves, still collides
        bool enabled    = true;     // disabled bodies are skipped completely
        bool on_ground  = false;    // set by `world_t::step`
    };

    struct world_t {
        std::vector<body_t> bodies;
        vec3_t<GLfloat> gravity = vec3_t<GLfloat>(0.0f, -9.81f, 0.0f);
        GLfloat step_dt     = 1.0f / 120.0f;
        GLfloat accumulator = 0.0f;

        size_t add(body_t body);
        // fixed timestep, safe to call with the frame time
        void update(GLfloat dt);
        void step(GLfloat dt);
    };

    bool overlap(const body_t& a, const body_t& b);
}

///// IMPLEMENTATION ///////////////////////////////////////////////////////////

namespace phys::detail {
    inline GLfloat& comp(vec3_t<GLfloat>& v, int i)
    {
        return i == 0 ? v.x : (i == 1 ? v.y : v.z);
    }
    inline GLfloat comp(const vec3_t<GLfloat>& v, int i)
    {
        return i == 0 ? v.x : (i == 1 ? v.y : v.z);
    }
}

inline bool phys::overlap(const body_t& a, const body_t& b)
{
    using detail::comp;
    for (int i = 0; i < 3; ++i) {
        GLfloat d     = std::fabs(comp(a.pos, i) - comp(b.pos, i));
        GLfloat reach = comp(a.half, i) + comp(b.half, i);
        if (d >= reach - EPS) return false;
    }
    return true;
}

inline size_t phys::world_t::add(body_t body)
{
    bodies.push_back(body);
    return bodies.size() - 1;
}

inline void phys::world_t::update(GLfloat dt)
{
    // avoids the spiral of death after a long stall
    accumulator += std::min(dt, 0.1f);
    while (accumulator >= step_dt) {
        step(step_dt);
        accumulator -= step_dt;
    }
}

// semi-implicit Euler, each axis is moved and resolved separately
inline void phys::world_t::step(GLfloat dt)
{
    using detail::comp;

    for (size_t i = 0; i < bodies.size(); ++i) {
        body_t& b = bodies[i];
        if (b.is_static || !b.enabled) continue;

        b.on_ground = false;
        b.vel += gravity * dt;
        b.vel.y = std::max(b.vel.y, -TERMINAL_SPEED);

        for (int axis = 0; axis < 3; ++axis) {
            // velocity of this axis before any resolve zeroes it, all overlaps use it
            const GLfloat v = comp(b.vel, axis);
            comp(b.pos, axis) += v * dt;

            for (size_t j = 0; j < bodies.size(); ++j) {
                if (j == i) continue;
                const body_t& o = bodies[j];
                if (!o.enabled || !overlap(b, o)) continue;

                // only motion along this axis can have caused the overlap, so push back against it
                if (v == 0.0f) continue;

                GLfloat reach = comp(b.half, axis) + comp(o.half, axis);
                bool    up    = v > 0.0f;

                comp(b.pos, axis) = comp(o.pos, axis) + (up ? -reach : reach);
                if (axis == 1 && !up) b.on_ground = true;
                comp(b.vel, axis) = -v * b.restitution;
            }
        }

        if (b.on_ground) {
            GLfloat k = std::exp(-b.friction * dt);
            b.vel.x *= k;
            b.vel.z *= k;
        }
    }
}
