/*
 * scene.hpp -
 *     Named meshes, materials and objects on top of `mesh_t` / `mesh_gpu_t`, plus the physics world.
 *     Header only.
 */
#pragma once

#include <algorithm>
#include <cmath>
#include <map>
#include <optional>
#include <string>
#include <utility>

#include <gl/mesh.hpp>
#include <gl/shader.hpp>
#include <basic.hpp>
#include <math/2d.hpp>
#include <math/3d.hpp>
#include <physics.hpp>

struct material_t {
    color_t ambient, diffuse, specular;
    GLfloat shininess;
};

enum class body_kind_t {
    none,    // visual only
    dynamic, // gravity + collisions
    fixed,   // collides, never moves
};

// everything needed to create an object, see `scene_t::add`
struct object_desc_t {
    std::string mesh;     // key from `add_mesh`
    std::string material; // key from `add_material`
    vec3_t<GLfloat> pos;
    vec3_t<GLfloat> rot;  // euler XYZ, radians, visual only (colliders are never rotated)
    vec3_t<GLfloat> scale = vec3_t<GLfloat>(1.0f);
    body_kind_t body = body_kind_t::none;
    // collider half extents, default is mesh bounds * scale
    std::optional<vec3_t<GLfloat>> half;
    GLfloat restitution = 0.0f;
    GLfloat friction    = 4.0f;
};

struct object_t {
    std::string mesh, material;
    vec3_t<GLfloat> pos, rot, scale;
    bool visible = true;
    int  body    = -1; // index into `scene_t::world.bodies`, -1 = not simulated
};

/* Usage:
```cpp
scene_t scene;
scene.add_mesh("cube", mesh_t::cube());
scene.add_material("wood", {...});

scene.add("crate", {.mesh = "cube", .material = "wood", .pos = {0, 3, 0}, .scale = vec3_t<GLfloat>(0.5f),
                    .body = body_kind_t::dynamic});
scene.get("crate")->material = "gold";
scene.remove("crate");

// every frame
scene.update(dt);
scene.draw(shader); // shader needs the `model` and `material_*` uniforms
```
*/
struct scene_t {
    phys::world_t world;
    std::map<std::string, material_t> materials;
    std::map<std::string, object_t>   objects;

    // uploads the mesh, normals must already be computed
    bool add_mesh(const std::string& name, const mesh_t& mesh);
    bool add_material(const std::string& name, const material_t& material);

    // nullptr if `name` is taken or the mesh / material is unknown
    object_t *add(const std::string& name, const object_desc_t& desc);
    object_t *get(const std::string& name);
    bool      remove(const std::string& name);

    phys::body_t *body_of(const object_t& object);

    void update(GLfloat dt);
    bool draw(gl::shader_t& shader);

private:
    struct mesh_entry_t {
        mesh_gpu_t gpu;
        vec3_t<GLfloat> half; // half extents of the bounds around the origin
    };
    std::map<std::string, mesh_entry_t> meshes;
};

///// IMPLEMENTATION ///////////////////////////////////////////////////////////

inline bool scene_t::add_mesh(const std::string& name, const mesh_t& mesh)
{
    if (meshes.contains(name) || mesh.vertices.empty()) return false;

    vec3_t<GLfloat> half;
    for (const auto& v : mesh.vertices) {
        half = vec3_t<GLfloat>(
            std::max(half.x, std::fabs(v.x)),
            std::max(half.y, std::fabs(v.y)),
            std::max(half.z, std::fabs(v.z))
        );
    }

    mesh_entry_t& e = meshes[name];
    if (!e.gpu.upload(mesh)) {
        meshes.erase(name);
        return false;
    }
    e.half = half;
    return true;
}

inline bool scene_t::add_material(const std::string& name, const material_t& material)
{
    return materials.emplace(name, material).second;
}

inline object_t *scene_t::add(const std::string& name, const object_desc_t& d)
{
    if (objects.contains(name)) return nullptr;
    auto mesh_it = meshes.find(d.mesh);
    if (mesh_it == meshes.end() || !materials.contains(d.material)) return nullptr;

    object_t o{.mesh = d.mesh, .material = d.material, .pos = d.pos, .rot = d.rot, .scale = d.scale};

    if (d.body != body_kind_t::none) {
        const vec3_t<GLfloat>& mh = mesh_it->second.half;
        vec3_t<GLfloat> half = d.half.value_or(vec3_t<GLfloat>(mh.x * d.scale.x, mh.y * d.scale.y, mh.z * d.scale.z));
        o.body = (int)world.add(phys::body_t{
            .pos = d.pos, .half = half,
            .restitution = d.restitution, .friction = d.friction,
            .is_static = d.body == body_kind_t::fixed,
        });
    }

    return &objects.emplace(name, std::move(o)).first->second;
}

inline object_t *scene_t::get(const std::string& name)
{
    auto it = objects.find(name);
    return it == objects.end() ? nullptr : &it->second;
}

inline bool scene_t::remove(const std::string& name)
{
    auto it = objects.find(name);
    if (it == objects.end()) return false;
    // bodies are addressed by index, so the slot stays and is just switched off
    if (it->second.body >= 0) world.bodies[it->second.body].enabled = false;
    objects.erase(it);
    return true;
}

inline phys::body_t *scene_t::body_of(const object_t& o)
{
    return o.body < 0 ? nullptr : &world.bodies[o.body];
}

inline void scene_t::update(GLfloat dt)
{
    world.update(dt);
    for (auto& [_name, o] : objects) {
        if (o.body >= 0) o.pos = world.bodies[o.body].pos;
    }
}

inline bool scene_t::draw(gl::shader_t& shader)
{
    auto vec4 = [&](const char *name, const color_t& c) { return shader.uniform(name, c.r, c.g, c.b, c.a); };

    for (auto& [_name, o] : objects) {
        if (!o.visible) continue;
        auto mesh_it = meshes.find(o.mesh);
        auto mat_it  = materials.find(o.material);
        if (mesh_it == meshes.end() || mat_it == materials.end()) continue;

        const material_t& m = mat_it->second;
        const mat4_t model = mat4_t::trs(o.pos, o.rot, o.scale);
        shader.uniform(gl::m_type_t::m4x4, "model", 1, gl::gl_bool::FALSE, model.m);
        vec4("material_ambient",  m.ambient);
        vec4("material_diffuse",  m.diffuse);
        vec4("material_specular", m.specular);
        shader.uniform("material_shininess", m.shininess);

        if (!mesh_it->second.gpu.draw()) return false;
    }
    return true;
}
