#include "scene.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

using namespace math;

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

bool scene_t::add_model(const std::string& key, const obj_model_t& model)
{
    if (models.contains(key) || model.parts.empty()) return false;

    model_t t;
    t.center = (model.bounds_min + model.bounds_max) * 0.5f;
    t.half   = (model.bounds_max - model.bounds_min) * 0.5f;

    for (const obj_part_t& part : model.parts) {
        const obj_material_t& m = part.material;
        const std::string part_key = key + "/" + m.name;

        mesh_t mesh = part.mesh;
        for (vec3_t<gl::f32>& v : mesh.vertices) v = v - t.center;
        if (!add_mesh(part_key, mesh)) return false;

        add_material(part_key, material_t{
            .ambient   = color_t(m.diffuse.r + m.emission.r, m.diffuse.g + m.emission.g, m.diffuse.b + m.emission.b, 1.0f),
            .diffuse   = m.diffuse,
            .specular  = m.specular,
            .shininess = std::clamp(m.shininess, 1.0f, 128.0f),
        });
        t.parts.push_back({part_key, part_key});
    }
    models.emplace(key, std::move(t));
    return true;
}

object_t *scene_t::place_model(const std::string& key, const std::string& name, const model_desc_t& d)
{
    auto it = models.find(key);
    if (it == models.end() || objects.contains(name)) return nullptr;
    const model_t& m = it->second;

    const mat4_t R = mat4_t::trs(vec3_t<gl::f32>(), vec3_t<gl::f32>(0.0f, d.yaw, 0.0f), vec3_t<gl::f32>(1.0f));
    const vec3_t<gl::f32> centre = d.pos + R.transform_point(m.center * d.scale);

    vec3_t<gl::f32> half = m.half * d.scale;
    if (d.body == body_kind_t::fixed) {
        // axis aligned box around the turned one
        half = vec3_t<gl::f32>(std::fabs(R.at(0, 0)) * half.x + std::fabs(R.at(0, 2)) * half.z, half.y,
                               std::fabs(R.at(2, 0)) * half.x + std::fabs(R.at(2, 2)) * half.z);
    }

    object_t *root = nullptr;
    for (size_t i = 0; i < m.parts.size(); ++i) {
        object_desc_t od;
        od.mesh     = m.parts[i].mesh;
        od.material = m.parts[i].material;
        od.pos      = centre;
        od.rot      = vec3_t<gl::f32>(0.0f, d.yaw, 0.0f);
        od.scale    = vec3_t<gl::f32>(d.scale);
        if (i == 0) {
            od.body         = d.body;
            od.half         = half;
            od.restitution  = d.restitution;
            od.friction     = d.friction;
            od.mass         = d.mass;
            od.prompt       = d.prompt;
            od.on_interact  = d.on_interact;
            od.on_collision = d.on_collision;
        }
        object_t *o = add(i == 0 ? name : name + "/" + m.parts[i].mesh.substr(key.size() + 1), od);
        if (o == nullptr) return nullptr;
        if (i == 0) root = o;
        else o->parent = name;
    }
    return root;
}

