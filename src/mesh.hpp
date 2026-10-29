#pragma once

#include <numbers>
#include <optional>
#include <span>
#include <string>
#include <vector>
#include <cstdint>

#include "gl_first.hpp"
#include "gl_const.hpp"
#include "gl_shader.hpp"
#include "math3d.hpp"

struct mesh_t {
    std::vector<vec3_t<GLfloat>> vertices;
    std::vector<uint32_t>        indices;        // 3 per triangle
    std::vector<vec3_t<GLfloat>> face_normals;   // unit, per triangle
    std::vector<vec3_t<GLfloat>> vertex_normals; // unit, averaged from adjacent faces

    bool load_smf(const char *path);
    void normalize(); // centre bbox at origin, max radius = 1
    void compute_normals();
    size_t tri_count() const;

    // 3 vertices per triangle, 9 floats each: pos, smooth normal, |face normal|
    void build_buffer(std::vector<GLfloat>& out) const;
};

struct mesh_gpu_t {
    gl::vao_t vao;
    gl::vbo_t vbo;
    GLsizei   vertex_count = 0;

    bool upload(const mesh_t& mesh);
    bool draw();
};
