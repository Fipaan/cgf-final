/*
 * 3d/rigid.hpp -
 *     Oriented rigid bodies (boxes and spheres) with gravity, friction, torque and rotation, so things
 *     tip over, roll and come to rest the way they would in real life. Header only.
 *     Static geometry and the player are still the AABB bodies of `phys::world_t`, rigid bodies collide
 *     with both (the player is pushed back by what it pushes).
 */
#pragma once

#include <algorithm>
#include <cmath>
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
        mat4_t orient = mat4_t::identity(); // only the 3x3 part is used
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

    struct rigid_world_t {
        std::vector<rigid_t> bodies;
        vec3_t<gl::f32> gravity = vec3_t<gl::f32>(0.0f, -9.81f, 0.0f);
        int iterations = 10;

        size_t add(rigid_t body);
        void step(gl::f32 dt, world_t& statics, int player);
    };

    // rotation by `angle` around the unit `axis`
    mat4_t axis_angle(vec3_t<gl::f32> axis, gl::f32 angle);
    // pushes the basis back to a proper rotation after many small ones
    void orthonormalize(mat4_t& orient);
}

///// IMPLEMENTATION ///////////////////////////////////////////////////////////

namespace phys::detail {
    using v3 = vec3_t<gl::f32>;
    using gl::f32;

    constexpr f32 SLOP           = 0.005f; // overlap that is left alone
    constexpr f32 BETA           = 0.2f;   // how fast overlap is pushed out
    constexpr f32 MAX_PUSH       = 2.0f;   // m/s
    constexpr f32 REST_THRESHOLD = 1.0f;   // slower impacts don't bounce
    constexpr f32 MAX_PLAYER_KICK = 4.0f;  // m/s, most a single step of contacts can add to the player's velocity
    constexpr f32 MAX_SPEED      = 15.0f;  // keeps thin boards from being skipped
    constexpr f32 MAX_SPIN       = 30.0f;
    constexpr f32 ROLL_DAMPING   = 4.0f;   // 1/s, only while touching something
    constexpr f32 AIR_DAMPING    = 0.05f;
    constexpr f32 SLEEP_SPEED    = 0.05f;
    constexpr f32 SLEEP_SPIN     = 0.15f;
    constexpr f32 SLEEP_TIME     = 0.5f;

    struct m3_t { f32 m[3][3] = {}; };

    inline v3 col(const mat4_t& R, int c) { return v3(R.at(0, c), R.at(1, c), R.at(2, c)); }
    inline v3 rotate(const mat4_t& R, const v3& v)   { return col(R, 0) * v.x + col(R, 1) * v.y + col(R, 2) * v.z; }
    inline v3 unrotate(const mat4_t& R, const v3& v) { return v3(vec3::dot(col(R, 0), v), vec3::dot(col(R, 1), v), vec3::dot(col(R, 2), v)); }
    inline v3 mul(const m3_t& a, const v3& v)
    {
        return v3(a.m[0][0]*v.x + a.m[0][1]*v.y + a.m[0][2]*v.z,
                  a.m[1][0]*v.x + a.m[1][1]*v.y + a.m[1][2]*v.z,
                  a.m[2][0]*v.x + a.m[2][1]*v.y + a.m[2][2]*v.z);
    }

    // what the collision tests need to know about any shape
    struct view_t {
        shape_t shape;
        v3 pos, half;
        const mat4_t *R;
    };

    // contact between A and B, `normal` points from A to B
    struct hit_t {
        v3  point, normal;
        f32 depth;
    };

    inline const mat4_t& identity()
    {
        static const mat4_t I = mat4_t::identity();
        return I;
    }

    inline f32 bound_radius(const view_t& s)
    {
        return s.shape == shape_t::sphere ? s.half.x : vec3::len(s.half);
    }