object_t *scene_t::add(const std::string& name, const object_desc_t& d)
{
    if (objects.contains(name)) return nullptr;
    auto mesh_it = meshes.find(d.mesh);
    if (mesh_it == meshes.end() || !materials.contains(d.material)) return nullptr;

    object_t o{.mesh = d.mesh, .material = d.material, .pos = d.pos, .scale = d.scale};
    o.orient = mat4_t::trs(vec3_t<gl::f32>(), d.rot, vec3_t<gl::f32>(1.0f));

    o.prompt       = d.prompt;
    o.on_interact  = d.on_interact;
    o.on_collision = d.on_collision;

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
    for (auto child = objects.begin(); child != objects.end();) {
        if (child->second.parent == name) child = objects.erase(child);
        else ++child;
    }
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
    const vec3_t<gl::f32> o = phys::unrotate(R, origin - pos);
    const vec3_t<gl::f32> d = phys::unrotate(R, dir);
    gl::f32 t_in = 0.0f, t_out = max_dist;
    for (int axis = 0; axis < 3; ++axis) {
        const gl::f32 oa = o.v[axis], da = d.v[axis], h = half.v[axis];
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

trigger_t *scene_t::add_trigger(const std::string& name, const trigger_t& trigger)
{
    auto [it, added] = triggers.emplace(name, trigger);
    return added ? &it->second : nullptr;
}

bool scene_t::remove_trigger(const std::string& name)
{
    return triggers.erase(name) > 0;
}

pick_t scene_t::pick(vec3_t<gl::f32> origin, vec3_t<gl::f32> dir, gl::f32 max_dist, int ignore)
{
    pick_t hit;
    gl::f32 best = max_dist;
    int nearest_body = -1, nearest_rigid = -1;

    for (size_t i = 0; i < world.bodies.size(); ++i) {
        const phys::body_t& b = world.bodies[i];
        if ((int)i == ignore || !b.enabled) continue;
        const gl::f32 t = ray_box(origin, dir, b.pos, b.half, mat4_t::id(), best);
        if (t > 0.0f && t < best) { best = t; nearest_body = (int)i; nearest_rigid = -1; }
    }
    for (size_t i = 0; i < rigids.bodies.size(); ++i) {
        const phys::rigid_t& r = rigids.bodies[i];
        if (!r.enabled) continue;
        const gl::f32 t = r.shape == phys::shape_t::sphere ?
            ray_sphere(origin, dir, r.pos, r.half.x, best) : ray_box(origin, dir, r.pos, r.half, r.orient, best);
        if (t > 0.0f && t < best) { best = t; nearest_rigid = (int)i; nearest_body = -1; }
    }
    if (nearest_rigid < 0 && nearest_body < 0) return hit;

    // a wall or a board that is no object of its own (or the player's own body) still hides what is behind
    for (const auto& [name, o] : objects) {
        if ((nearest_rigid >= 0 && o.rigid == nearest_rigid) || (nearest_body >= 0 && o.body == nearest_body)) {
            hit.name    = name;
            hit.dist    = best;
            hit.point   = origin + dir * best;
            hit.movable = nearest_rigid >= 0;
            break;
        }
    }
    return hit;
}

bool scene_t::interact(vec3_t<gl::f32> origin, vec3_t<gl::f32> dir, gl::f32 reach, int ignore)
{
    const pick_t hit = pick(origin, dir, reach, ignore);
    if (hit.name.empty()) return false;
    const object_t *o = get(hit.name);
    if (o == nullptr || !o->on_interact) return false;

    interact_fn_t callback = o->on_interact; // the callback may remove the object
    callback(*this, hit.name);
    return true;
}

// pairs that start pressing on each other become events, one that merely flickers off and on doesn't
void scene_t::_collect_touches()
{
    std::set<std::tuple<int, int, int>> now;
    for (const phys::touch_t& t : rigids.touches) {
        const auto key = std::make_tuple(t.a, (int)t.kind, t.b);
        now.insert(key);
        auto it = _touching.find(key);
        if (it == _touching.end()) _events.push_back({t.a, t.kind, t.b, t.impulse});
        _touching[key] = 0;
    }
    for (auto it = _touching.begin(); it != _touching.end();) {
        if (!now.contains(it->first) && ++it->second > 6) it = _touching.erase(it);
        else ++it;
    }
}

void scene_t::_dispatch_events()
{
    // who is who, indices are not stable names
    std::vector<const std::string *> rigid_name(rigids.bodies.size(), nullptr), body_name(world.bodies.size(), nullptr);
    for (const auto& [name, o] : objects) {
        if (o.rigid >= 0) rigid_name[o.rigid] = &name;
        if (o.body  >= 0) body_name[o.body]   = &name;
    }

    // copied: callbacks are free to add and remove things
    std::vector<std::tuple<std::string, std::string, gl::f32>> calls; // self, other, impulse
    for (const touch_event_t& e : _events) {
        const std::string a = rigid_name[e.a] ? *rigid_name[e.a] : "";
        std::string b;
        if (e.kind == phys::touch_t::rigid)       b = rigid_name[e.b] ? *rigid_name[e.b] : "";
        else if (e.kind == phys::touch_t::fixed)  b = body_name[e.b] ? *body_name[e.b] : "";
        else                                      b = "player";
        if (!a.empty() && !b.empty()) {
            calls.emplace_back(a, b, e.impulse);
            if (e.kind != phys::touch_t::player) calls.emplace_back(b, a, e.impulse);
        }
    }
    _events.clear();

    for (const auto& [self, other, impulse] : calls) {
        const object_t *o = get(self);
        if (o == nullptr || !o->on_collision) continue;
        collision_fn_t callback = o->on_collision;
        callback(*this, self, other, impulse);
    }

    // triggers: who is inside now, compared with who was
    std::vector<std::string> trigger_names;
    for (const auto& [tname, _trig] : triggers) trigger_names.push_back(tname);
    for (const std::string& tname : trigger_names) {
        auto trig_it = triggers.find(tname); // callbacks of earlier triggers may have removed this one
        if (trig_it == triggers.end()) continue;
        trigger_t& trig = trig_it->second;
        std::set<std::string> now;
        for (const auto& [name, o] : objects) {
            if (o.rigid < 0 || !rigids.bodies[o.rigid].enabled) continue;
            const vec3_t<gl::f32> d = o.pos - trig.pos;
            if (std::fabs(d.x) > trig.half.x || std::fabs(d.y) > trig.half.y || std::fabs(d.z) > trig.half.z) continue;
            if (trig.filter && !trig.filter(name, o)) continue;
            now.insert(name);
        }
        const std::set<std::string> before = trig.inside;
        trig.inside = now;
        const std::string trigger_name = tname;
        const trigger_fn_t on_enter = trig.on_enter, on_exit = trig.on_exit;
        for (const std::string& n : now)    if (!before.contains(n) && on_enter) on_enter(*this, trigger_name, n);
        for (const std::string& n : before) if (!now.contains(n)    && on_exit)  on_exit(*this, trigger_name, n);
    }
}

void scene_t::update(gl::f32 dt)
{
    // fixed steps, the player moves first and is then pushed back by what it runs into
    _step_acc += std::min(dt, 0.1f);
    while (_step_acc >= world.step_dt) {
        world.step(world.step_dt);
        rigids.step(world.step_dt, world, player_body);
        _collect_touches();
        _step_acc -= world.step_dt;
    }

    for (auto& [_name, o] : objects) {
        if (o.rigid < 0) continue;
        const phys::rigid_t& r = rigids.bodies[o.rigid];
        o.pos    = r.pos;
        o.orient = r.orient;
    }
    for (auto& [_name, o] : objects) {
        if (o.parent.empty()) continue;
        if (const object_t *p = get(o.parent)) {
            o.pos    = p->pos;
            o.orient = p->orient;
        }
    }

    _dispatch_events();
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
