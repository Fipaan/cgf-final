/*
 * scene/particles.hpp -
 *     Billboard particles (bubbles, steam, sparks, smoke, dust): simulated on the CPU, drawn as one batch of
 *     soft round point sprites, blended additively.
 */
#pragma once

#include <random>
#include <vector>

#include <core/gl/first.hpp>
#include <core/gl/shader.hpp>
#include <3d/math.hpp>

/* Usage:
```cpp
particles_t particles;
if (!particles.init("shaders/particle.vert", "shaders/particle.frag")) return 1;
particles.sparks({0, 1, 0}, 40, 5.0f, {1.0f, 0.6f, 0.2f});

// every frame
particles.update(dt);
particles.draw(view, proj, fovy); // after the opaque objects
```
*/
struct particle_t {
    math::vec3_t<gl::f32> pos, vel;
    math::vec3_t<gl::f32> color = math::vec3_t<gl::f32>(1.0f);
    gl::f32 life  = 1.0f; // seconds
    gl::f32 age   = 0.0f;
    gl::f32 size0 = 0.1f, size1 = 0.1f; // world units at birth and at death
    gl::f32 alpha = 1.0f;
    gl::f32 gravity = 0.0f; // acceleration along Y, negative falls
    gl::f32 drag    = 0.0f; // 1/s
};

constexpr size_t MAX_PARTICLES = 4000;

struct particles_t {
    bool initialized = false;
    gl::shader_t shader;
    GLuint vao = 0, vbo = 0;
    std::vector<particle_t> list;
    std::vector<gl::f32>    _buffer;
    std::mt19937 rng{4321};

    particles_t() = default;
    ~particles_t();
    particles_t(const particles_t&) = delete;
    particles_t& operator=(const particles_t&) = delete;

    bool init(const char *vertex_path, const char *fragment_path);
    void emit(const particle_t& particle); // dropped when the limit is reached
    void update(gl::f32 dt);
    bool draw(const math::mat4_t& view, const math::mat4_t& proj, gl::f32 fovy);

    // ready-made effects
    void sparks(math::vec3_t<gl::f32> pos, int count, gl::f32 speed, math::vec3_t<gl::f32> color);   // fly out, fall
    void smoke(math::vec3_t<gl::f32> pos, int count, math::vec3_t<gl::f32> color);                   // puffs that grow and rise
    void fountain(math::vec3_t<gl::f32> pos, int count, math::vec3_t<gl::f32> color);                // goes up and sprays

    gl::f32 rand(gl::f32 lo, gl::f32 hi);
};
