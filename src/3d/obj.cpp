#include "obj.hpp"

#include <algorithm>
#include <cstdint>
#include <unordered_map>

#include <core/log.hpp>
#include <core/fast_obj/obj.hpp>

using namespace math;

namespace {

// the same position with another normal or another uv is another vertex
struct corner_key_t {
    uint32_t p, n, t;
    bool operator==(const corner_key_t&) const = default;
};

struct corner_hash_t {
    size_t operator()(const corner_key_t& k) const
    {
        uint64_t h = ((uint64_t)k.p << 32) | k.n;
        h ^= (uint64_t)k.t * 0x9E3779B97F4A7C15ull + (h << 6) + (h >> 2);
        return (size_t)h;
    }
};

} // namespace

bool obj_model_t::load(const char *path, bool z_up)
{
    fastObjMesh *obj = fast_obj_read(path);
    if (obj == nullptr) {
        errorfn("Failed to read {}", path);
        return false;
    }

    parts.clear();
    std::vector<int> part_of(obj->material_count, -1);
    // (position, normal, uv) -> vertex of the part
    std::vector<std::unordered_map<corner_key_t, uint32_t, corner_hash_t>> vertex_of;
    std::vector<bool> missing_normals;

    size_t corner = 0;
    for (unsigned int f = 0; f < obj->face_count; ++f) {
        const unsigned int count = obj->face_vertices[f];
        const unsigned int first = (unsigned int)corner;
        corner += count;
        if (count < 3) continue; // lines and points

        const unsigned int material = obj->face_materials[f];
        if (part_of[material] < 0) {
            const fastObjMaterial& m = obj->materials[material];
            obj_part_t part;
            part.material.name      = m.name != nullptr ? m.name : "";
            part.material.ambient   = color_t(m.Ka[0], m.Ka[1], m.Ka[2], 1.0f);
            part.material.diffuse   = color_t(m.Kd[0], m.Kd[1], m.Kd[2], m.d);
            part.material.specular  = color_t(m.Ks[0], m.Ks[1], m.Ks[2], 1.0f);
            part.material.emission  = color_t(m.Ke[0], m.Ke[1], m.Ke[2], 1.0f);
            part.material.shininess = m.Ns;
            if (m.map_Kd != 0 && m.map_Kd < obj->texture_count) {
                part.material.diffuse_map = obj->textures[m.map_Kd].path;
            }
            part_of[material] = (int)parts.size();
            parts.push_back(std::move(part));
            vertex_of.emplace_back();
            missing_normals.push_back(false);
        }
        const int pi = part_of[material];
        mesh_t& mesh = parts[pi].mesh;

        std::vector<uint32_t> face(count);
        for (unsigned int k = 0; k < count; ++k) {
            const fastObjIndex& index = obj->indices[first + k];
            const corner_key_t key{index.p, index.n, index.t};

            auto it = vertex_of[pi].find(key);
            if (it == vertex_of[pi].end()) {
                const float *p = &obj->positions[3 * index.p];
                vec3_t<gl::f32> position(p[0], p[1], p[2]);
                vec3_t<gl::f32> normal;
                if (index.n != 0) {
                    const float *n = &obj->normals[3 * index.n];
                    normal = vec3_t<gl::f32>(n[0], n[1], n[2]);
                } else {
                    missing_normals[pi] = true;
                }
                vec2_t<gl::f32> uv(0.0f, 0.0f);
                if (index.t != 0) {
                    const float *t = &obj->texcoords[2 * index.t];
                    uv = vec2_t<gl::f32>(t[0], t[1]);
                }
                if (z_up) {
                    position = vec3_t<gl::f32>(position.x, position.z, -position.y);
                    normal   = vec3_t<gl::f32>(normal.x, normal.z, -normal.y);
                }
                it = vertex_of[pi].emplace(key, (uint32_t)mesh.vertices.size()).first;
                mesh.vertices.push_back(position);
                mesh.vertex_normals.push_back(vec3::norm(normal));
                mesh.uvs.push_back(uv);
            }
            face[k] = it->second;
        }
        // fan triangulation
        for (unsigned int k = 2; k < count; ++k) {
            mesh.indices.insert(mesh.indices.end(), {face[0], face[k - 1], face[k]});
        }
    }
    fast_obj_destroy(obj);

    if (parts.empty()) {
        errorfn("{}: no faces found", path);
        return false;
    }

    size_t vertex_total = 0, triangle_total = 0;
    bool first_vertex = true;
    for (size_t i = 0; i < parts.size(); ++i) {
        mesh_t& mesh = parts[i].mesh;
        if (missing_normals[i]) mesh.compute_normals(); // averaged, the file didn't have them
        else mesh.compute_face_normals();

        for (const vec3_t<gl::f32>& v : mesh.vertices) {
            if (first_vertex) { bounds_min = bounds_max = v; first_vertex = false; continue; }
            bounds_min = vec3_t<gl::f32>(std::min(bounds_min.x, v.x), std::min(bounds_min.y, v.y), std::min(bounds_min.z, v.z));
            bounds_max = vec3_t<gl::f32>(std::max(bounds_max.x, v.x), std::max(bounds_max.y, v.y), std::max(bounds_max.z, v.z));
        }
        vertex_total   += mesh.vertices.size();
        triangle_total += mesh.tri_count();
    }

    infofn("Loaded {}: {} materials, {} vertices, {} triangles", path, parts.size(), vertex_total, triangle_total);
    return true;
}
