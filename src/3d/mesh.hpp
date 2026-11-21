/*
 * 3d/mesh.hpp -
 *     Mesh storing and manipulating.
 */
#pragma once

#include <vector>
#include <cstdint>

#include <core/gl/first.hpp>
#include <core/gl/shader.hpp>
#include <3d/vec.hpp>
#include <2d/vec.hpp>
#include <gl/flat.hpp>

namespace math {

struct mesh_t {
    std::vector<vec3_t<gl::f32>> vertices;
    std::vector<uint32_t>        indices;        // 3 per triangle
    std::vector<vec3_t<gl::f32>> face_normals;   // unit, per triangle
    std::vector<vec3_t<gl::f32>> vertex_normals; // unit, averaged from adjacent faces
    std::vector<vec2_t<gl::f32>> uvs;            // per vertex, (0, 0) is the bottom left of the image; empty = none

    bool load_smf(const char *path);
    void norm(); // centre bbox at origin, max radius = 1
    void compute_normals();
    void compute_face_normals(); // only `face_normals`, `vertex_normals` are left as they are
    size_t tri_count() const;

    // primitives, normals already computed
    static mesh_t cube();                                 // [-1, 1]^3, flat normals
    static mesh_t sphere(int stacks = 16, int slices = 32); // radius 1, smooth normals
    static mesh_t plane();                                // [-1, 1] on XZ, normal +Y

    // 3 vertices per triangle, 11 floats each: pos, smooth normal, |face normal|, uv
    void spread(gl::flat_t<gl::f32>& container) const;
};

} // namespace math

namespace gl {

struct mesh_gpu_t {
    gl::vao_t vao;
    gl::vbo_t vbo;
    GLsizei   vertex_count = 0;

    bool upload(const math::mesh_t& mesh);
    bool draw();
};

} // namespace gl