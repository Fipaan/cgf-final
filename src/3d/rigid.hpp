/*
 * 3d/rigid.hpp -
 *     Oriented rigid bodies (boxes and spheres) with gravity, friction, torque and rotation, so things
 *     tip over, roll and come to rest the way they would in real life.
 *     Static geometry and the player are still the AABB bodies of `phys::world_t`, rigid bodies collide
 *     with both (the player is pushed back by what it pushes).
 */
#pragma once

#include <vector>

#include <core/gl/first.hpp>
#include <3d/math.hpp>
#include <3d/physics.hpp>

/* Usage:
```cpp
phys::world_t world;       // static boxes + player
phys::rigid_world_t rigids;
size_t crate = rigids.add({.shape = phys::shape_t::box, .pos = {0, 3, 0}, .half = {0.5f, 0.5f, 0.5f}, .mass = 20.0f});

// every fixed step, `player` is an index into `world.bodies` (or -1)
world.step(dt);
rigids.step(dt, world, player);
mat4_t model = mat4_t::translate(rigids.bodies[crate].pos) * rigids.bodies[crate].orient;
```
*/
namespace phys {
    enum class shape_t { box, sphere };

    struct rigid_t {
        shape_t shape = shape_t::box;
        vec3_t<gl::f32> pos;
        vec3_t<gl::f32> half;   // box half extents, the sphere radius is `half.x`
        mat4_t orient = mat4_t::id(); // only the 3x3 part is used
        vec3_t<gl::f32> vel;
        vec3_t<gl::f32> angvel; // world axes, rad/s
        gl::f32 mass        = 1.0f;
        gl::f32 restitution = 0.3f;
        gl::f32 friction    = 0.6f; // Coulomb coefficient
        bool enabled = true;
        bool held    = false;   // driven from outside: never falls asleep, spin is damped

        gl::f32 rest_time = 0.0f; // set by `step`
        bool    touching  = false;
    };

    // a pair that pressed on each other during the last `step`
    struct touch_t {
        enum kind_t { rigid, fixed, player };
        int     a;       // rigid body
        kind_t  kind;    // what it touched
        int     b;       // rigid: index into `bodies`, fixed: index into the AABB world, player: -1
        gl::f32 impulse; // normal impulse the solver needed, how hard the touch was
    };

    struct rigid_world_t {
        std::vector<rigid_t> bodies;
        std::vector<touch_t> touches; // rebuilt by every `step`
        vec3_t<gl::f32> gravity = vec3_t<gl::f32>(0.0f, -9.81f, 0.0f);
        int iterations = 10;

        size_t add(rigid_t body);
        void step(gl::f32 dt, world_t& statics, int player);
    };

    // 3x3 part of `R` applied to `v`, and its transpose (the inverse, as long as `R` is a rotation)
    vec3_t<gl::f32> rotate(const mat4_t& R, const vec3_t<gl::f32>& v);
    vec3_t<gl::f32> unrotate(const mat4_t& R, const vec3_t<gl::f32>& v);
    // rotation by `angle` around the unit `axis`
    mat4_t axis_angle(vec3_t<gl::f32> axis, gl::f32 angle);
    // pushes the basis back to a proper rotation after many small ones
    void orthonormalize(mat4_t& orient);
}
