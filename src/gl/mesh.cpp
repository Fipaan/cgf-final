#include "mesh.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sstream>

#include <basic.hpp>

bool mesh_t::load_smf(const char *path)
{
    std::ifstream in(path);
    if (!in) {
        errorfn("Failed to open file: {}", path);
        return false;
    }

    vertices.clear();
    indices.clear();

    std::string line;
    while (std::getline(in, line)) {
        std::istringstream ss(line);
        std::string tag;
        ss >> tag;

        if (tag == "v") {
            GLfloat x, y, z;
            if (ss >> x >> y >> z) vertices.emplace_back(x, y, z);
        } else if (tag == "f") {
            std::vector<uint32_t> face;
            std::string tok;
            while (ss >> tok) {
                char *end = nullptr;
                long i = std::strtol(tok.c_str(), &end, 10);
                if (end == tok.c_str()) continue;
                long idx = i > 0 ? i - 1 : (long)vertices.size() + i;
                if (idx < 0 || idx >= (long)vertices.size()) {
                    errorfn("{}: face index {} out of range", path, i);
                    return false;
                }
                face.push_back((uint32_t)idx);
            }
            // fan triangulation
            for (size_t k = 2; k < face.size(); ++k) {
                indices.push_back(face[0]);
                indices.push_back(face[k - 1]);
                indices.push_back(face[k]);
            }
        }
    }

    if (indices.empty()) {
        errorfn("{}: no triangles found", path);
        return false;
    }

    infofn("Loaded {}: {} vertices, {} triangles", path, vertices.size(), tri_count());
    return true;
}

void mesh_t::normalize()
{
    vec3_t<GLfloat> lo = vertices[0], hi = vertices[0];
    for (const auto& v : vertices) {
        lo = vec3_t<GLfloat>(std::min(lo.x, v.x), std::min(lo.y, v.y), std::min(lo.z, v.z));
        hi = vec3_t<GLfloat>(std::max(hi.x, v.x), std::max(hi.y, v.y), std::max(hi.z, v.z));
    }
    vec3_t<GLfloat> c = (lo + hi) / 2.0f;

    GLfloat r = 0;
    for (const auto& v : vertices) r = std::max(r, vec3::len(v - c));
    if (r < 1e-12f) r = 1.0f;

    for (auto& v : vertices) v = (v - c) / r;
}

void mesh_t::compute_normals()
{
    face_normals.assign(tri_count(), vec3_t<GLfloat>());
    vertex_normals.assign(vertices.size(), vec3_t<GLfloat>());

    for (size_t t = 0; t < tri_count(); ++t) {
        uint32_t a = indices[3*t + 0];
        uint32_t b = indices[3*t + 1];
        uint32_t c = indices[3*t + 2];
        vec3_t<GLfloat> n = vec3::normalize(vec3::cross(vertices[b] - vertices[a], vertices[c] - vertices[a]));
        face_normals[t] = n;
        // plain average of unit face normals (not area weighted)
        vertex_normals[a] += n;
        vertex_normals[b] += n;
        vertex_normals[c] += n;
    }
    for (auto& n : vertex_normals) n = vec3::normalize(n);
}

size_t mesh_t::tri_count() const
{
    return indices.size() / 3;
}

