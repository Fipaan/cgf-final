#include "particles.hpp"

#include <algorithm>
#include <cmath>

using namespace math;
using V = vec3_t<gl::f32>;

constexpr size_t FLOATS_PER_PARTICLE = 8; // pos, colour + alpha, size

particles_t::~particles_t()
{
    if (vbo != 0) glDeleteBuffers(1, &vbo);
    if (vao != 0) glDeleteVertexArrays(1, &vao);
}

gl::f32 particles_t::rand(gl::f32 lo, gl::f32 hi)
{
    return std::uniform_real_distribution<gl::f32>(lo, hi)(rng);
}

bool particles_t::init(const char *vertex_path, const char *fragment_path)
{
    if (!shader.from_files(vertex_path, fragment_path)) return false;

    const GLsizei stride = (GLsizei)(FLOATS_PER_PARTICLE * sizeof(gl::f32));
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(MAX_PARTICLES * stride), nullptr, GL_STREAM_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (const void *)0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (const void *)(3 * sizeof(gl::f32)));
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride, (const void *)(7 * sizeof(gl::f32)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    list.reserve(MAX_PARTICLES);
    initialized = true;
    return true;
}

void particles_t::emit(const particle_t& particle)
{
    if (!initialized || list.size() >= MAX_PARTICLES) return;
    list.push_back(particle);
}

void particles_t::update(gl::f32 dt)
{
    dt = std::min(dt, 0.1f);
    for (size_t i = 0; i < list.size();) {
        particle_t& p = list[i];
        p.age += dt;
        if (p.age >= p.life || p.pos.y < 0.0f) {
            p = list.back();
            list.pop_back();
            continue;
        }
        p.vel.y += p.gravity * dt;
        p.vel = p.vel * std::exp(-p.drag * dt);
        p.pos += p.vel * dt;
        ++i;
    }
}

bool particles_t::draw(const mat4_t& view, const mat4_t& proj, gl::f32 fovy)
{
    if (!initialized || list.empty()) return true;

    _buffer.clear();
    for (const particle_t& p : list) {
        const gl::f32 t = p.age / p.life;
        // quick fade in (so they don't pop), slow fade out
        const gl::f32 a = p.alpha * std::min(p.age * 5.0f, 1.0f) * (1.0f - t);
        const gl::f32 size = p.size0 + (p.size1 - p.size0) * t;
        const gl::f32 v[FLOATS_PER_PARTICLE] = {p.pos.x, p.pos.y, p.pos.z, p.color.x, p.color.y, p.color.z, a, size};
        _buffer.insert(_buffer.end(), v, v + FLOATS_PER_PARTICLE);
    }

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    const gl::f32 px_scale = (gl::f32)viewport[3] * 0.5f / std::tan(fovy * 0.5f);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)(_buffer.size() * sizeof(gl::f32)), _buffer.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    shader.use();
    shader.uniform(gl::m_type_t::m4x4, "view", 1, gl::gl_bool::FALSE, view.m);
    shader.uniform(gl::m_type_t::m4x4, "proj", 1, gl::gl_bool::FALSE, proj.m);
    shader.uniform("px_scale", px_scale);

    const bool was_blend = glIsEnabled(GL_BLEND);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE); // objects hide them, they don't hide each other
    glEnable(GL_PROGRAM_POINT_SIZE);

    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, (GLsizei)list.size());
    glBindVertexArray(0);

    glDisable(GL_PROGRAM_POINT_SIZE);
    glDepthMask(GL_TRUE);
    if (!was_blend) glDisable(GL_BLEND);
    return true;
}

void particles_t::sparks(V pos, int count, gl::f32 speed, V color)
{
    for (int i = 0; i < count; ++i) {
        V dir(rand(-1.0f, 1.0f), rand(-0.3f, 1.0f), rand(-1.0f, 1.0f));
        if (vec3::len(dir) < 1e-3f) dir = V(0.0f, 1.0f, 0.0f);
        dir = vec3::norm(dir);
        particle_t p;
        p.pos = pos; p.vel = dir * rand(0.3f * speed, speed);
        p.color = color * rand(0.8f, 1.0f);
        p.life = rand(0.6f, 1.4f);
        p.size0 = 0.035f; p.size1 = 0.01f;
        p.alpha = 0.95f; p.gravity = -6.0f; p.drag = 0.8f;
        emit(p);
    }
}

void particles_t::smoke(V pos, int count, V color)
{
    for (int i = 0; i < count; ++i) {
        particle_t p;
        p.pos = pos + V(rand(-0.2f, 0.2f), rand(-0.1f, 0.2f), rand(-0.2f, 0.2f));
        p.vel = V(rand(-0.4f, 0.4f), rand(0.3f, 1.0f), rand(-0.4f, 0.4f));
        p.color = color;
        p.life = rand(1.5f, 2.6f);
        p.size0 = 0.2f; p.size1 = rand(0.8f, 1.3f);
        p.alpha = 0.16f; p.drag = 1.2f;
        emit(p);
    }
}

void particles_t::fountain(V pos, int count, V color)
{
    for (int i = 0; i < count; ++i) {
        particle_t p;
        p.pos = pos + V(rand(-0.15f, 0.15f), 0.0f, rand(-0.15f, 0.15f));
        p.vel = V(rand(-1.0f, 1.0f), rand(1.5f, 4.0f), rand(-1.0f, 1.0f));
        p.color = color * rand(0.7f, 1.0f);
        p.life = rand(1.0f, 1.8f);
        p.size0 = 0.06f; p.size1 = 0.02f;
        p.alpha = 0.9f; p.gravity = -3.0f; p.drag = 0.5f;
        emit(p);
    }
}
