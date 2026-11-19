#include "obj.hpp"

#include <algorithm>
#include <cstdint>
#include <unordered_map>

#include <core/log.hpp>
#include <core/fast_obj/obj.hpp>
#include <core/stb/image.hpp>

using namespace math;

// NOTE: textures are not drawn yet, so a material that takes its colour from one gets the average colour
//       of the image, otherwise wood would come out white
static void tint_with_texture(obj_material_t& material)
{
    int w = 0, h = 0, channels = 0;
    unsigned char *pixels = stbi_load(material.diffuse_map.c_str(), &w, &h, &channels, 3);
    if (pixels == nullptr) {
        errorfn("{}: {}", material.diffuse_map, stbi_failure_reason());
        return;
    }
    double sum[3] = {0.0, 0.0, 0.0};
    const size_t count = (size_t)w * h;
    for (size_t i = 0; i < count; ++i) {
        for (int k = 0; k < 3; ++k) sum[k] += pixels[3 * i + k];
    }
    stbi_image_free(pixels);

    material.diffuse = color_t(material.diffuse.r * (gl::f32)(sum[0] / count / 255.0),
                               material.diffuse.g * (gl::f32)(sum[1] / count / 255.0),
                               material.diffuse.b * (gl::f32)(sum[2] / count / 255.0),
                               material.diffuse.a);
}

bool obj_model_t::load(const char *path, bool z_up)
{
    fastObjMesh *obj = fast_obj_read(path);
    if (obj == nullptr) {
        errorfn("Failed to read {}", path);
        return false;
    }

    parts.clear();
    std::vector<int> part_of(obj->material_count, -1);
    // (position, normal) -> vertex of the part, the same position with another normal is another vertex
    std::vector<std::unordered_map<uint64_t, uint32_t>> vertex_of;
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
                tint_with_texture(part.material);
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
            const uint64_t key = ((uint64_t)index.p << 32) | index.n;

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
                if (z_up) {
                    position = vec3_t<gl::f32>(position.x, position.z, -position.y);
                    normal   = vec3_t<gl::f32>(normal.x, normal.z, -normal.y);
                }
                it = vertex_of[pi].emplace(key, (uint32_t)mesh.vertices.size()).first;
                mesh.vertices.push_back(position);
                mesh.vertex_normals.push_back(vec3::norm(normal));
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
