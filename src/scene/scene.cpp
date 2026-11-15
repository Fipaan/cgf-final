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

    object_t o{.mesh = d.mesh, .material = d.material, .pos = d.pos, .scale = d.scale};
    o.orient = mat4_t::trs(vec3_t<gl::f32>(), d.rot, vec3_t<gl::f32>(1.0f));

    if (d.body != body_kind_t::none) {
        const vec3_t<gl::f32>& mh = mesh_it->second.half;
        vec3_t<gl::f32> half = d.half.value_or(vec3_t<gl::f32>(mh.x * d.scale.x, mh.y * d.scale.y, mh.z * d.scale.z));

        if (d.body == body_kind_t::fixed) {
            o.body = (int)world.add(phys::body_t{.pos = d.pos, .half = half, .is_static = true});
        } else {
            const gl::f32 volume = d.shape == phys::shape_t::sphere ?
                4.0f / 3.0f * 3.14159265f * half.x * half.x * half.x : 8.0f * half.x * half.y * half.z;
            phys::rigid_t r;
            r.shape       = d.shape;
            r.pos         = d.pos;
            r.half        = half;
            r.orient      = o.orient;
            r.mass        = d.mass.value_or(volume * DENSITY);
            r.restitution = d.restitution;
            r.friction    = d.friction;
            o.rigid = (int)rigids.add(r);
        }
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
    if (it->second.body >= 0)  world.bodies[it->second.body].enabled = false;
    if (it->second.rigid >= 0) rigids.bodies[it->second.rigid].enabled = false;
    objects.erase(it);
    return true;
}

phys::body_t *scene_t::body_of(const object_t& o)
{
    return o.body < 0 ? nullptr : &world.bodies[o.body];
}

phys::rigid_t *scene_t::rigid_of(const object_t& o)
{
    return o.rigid < 0 ? nullptr : &rigids.bodies[o.rigid];
}

// distance along the ray to a box (rotated by `R`), -1 if missed or the ray starts inside
static gl::f32 ray_box(vec3_t<gl::f32> origin, vec3_t<gl::f32> dir, vec3_t<gl::f32> pos, vec3_t<gl::f32> half, const mat4_t& R, gl::f32 max_dist)
{
    using namespace phys::detail;
    const vec3_t<gl::f32> o = unrotate(R, origin - pos);
    const vec3_t<gl::f32> d = unrotate(R, dir);
    gl::f32 t_in = 0.0f, t_out = max_dist;
    for (int axis = 0; axis < 3; ++axis) {
        const gl::f32 oa = comp(o, axis), da = comp(d, axis), h = comp(half, axis);
        if (std::fabs(da) < 1e-8f) {
            if (oa < -h || oa > h) return -1.0f;
            continue;
        }
        gl::f32 t0 = (-h - oa) / da, t1 = (h - oa) / da;
        if (t0 > t1) std::swap(t0, t1);
        t_in  = std::max(t_in, t0);
        t_out = std::min(t_out, t1);
        if (t_in > t_out) return -1.0f;
    }
    return t_in > 0.0f ? t_in : -1.0f;
}

static gl::f32 ray_sphere(vec3_t<gl::f32> origin, vec3_t<gl::f32> dir, vec3_t<gl::f32> centre, gl::f32 radius, gl::f32 max_dist)
{
    const vec3_t<gl::f32> m = origin - centre;
    const gl::f32 b = vec3::dot(m, dir);
    const gl::f32 c = vec3::dot(m, m) - radius * radius;
    if (c > 0.0f && b > 0.0f) return -1.0f;
    const gl::f32 disc = b * b - c;
    if (disc < 0.0f) return -1.0f;
    const gl::f32 t = -b - std::sqrt(disc);
    return t > 0.0f && t < max_dist ? t : -1.0f;
}

pick_t scene_t::pick(vec3_t<gl::f32> origin, vec3_t<gl::f32> dir, gl::f32 max_dist, int ignore)
{
    pick_t hit;
    gl::f32 best = max_dist;
    int nearest_rigid = -1; // stays -1 if a wall or board is nearer

    for (size_t i = 0; i < world.bodies.size(); ++i) {
        const phys::body_t& b = world.bodies[i];
        if ((int)i == ignore || !b.enabled) continue;
        const gl::f32 t = ray_box(origin, dir, b.pos, b.half, mat4_t::identity(), best);
        if (t > 0.0f && t < best) { best = t; nearest_rigid = -1; }
    }
    for (size_t i = 0; i < rigids.bodies.size(); ++i) {
        const phys::rigid_t& r = rigids.bodies[i];
        if (!r.enabled) continue;
        const gl::f32 t = r.shape == phys::shape_t::sphere ?
            ray_sphere(origin, dir, r.pos, r.half.x, best) : ray_box(origin, dir, r.pos, r.half, r.orient, best);
        if (t > 0.0f && t < best) { best = t; nearest_rigid = (int)i; }
    }
    if (nearest_rigid < 0) return hit;

    for (const auto& [name, o] : objects) {
        if (o.rigid == nearest_rigid) {
            hit.name  = name;
            hit.dist  = best;
            hit.point = origin + dir * best;
            break;
        }
    }
    return hit;
}

void scene_t::update(gl::f32 dt)
{
    // fixed steps, the player moves first and is then pushed back by what it runs into
    _step_acc += std::min(dt, 0.1f);
    while (_step_acc >= world.step_dt) {
        world.step(world.step_dt);
        rigids.step(world.step_dt, world, player_body);
        _step_acc -= world.step_dt;
    }

    for (auto& [_name, o] : objects) {
        if (o.rigid < 0) continue;
        const phys::rigid_t& r = rigids.bodies[o.rigid];
        o.pos    = r.pos;
        o.orient = r.orient;
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
        const mat4_t model = mat4_t::translate(o.pos) * o.orient * mat4_t::scale(o.scale);
        shader.uniform(gl::m_type_t::m4x4, "model", 1, gl::gl_bool::FALSE, model.m);
        vec4("material_ambient",  m.ambient);
        vec4("material_diffuse",  m.diffuse);
        vec4("material_specular", m.specular);
        shader.uniform("material_shininess", m.shininess);

        if (!mesh_it->second.gpu.draw()) return false;
    }
    return true;
}