    inline void corners(const view_t& s, v3 out[8])
    {
        for (int i = 0; i < 8; ++i) {
            out[i] = s.pos + rotate(*s.R, v3((i & 1) ? s.half.x : -s.half.x,
                                             (i & 2) ? s.half.y : -s.half.y,
                                             (i & 4) ? s.half.z : -s.half.z));
        }
    }

    // Separating axis test on the six face normals. The axis with the least overlap decides the contact
    // normal for the whole pair, the contact points are the corners of the other box that went through that
    // face. (Edge against edge is not detected, that is enough for things to rest, tip and topple.)
    inline void box_box(const view_t& A, const view_t& B, std::vector<hit_t>& out)
    {
        const v3 ax[2][3] = {{col(*A.R, 0), col(*A.R, 1), col(*A.R, 2)}, {col(*B.R, 0), col(*B.R, 1), col(*B.R, 2)}};
        const f32 hv[2][3] = {{A.half.x, A.half.y, A.half.z}, {B.half.x, B.half.y, B.half.z}};
        const v3  d = B.pos - A.pos;

        f32 best = 1e30f;
        int ref_box = 0, ref_axis = 0;
        for (int b = 0; b < 2; ++b) {
            for (int i = 0; i < 3; ++i) {
                const v3& a = ax[b][i];
                f32 rA = 0.0f, rB = 0.0f;
                for (int k = 0; k < 3; ++k) {
                    rA += std::fabs(vec3::dot(a, ax[0][k])) * hv[0][k];
                    rB += std::fabs(vec3::dot(a, ax[1][k])) * hv[1][k];
                }
                const f32 overlap = rA + rB - std::fabs(vec3::dot(d, a));
                if (overlap <= 0.0f) return;
                if (overlap < best - 1e-4f) { best = overlap; ref_box = b; ref_axis = i; }
            }
        }

        const view_t& ref = ref_box == 0 ? A : B;
        const view_t& inc = ref_box == 0 ? B : A;
        v3 n = ax[ref_box][ref_axis]; // out of `ref`, towards `inc`
        if (vec3::dot(inc.pos - ref.pos, n) < 0.0f) n = -n;
        const v3 normal = ref_box == 0 ? n : -n; // A -> B
        const int u = (ref_axis + 1) % 3, w = (ref_axis + 2) % 3;

        v3 c[8];
        corners(inc, c);
        const size_t before = out.size();
        for (const v3& p : c) {
            const v3  rel   = p - ref.pos;
            const f32 depth = hv[ref_box][ref_axis] - vec3::dot(rel, n);
            if (depth <= 0.0f) continue;
            if (std::fabs(vec3::dot(rel, ax[ref_box][u])) > hv[ref_box][u] + 1e-3f) continue;
            if (std::fabs(vec3::dot(rel, ax[ref_box][w])) > hv[ref_box][w] + 1e-3f) continue;
            out.push_back({p, normal, depth});
        }
        // overlapping without a corner through the face (crossing boxes)
        if (out.size() == before) out.push_back({(ref.pos + inc.pos) * 0.5f, normal, best});
    }

    inline void sphere_box(const view_t& S, const view_t& B, bool sphere_is_a, std::vector<hit_t>& out)
    {
        const f32 r = S.half.x;
        const v3  q = unrotate(*B.R, S.pos - B.pos);
        const v3  c(std::clamp(q.x, -B.half.x, B.half.x), std::clamp(q.y, -B.half.y, B.half.y), std::clamp(q.z, -B.half.z, B.half.z));
        const v3  d = q - c;
        const f32 dist2 = vec3::dot(d, d);

        v3 n_local, point;
        f32 depth;
        if (dist2 > 1e-10f) {
            if (dist2 >= r * r) return;
            const f32 dist = std::sqrt(dist2);
            n_local = d * (1.0f / dist);
            depth   = r - dist;
            point   = B.pos + rotate(*B.R, c);
        } else {
            // the centre is inside the box, leave through the nearest face
            const f32 qv[3] = {q.x, q.y, q.z};
            const f32 hv[3] = {B.half.x, B.half.y, B.half.z};
            int axis = 0;
            f32 best = 1e30f;
            for (int i = 0; i < 3; ++i) {
                const f32 pen = hv[i] - std::fabs(qv[i]);
                if (pen < best) { best = pen; axis = i; }
            }
            f32 nv[3] = {0, 0, 0};
            nv[axis] = qv[axis] >= 0.0f ? 1.0f : -1.0f;
            n_local = v3(nv[0], nv[1], nv[2]);
            depth   = best + r;
            point   = S.pos;
        }
        const v3 n_box_to_sphere = rotate(*B.R, n_local);
        out.push_back({point, sphere_is_a ? -n_box_to_sphere : n_box_to_sphere, depth});
    }