mesh_t mesh_t::cube()
{
    struct face_t { vec3_t<GLfloat> n, u, v; }; // u x v = n
    const face_t faces[6] = {
        {{ 1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
        {{ 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.0f,-1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{ 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{ 0.0f, 0.0f,-1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
    };

    mesh_t m;
    // 4 vertices per face, so normals stay flat
    for (const auto& f : faces) {
        uint32_t b = (uint32_t)m.vertices.size();
        m.vertices.push_back(f.n - f.u - f.v);
        m.vertices.push_back(f.n + f.u - f.v);
        m.vertices.push_back(f.n + f.u + f.v);
        m.vertices.push_back(f.n - f.u + f.v);
        m.indices.insert(m.indices.end(), {b, b + 1, b + 2, b, b + 2, b + 3});
    }
    m.compute_normals();
    return m;
}

mesh_t mesh_t::sphere(int stacks, int slices)
{
    mesh_t m;
    m.vertices.emplace_back(0.0f, 1.0f, 0.0f); // north pole, index 0
    for (int i = 1; i < stacks; ++i) {
        GLfloat phi = gl::PI * (GLfloat)i / (GLfloat)stacks;
        for (int j = 0; j < slices; ++j) {
            GLfloat th = 2.0f * gl::PI * (GLfloat)j / (GLfloat)slices;
            m.vertices.emplace_back(std::sin(phi) * std::cos(th), std::cos(phi), std::sin(phi) * std::sin(th));
        }
    }
    m.vertices.emplace_back(0.0f, -1.0f, 0.0f); // south pole
    const uint32_t south = (uint32_t)m.vertices.size() - 1;

    auto ring = [&](int i, int j) -> uint32_t { return (uint32_t)(1 + (i - 1) * slices + (j % slices)); };

    for (int j = 0; j < slices; ++j) {
        m.indices.insert(m.indices.end(), {0u, ring(1, j + 1), ring(1, j)});
    }
    for (int i = 1; i < stacks - 1; ++i) {
        for (int j = 0; j < slices; ++j) {
            uint32_t a = ring(i, j),     b = ring(i, j + 1);
            uint32_t d = ring(i + 1, j), c = ring(i + 1, j + 1);
            m.indices.insert(m.indices.end(), {a, b, d, b, c, d});
        }
    }
    for (int j = 0; j < slices; ++j) {
        m.indices.insert(m.indices.end(), {south, ring(stacks - 1, j), ring(stacks - 1, j + 1)});
    }

    m.compute_normals();
    // exact normals on a unit sphere
    for (size_t i = 0; i < m.vertices.size(); ++i) m.vertex_normals[i] = vec3::normalize(m.vertices[i]);
    return m;
}

mesh_t mesh_t::plane()
{
    mesh_t m;
    m.vertices = {
        {-1.0f, 0.0f, -1.0f},
        {-1.0f, 0.0f,  1.0f},
        { 1.0f, 0.0f,  1.0f},
        { 1.0f, 0.0f, -1.0f},
    };
    m.indices = {0, 1, 2, 0, 2, 3};
    m.compute_normals();
    return m;
}

void mesh_t::build_buffer(std::vector<GLfloat>& out) const
{
    out.clear();
    out.reserve(indices.size() * 9);

    for (size_t t = 0; t < tri_count(); ++t) {
        vec3_t<GLfloat> col = vec3::abs(face_normals[t]);
        for (size_t k = 0; k < 3; ++k) {
            uint32_t i = indices[3*t + k];
            const vec3_t<GLfloat>& p = vertices[i];
            const vec3_t<GLfloat>& n = vertex_normals[i];
            out.insert(out.end(), {p.x, p.y, p.z, n.x, n.y, n.z, col.x, col.y, col.z});
        }
    }
}

bool mesh_gpu_t::upload(const mesh_t& mesh)
{
    std::vector<GLfloat> buf;
    mesh.build_buffer(buf);

    vao.gen_array();
    if (!vao.bind()) return false;

    vbo.gen_buffer();
    if (!vbo.add_vad(gl::vad_t(0, gl::vad_cpv_t::comp_3, 9, 0))) return false; // pos
    if (!vbo.add_vad(gl::vad_t(1, gl::vad_cpv_t::comp_3, 9, 3))) return false; // smooth normal
    if (!vbo.add_vad(gl::vad_t(2, gl::vad_cpv_t::comp_3, 9, 6))) return false; // flat color
    if (!vbo.init_buffer(std::span<GLfloat>(buf.data(), buf.size()), gl::dsu_t::static_draw)) return false;

    vertex_count = (GLsizei)(buf.size() / 9);
    return true;
}

bool mesh_gpu_t::draw()
{
    if (!vao.bind()) return false;
    glDrawArrays(GL_TRIANGLES, 0, vertex_count);
    return true;
}
