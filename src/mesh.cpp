#include "mesh.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sstream>

#include "basic.hpp"

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