    inline void sphere_sphere(const view_t& A, const view_t& B, std::vector<hit_t>& out)
    {
        const v3  d = B.pos - A.pos;
        const f32 reach = A.half.x + B.half.x;
        const f32 dist2 = vec3::dot(d, d);
        if (dist2 >= reach * reach) return;
        const f32 dist = std::sqrt(dist2);
        const v3  n = dist > 1e-6f ? d * (1.0f / dist) : v3(0.0f, 1.0f, 0.0f);
        out.push_back({A.pos + n * A.half.x, n, reach - dist});
    }

    inline void collide(const view_t& A, const view_t& B, std::vector<hit_t>& out)
    {
        if (vec3::len(A.pos - B.pos) > bound_radius(A) + bound_radius(B)) return;
        const bool sa = A.shape == shape_t::sphere, sb = B.shape == shape_t::sphere;
        if (!sa && !sb)     box_box(A, B, out);
        else if (sa && !sb) sphere_box(A, B, true, out);
        else if (!sa && sb) sphere_box(B, A, false, out);
        else                sphere_sphere(A, B, out);
    }

    // one side of a contact: what the solver may change and how easily
    struct side_t {
        v3 *vel = nullptr, *ang = nullptr; // null = does not move
        v3  pos;
        f32 inv_mass = 0.0f;
        m3_t inv_i;
    };

    struct contact_t {
        side_t a, b;
        int ia = -1, ib = -1;  // rigid index, `PLAYER` or `STATIC`
        v3  ra, rb, normal, t1, t2;
        f32 depth = 0.0f, mu = 0.0f, bias = 0.0f;
        f32 mass_n = 0.0f, mass_t1 = 0.0f, mass_t2 = 0.0f;
        f32 acc_n = 0.0f, acc_t1 = 0.0f, acc_t2 = 0.0f;
    };

    constexpr int STATIC = -1;
    constexpr int PLAYER = -2;

    inline v3 velocity_at(const side_t& s, const v3& r)
    {
        v3 v = s.vel ? *s.vel : v3();
        if (s.ang) v += vec3::cross(*s.ang, r);
        return v;
    }

    inline f32 effective_mass(const contact_t& c, const v3& dir)
    {
        const v3 ra = vec3::cross(c.ra, dir), rb = vec3::cross(c.rb, dir);
        const f32 k = c.a.inv_mass + c.b.inv_mass
                    + vec3::dot(vec3::cross(mul(c.a.inv_i, ra), c.ra), dir)
                    + vec3::dot(vec3::cross(mul(c.b.inv_i, rb), c.rb), dir);
        return k > 0.0f ? 1.0f / k : 0.0f;
    }

    inline void apply(contact_t& c, const v3& P)
    {
        if (c.a.vel) *c.a.vel -= P * c.a.inv_mass;
        if (c.a.ang) *c.a.ang -= mul(c.a.inv_i, vec3::cross(c.ra, P));
        if (c.b.vel) *c.b.vel += P * c.b.inv_mass;
        if (c.b.ang) *c.b.ang += mul(c.b.inv_i, vec3::cross(c.rb, P));
    }

