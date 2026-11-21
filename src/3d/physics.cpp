#include "physics.hpp"

#include <algorithm>
#include <cmath>

using namespace phys;

bool body_t::overlap(const body_t& a, const body_t& b)
{
    for (int i = 0; i < 3; ++i) {
        gl::f32 d     = std::fabs(a.pos.v[i] - b.pos.v[i]);
        gl::f32 reach = a.half.v[i] + b.half.v[i];
        if (d >= reach - EPS) return false;
    }
    return true;
}

size_t world_t::add(body_t body)
{
    bodies.push_back(body);
    return bodies.size() - 1;
}

void world_t::update(gl::f32 dt)
{
    // avoids the spiral of death after a long stall
    accumulator += std::min(dt, 0.1f);
    while (accumulator >= step_dt) {
        step(step_dt);
        accumulator -= step_dt;
    }
}

void world_t::step(gl::f32 dt)
{
    for (size_t i = 0; i < bodies.size(); ++i) {
        body_t& b = bodies[i];
        if (b.is_static || !b.enabled) continue;

        b.on_ground = false;
        b.vel += gravity * dt;
        b.vel.y = std::max(b.vel.y, -TERMINAL_SPEED);

        for (int axis = 0; axis < 3; ++axis) {
            // velocity of this axis before any resolve zeroes it, all overlaps use it
            const gl::f32 v = b.vel.v[axis];
            b.pos.v[axis] += v * dt;

            for (size_t j = 0; j < bodies.size(); ++j) {
                if (j == i) continue;
                const body_t& o = bodies[j];
                if (!o.enabled || !body_t::overlap(b, o)) continue;

                // only motion along this axis can have caused the overlap, so push back against it
                if (v == 0.0f) continue;

                gl::f32 reach = b.half.v[axis] + o.half.v[axis];
                bool    up    = v > 0.0f;

                b.pos.v[axis] = o.pos.v[axis] + (up ? -reach : reach);
                if (axis == 1 && !up) b.on_ground = true;

                b.vel.v[axis] = -v * b.restitution;
            }
        }

        if (b.on_ground) {
            gl::f32 k = std::exp(-b.friction * dt);
            b.vel.x *= k;
            b.vel.z *= k;
        }
    }
}
