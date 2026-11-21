/*
 * 3d/physics.hpp -
 *     Minimal rigid body physics: gravity + AABB collisions.
 */
#pragma once

#include <vector>

#include <core/gl/first.hpp>
#include <3d/math.hpp>

/* Usage:
```cpp
phys::world_t world;
size_t ground = world.add({.pos = {0, -5, 0}, .half = {50, 5, 50}, .is_static = true});
size_t crate  = world.add({.pos = {0,  3, 0}, .half = {0.5f, 0.5f, 0.5f}});

// every frame
world.update(dt);
vec3_t<gl::f32> p = world.bodies[crate].pos;
```
*/
namespace phys {

using namespace math;

constexpr gl::f32 TERMINAL_SPEED = 50.0f;
// contacts closer than this are not counted as overlap (resting contact stays stable)
constexpr gl::f32 EPS = 1e-4f;

struct body_t {
    vec3_t<gl::f32> pos;  // centre
    vec3_t<gl::f32> half; // half extents
    vec3_t<gl::f32> vel;
    gl::f32 restitution = 0.0f;  // 0 = no bounce, 1 = perfect bounce
    gl::f32 friction    = 4.0f;  // horizontal drag while on ground, 1/s
    gl::f32 mass        = 1.0f;  // used by `rigid_world_t` when the body is pushed by rigid bodies
    bool is_static      = false; // never moves, still collides
    bool enabled        = true;  // disabled bodies are skipped completely
    bool on_ground      = false; // set by `world_t::step`

    static bool overlap(const body_t& a, const body_t& b);
};

struct world_t {
    std::vector<body_t> bodies;
    vec3_t<gl::f32> gravity = vec3_t<gl::f32>(0.0f, -9.81f, 0.0f);
    gl::f32 step_dt     = 1.0f / 120.0f;
    gl::f32 accumulator = 0.0f;

    size_t add(body_t body);
    // fixed timestep, safe to call with the frame time
    void update(gl::f32 dt);
    // semi-implicit Euler, each axis is moved and resolved separately
    void step(gl::f32 dt);
};

} // namespace phys