    inline v3 inertia_local(const rigid_t& r)
    {
        const f32 m = r.mass;
        if (r.shape == shape_t::sphere) {
            const f32 i = 0.4f * m * r.half.x * r.half.x;
            return v3(i, i, i);
        }
        return v3(m / 3.0f * (r.half.y * r.half.y + r.half.z * r.half.z),
                  m / 3.0f * (r.half.x * r.half.x + r.half.z * r.half.z),
                  m / 3.0f * (r.half.x * r.half.x + r.half.y * r.half.y));
    }

    // R * diag(1 / I) * R^T
    inline m3_t inverse_inertia_world(const rigid_t& r)
    {
        const v3 I = inertia_local(r);
        const f32 inv[3] = {1.0f / I.x, 1.0f / I.y, 1.0f / I.z};
        m3_t out;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                f32 s = 0.0f;
                for (int k = 0; k < 3; ++k) s += r.orient.at(i, k) * inv[k] * r.orient.at(j, k);
                out.m[i][j] = s;
            }
        }
        return out;
    }

    inline view_t view_of(const rigid_t& r)
    {
        return view_t{r.shape, r.pos, r.half, &r.orient};
    }
    inline view_t view_of(const body_t& b)
    {
        return view_t{shape_t::box, b.pos, b.half, &identity()};
    }

    inline side_t side_of(rigid_t& r)
    {
        side_t s;
        s.vel = &r.vel;
        s.ang = &r.angvel;
        s.pos = r.pos;
        s.inv_mass = 1.0f / r.mass;
        s.inv_i = inverse_inertia_world(r);
        return s;
    }
    inline side_t side_of(body_t& b) // the player: pushed around, never rotated
    {
        side_t s;
        s.vel = &b.vel;
        s.pos = b.pos;
        s.inv_mass = 1.0f / b.mass;
        return s;
    }
}

inline size_t phys::rigid_world_t::add(rigid_t body)
{
    bodies.push_back(body);
    return bodies.size() - 1;
}

inline mat4_t phys::axis_angle(vec3_t<gl::f32> axis, gl::f32 angle)
{
    const gl::f32 c = std::cos(angle), s = std::sin(angle), t = 1.0f - c;
    const gl::f32 x = axis.x, y = axis.y, z = axis.z;
    mat4_t r = mat4_t::identity();
    r.at(0,0) = t*x*x + c;   r.at(0,1) = t*x*y - s*z; r.at(0,2) = t*x*z + s*y;
    r.at(1,0) = t*x*y + s*z; r.at(1,1) = t*y*y + c;   r.at(1,2) = t*y*z - s*x;
    r.at(2,0) = t*x*z - s*y; r.at(2,1) = t*y*z + s*x; r.at(2,2) = t*z*z + c;
    return r;
}

inline void phys::orthonormalize(mat4_t& orient)
{
    using namespace detail;
    const v3 x = vec3::norm(col(orient, 0));
    const v3 y = vec3::norm(col(orient, 1) - x * vec3::dot(col(orient, 1), x));
    const v3 z = vec3::cross(x, y);
    for (int r = 0; r < 3; ++r) {
        orient.at(r, 0) = comp(x, r);
        orient.at(r, 1) = comp(y, r);
        orient.at(r, 2) = comp(z, r);
    }
}

