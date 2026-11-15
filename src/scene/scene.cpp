#include "scene.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

scene_t::scene_t()
{
    initialized = false;
}

bool scene_t::init(const char *vertex_path, const char *fragment_path)
{
    assert(!initialized);
    if (!shader.from_files(vertex_path, fragment_path)) return false;
    initialized = true;
    return true;
}

bool scene_t::add_mesh(const std::string& name, const mesh_t& mesh)
{
    if (meshes.contains(name) || mesh.vertices.empty()) return false;

    vec3_t<gl::f32> half;
    for (const auto& v : mesh.vertices) {
        half = vec3_t<gl::f32>(
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

bool scene_t::add_material(const std::string& name, const material_t& material)
{
    return materials.emplace(name, material).second;
}

object_t *scene_t::add(const std::string& name, const object_desc_t& d)
{
    if (objects.contains(name)) return nullptr;
    auto mesh_it = meshes.find(d.mesh);
    if (mesh_it == meshes.end() || !materials.contains(d.material)) return nullptr;

    object_t o{.mesh = d.mesh, .material = d.material, .pos = d.pos, .rot = d.rot, .scale = d.scale};

    if (d.body != body_kind_t::none) {
        const vec3_t<gl::f32>& mh = mesh_it->second.half;
        vec3_t<gl::f32> half = d.half.value_or(vec3_t<gl::f32>(mh.x * d.scale.x, mh.y * d.scale.y, mh.z * d.scale.z));
        o.body = (int)world.add(phys::body_t{
            .pos = d.pos, .half = half,
            .restitution = d.restitution, .friction = d.friction,
            .is_static = d.body == body_kind_t::fixed,
        });
    }

    return &objects.emplace(name, std::move(o)).first->second;
}

object_t *scene_t::get(const std::string& name)
{
    auto it = objects.find(name);
    return it == objects.end() ? nullptr : &it->second;
}

bool scene_t::remove(const std::string& name)
{
    auto it = objects.find(name);
    if (it == objects.end()) return false;
    // bodies are addressed by index, so the slot stays and is just switched off
    if (it->second.body >= 0) world.bodies[it->second.body].enabled = false;
    objects.erase(it);
    return true;
}

phys::body_t *scene_t::body_of(const object_t& o)
{
    return o.body < 0 ? nullptr : &world.bodies[o.body];
}

void scene_t::update(gl::f32 dt)
{
    world.update(dt);
    for (auto& [_name, o] : objects) {
        if (o.body >= 0) o.pos = world.bodies[o.body].pos;
    }
}

bool scene_t::draw(gl::f32 aspect)
{
    assert(initialized);

    const mat4_t view = cam.view();
    const mat4_t proj = cam.proj(aspect);

    // lights in eye space
    const vec3_t<gl::f32> l0_eye = view.transform_point(lamp.to_cart_y_up());
    const vec3_t<gl::f32> l1_eye = headlight_eye;

    shader.use();
    shader.uniform(gl::m_type_t::m4x4, "view", 1, gl::gl_bool::FALSE, view.m);
    shader.uniform(gl::m_type_t::m4x4, "proj", 1, gl::gl_bool::FALSE, proj.m);
    shader.uniform("light0_pos", l0_eye.x, l0_eye.y, l0_eye.z);
    shader.uniform("light1_pos", l1_eye.x, l1_eye.y, l1_eye.z);
    shader.uniform("light_ambient",  light_ambient.r,  light_ambient.g,  light_ambient.b,  light_ambient.a);
    shader.uniform("light_diffuse",  light_diffuse.r,  light_diffuse.g,  light_diffuse.b,  light_diffuse.a);
    shader.uniform("light_specular", light_specular.r, light_specular.g, light_specular.b, light_specular.a);
    shader.uniform("shading_mode", (int)shading);

    return _draw_objects();
}

bool scene_t::_draw_objects()
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