// sequential impulses: detect contacts, then push velocities until none of them is closing
inline void phys::rigid_world_t::step(gl::f32 dt, world_t& statics, int player)
{
    using namespace detail;

    // what the player is doing on its own, before anything pushes it
    const v3 player_vel = player >= 0 ? statics.bodies[player].vel : v3();

    for (rigid_t& r : bodies) {
        r.touching = false;
        if (!r.enabled) continue;
        r.vel += gravity * dt;
        if (r.held) r.angvel = r.angvel * std::exp(-10.0f * dt);
    }

    std::vector<contact_t> contacts;
    std::vector<hit_t> hits;

    auto add_contacts = [&](int ia, int ib, const view_t& A, const view_t& B, const side_t& sa, const side_t& sb, f32 mu, f32 e) {
        hits.clear();
        collide(A, B, hits);
        for (const hit_t& h : hits) {
            contact_t c;
            c.a = sa; c.b = sb; c.ia = ia; c.ib = ib;
            c.normal = h.normal;
            c.depth  = h.depth;
            c.mu     = mu;
            c.ra = h.point - sa.pos;
            c.rb = h.point - sb.pos;
            c.t1 = vec3::norm(vec3::cross(c.normal, std::fabs(c.normal.x) > 0.9f ? v3(0, 1, 0) : v3(1, 0, 0)));
            c.t2 = vec3::cross(c.normal, c.t1);
            c.mass_n  = effective_mass(c, c.normal);
            c.mass_t1 = effective_mass(c, c.t1);
            c.mass_t2 = effective_mass(c, c.t2);

            const f32 vn = vec3::dot(velocity_at(c.b, c.rb) - velocity_at(c.a, c.ra), c.normal);
            f32 bias = std::min(BETA / dt * std::max(h.depth - SLOP, 0.0f), MAX_PUSH);
            if (vn < -REST_THRESHOLD) bias = std::max(bias, -e * vn);
            c.bias = bias;
            contacts.push_back(c);
        }
    };

    for (size_t i = 0; i < bodies.size(); ++i) {
        rigid_t& r = bodies[i];
        if (!r.enabled) continue;
        const view_t vr = view_of(r);
        const side_t sr = side_of(r);

        // static boxes
        for (const body_t& s : statics.bodies) {
            if (!s.is_static || !s.enabled) continue;
            add_contacts((int)i, STATIC, vr, view_of(s), sr, side_t{}, r.friction, r.restitution);
        }
        // the player
        if (player >= 0 && statics.bodies[player].enabled) {
            body_t& p = statics.bodies[player];
            add_contacts((int)i, PLAYER, vr, view_of(p), sr, side_of(p), r.friction, r.restitution);
        }
        // other rigid bodies
        for (size_t j = i + 1; j < bodies.size(); ++j) {
            rigid_t& o = bodies[j];
            if (!o.enabled) continue;
            add_contacts((int)i, (int)j, vr, view_of(o), sr, side_of(o),
                         std::sqrt(r.friction * o.friction), std::max(r.restitution, o.restitution));
        }
    }

    for (int it = 0; it < iterations; ++it) {
        for (contact_t& c : contacts) {
            // normal
            f32 vn = vec3::dot(velocity_at(c.b, c.rb) - velocity_at(c.a, c.ra), c.normal);
            f32 lambda = c.mass_n * (-vn + c.bias);
            f32 acc = std::max(c.acc_n + lambda, 0.0f);
            lambda  = acc - c.acc_n;
            c.acc_n = acc;
            apply(c, c.normal * lambda);

            // friction, can't be stronger than mu times what presses the bodies together
            const f32 limit = c.mu * c.acc_n;
            const v3  dirs[2]  = {c.t1, c.t2};
            f32 *accs[2]       = {&c.acc_t1, &c.acc_t2};
            const f32 masses[2] = {c.mass_t1, c.mass_t2};
            for (int k = 0; k < 2; ++k) {
                const f32 vt = vec3::dot(velocity_at(c.b, c.rb) - velocity_at(c.a, c.ra), dirs[k]);
                f32 l   = -vt * masses[k];
                f32 new_acc = std::clamp(*accs[k] + l, -limit, limit);
                l = new_acc - *accs[k];
                *accs[k] = new_acc;
                apply(c, dirs[k] * l);
            }
        }
    }

    if (player >= 0) {
        body_t& pl = statics.bodies[player];
        const v3 kick = pl.vel - player_vel;
        const f32 k = vec3::len(kick);
        if (k > MAX_PLAYER_KICK) pl.vel = player_vel + kick * (MAX_PLAYER_KICK / k);
    }

    // The player is blocked by what it pushes: its velocity is overwritten by input every frame, so impulses
    // alone would let it walk through. Only the player's own motion into the body is undone (an object that
    // falls or is swung onto the player never shoves it around through this), never down (something resting
    // on the player is carried, the floor takes the weight) and never for something the player holds. After
    // that the player is moved back out of walls, so nothing can squeeze it through them.
    if (player >= 0) {
        body_t& pl = statics.bodies[player];
        std::vector<const contact_t *> deepest(bodies.size(), nullptr);
        for (const contact_t& c : contacts) {
            if (c.ib != PLAYER || bodies[c.ia].held || c.normal.y < -0.5f) continue;
            if (deepest[c.ia] == nullptr || c.depth > deepest[c.ia]->depth) deepest[c.ia] = &c;
        }
        bool moved = false;
        for (const contact_t *c : deepest) {
            if (c == nullptr || c->depth <= SLOP) continue;
            const f32 closing = std::max(0.0f, -vec3::dot(player_vel, c->normal)); // against the normal = into the body
            const f32 push = std::min(c->depth - SLOP, closing * dt + 0.002f);
            pl.pos += c->normal * push;
            moved = true;
        }
        if (moved) {
            for (const body_t& s : statics.bodies) {
                if (!s.is_static || !s.enabled) continue;
                const f32 pen[3] = {pl.half.x + s.half.x - std::fabs(pl.pos.x - s.pos.x),
                                    pl.half.y + s.half.y - std::fabs(pl.pos.y - s.pos.y),
                                    pl.half.z + s.half.z - std::fabs(pl.pos.z - s.pos.z)};
                if (pen[0] <= 0.0f || pen[1] <= 0.0f || pen[2] <= 0.0f) continue;
                const int axis = pen[0] < pen[1] ? (pen[0] < pen[2] ? 0 : 2) : (pen[1] < pen[2] ? 1 : 2);
                const f32 sign = comp(pl.pos, axis) >= comp(s.pos, axis) ? 1.0f : -1.0f;
                comp(pl.pos, axis) += sign * pen[axis];
                if (axis == 1 && sign > 0.0f) pl.on_ground = true;
            }
        }
    }

    for (const contact_t& c : contacts) {
        if (c.acc_n <= 0.0f) continue;
        if (c.ia >= 0) bodies[c.ia].touching = true;
        if (c.ib >= 0) bodies[c.ib].touching = true;
        // the player stands on what pushes it up
        if (c.ib == PLAYER && c.normal.y > 0.6f) statics.bodies[player].on_ground = true;
    }

    for (rigid_t& r : bodies) {
        if (!r.enabled) continue;

        const f32 speed = vec3::len(r.vel);
        if (speed > MAX_SPEED) r.vel = r.vel * (MAX_SPEED / speed);
        const f32 spin = vec3::len(r.angvel);
        if (spin > MAX_SPIN) r.angvel = r.angvel * (MAX_SPIN / spin);

        // things that roll lose it to the surface they roll on
        r.angvel = r.angvel * std::exp(-(r.touching ? ROLL_DAMPING : AIR_DAMPING) * dt);

        r.pos += r.vel * dt;
        const f32 w = vec3::len(r.angvel);
        if (w > 1e-6f) {
            r.orient = axis_angle(r.angvel * (1.0f / w), w * dt) * r.orient;
            orthonormalize(r.orient);
        }

        // settled bodies are held still, so they don't jitter on their contacts
        if (r.touching && !r.held && vec3::len(r.vel) < SLEEP_SPEED && vec3::len(r.angvel) < SLEEP_SPIN) r.rest_time += dt;
        else r.rest_time = 0.0f;
        if (r.rest_time > SLEEP_TIME) {
            r.vel    = v3();
            r.angvel = v3();
        }
    }
}
