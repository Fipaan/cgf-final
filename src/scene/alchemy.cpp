#include "alchemy.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <set>
#include <string_view>

#include <scene/room.hpp>

using namespace math;
using V = vec3_t<gl::f32>;

constexpr gl::f32 FLOAT_GRAVITY = 0.4f;
constexpr gl::f32 WAVE_GRAVITY  = 5.0f;

brew_t::brew_t(scene_t& scene, player_t& player, alchemy::ui_t& ui) :
    scene(scene),
    player(player),
    ui(ui)
{}

gl::f32 brew_t::rand(gl::f32 lo, gl::f32 hi)
{
    return std::uniform_real_distribution<gl::f32>(lo, hi)(rng);
}

void brew_t::setup(trigger_t *bowl_trigger, V spawn_point)
{
    bowl  = bowl_trigger;
    spawn = spawn_point;
    if (bowl != nullptr) bowl_centre = V(bowl->pos.x, bowl->pos.y + 0.6f, bowl->pos.z);

    // bottles: dynamic objects whose model is a potion. The ingredient goes round the six of them, so every
    // one is on the shelves, and the liquid takes its colour
    std::vector<std::string> bottles;
    for (const auto& [name, o] : scene.objects) {
        if (o.rigid >= 0 && o.parent.empty() && o.mesh.starts_with("Potion_")) bottles.push_back(name);
    }
    for (size_t k = 0; k < bottles.size(); ++k) {
        const int id = (int)(k % alchemy::ingredients.size());
        const std::string& name = bottles[k];
        ingredient_of[name] = id;

        for (auto& [part_name, part] : scene.objects) {
            if (part.parent != name || !part_name.ends_with("_Elixir")) continue;
            const std::string key = std::format("elixir/{}", id);
            if (!scene.materials.contains(key)) {
                material_t m = scene.materials.at(part.material);
                const auto& c = alchemy::ingredient_colors[(size_t)id];
                m.diffuse = color_t(c[0], c[1], c[2], 1.0f);
                m.ambient = color_t(c[0] * 1.15f, c[1] * 1.15f, c[2] * 1.15f, 1.0f);
                scene.add_material(key, m);
            }
            part.material = key;
        }

        scene.objects[name].prompt = "Inspect";
        scene.objects[name].on_interact = [this](scene_t&, const std::string& self) {
            if (bowl != nullptr && bowl->inside.contains(self)) { brew(); return; }
            const auto it = ingredient_of.find(self);
            if (it != ingredient_of.end()) ui.set_status(std::format("{} essence.", alchemy::ingredients[(size_t)it->second]));
        };
    }

    // the cauldron: E on its body or on the bowl, and only bottles count as what is inside
    for (auto& [name, o] : scene.objects) {
        if (!name.starts_with("cauldron") || o.body < 0) continue;
        o.prompt      = "Brew";
        o.on_interact = [this](scene_t&, const std::string&) { brew(); };
    }
    if (bowl != nullptr) {
        bowl->filter   = [this](const std::string& name, const object_t&) { return ingredient_of.contains(name); };
        bowl->on_enter = [this](scene_t&, const std::string&, const std::string& object) {
            ui.set_status(std::format("{} dropped in. Cauldron: {}. Press E to brew.",
                                      alchemy::ingredients[(size_t)ingredient_of[object]], contents()));
        };
        bowl->on_exit = [this](scene_t&, const std::string&, const std::string&) {
            const std::string now = contents();
            ui.set_status(now.empty() ? "The cauldron is empty." : "Cauldron: " + now + ". Press E to brew.");
        };
    }

    // where everything starts
    for (const auto& [name, o] : scene.objects) {
        if (o.rigid < 0 || !o.parent.empty()) continue;
        const phys::rigid_t& r = scene.rigids.bodies[(size_t)o.rigid];
        home[name] = home_t{.pos = o.pos, .scale = o.scale, .half = r.half, .orient = o.orient, .mass = r.mass, .restitution = r.restitution};
    }
    for (const auto& [name, o] : scene.objects) {
        if (o.parent.empty() || !home.contains(o.parent)) continue;
        home[name] = home_t{.pos = o.pos, .scale = o.scale, .orient = o.orient};
        parts_of[o.parent].push_back(name);
    }
}

std::string brew_t::contents() const
{
    if (bowl == nullptr) return "";
    std::set<int> ids;
    for (const std::string& name : bowl->inside) {
        const auto it = ingredient_of.find(name);
        if (it != ingredient_of.end()) ids.insert(it->second);
    }
    std::string text;
    for (int id : ids) text += (text.empty() ? "" : " + ") + std::string(alchemy::ingredients[(size_t)id]);
    return text;
}

void brew_t::brew()
{
    if (bowl == nullptr) return;

    std::set<int> ids;
    std::vector<std::string> used;
    for (const std::string& name : bowl->inside) {
        const auto it = ingredient_of.find(name);
        if (it == ingredient_of.end()) continue;
        ids.insert(it->second);
        used.push_back(name);
    }
    if (used.empty()) { ui.set_status("The cauldron is empty. Drop some bottles in first."); return; }
    if (ids.size() > 3) { ui.set_status("Too many different ingredients: three at most."); return; }

    std::array<int, 3> selection = {-1, -1, -1};
    int count = 0;
    for (int id : ids) selection[(size_t)count++] = id;

    const auto found = alchemy::find_recipe(selection, count);
    if (!found) { 
        scene.particles.smoke(bowl->pos + V(0.0f, 0.2f, 0.0f), 12, V(0.6f, 0.6f, 0.6f));
        ui.set_status("The mixture only bubbles. No known reaction.");
        return;
    }

    // the bottles are used up
    for (const std::string& name : used) {
        bowl->inside.erase(name);
        ingredient_of.erase(name);
        home.erase(name);
        if (const auto parts = parts_of.find(name); parts != parts_of.end()) {
            for (const std::string& part : parts->second) home.erase(part);
            parts_of.erase(parts);
        }
        scene.remove(name);
    }

    const alchemy::recipe_t& recipe = alchemy::recipes[*found];
    ui.record_recipe(recipe);
    apply(recipe);

    // the cauldron flashes in the colour of what came out
    switch (recipe.kind) {
        case alchemy::result_kind::basic:   flash_color = V(1.0f, 0.8f, 0.4f); break;
        case alchemy::result_kind::success: flash_color = V(0.3f, 1.0f, 0.5f); break;
        case alchemy::result_kind::danger:  flash_color = V(1.0f, 0.45f, 0.1f); break;
        case alchemy::result_kind::failure: flash_color = V(0.5f, 0.5f, 0.55f); break;
        case alchemy::result_kind::secret:  flash_color = V(0.7f, 0.3f, 1.0f); break;
    }
    flash = 1.0f;
    scene.particles.fountain(bowl->pos + V(0.0f, 0.2f, 0.0f), 90, flash_color);
}

std::vector<std::string> brew_t::loose() const
{
    std::vector<std::string> names;
    for (const auto& [name, o] : scene.objects) {
        if (o.rigid >= 0 && o.parent.empty() && home.contains(name)) names.push_back(name);
    }
    return names;
}

void brew_t::scale_loose(const std::function<gl::f32(const std::string& name)>& factor)
{
    for (const std::string& name : loose()) {
        object_t& o = scene.objects[name];
        const home_t& h = home[name];
        phys::rigid_t& r = scene.rigids.bodies[(size_t)o.rigid];
        const gl::f32 s = factor(name);

        const gl::f32 old_half_y = r.half.y;
        r.half = h.half * s;
        r.mass = h.mass * s * s * s;
        if (r.half.y > old_half_y) r.pos.y += r.half.y - old_half_y; // grows out of the floor, not into it
        o.pos   = r.pos;
        o.scale = h.scale * s;
        if (const auto parts = parts_of.find(name); parts != parts_of.end()) {
            for (const std::string& part : parts->second) {
                if (object_t *p = scene.get(part)) p->scale = home[part].scale * s;
            }
        }
    }
}

void brew_t::scatter_loose(gl::f32 half_x, gl::f32 y0, gl::f32 y1, gl::f32 half_z)
{
    for (const std::string& name : loose()) {
        object_t& o = scene.objects[name];
        phys::rigid_t& r = scene.rigids.bodies[(size_t)o.rigid];
        r.pos = V(rand(-half_x, half_x), rand(y0, y1), rand(-half_z, half_z));
        r.vel = r.angvel = V();
        o.pos = r.pos;
    }
}

void brew_t::set_float(bool on)
{
    scene.rigids.gravity = V(0.0f, on ? FLOAT_GRAVITY : -9.81f, 0.0f);
    if (!on) return;
    for (const std::string& name : loose()) {
        scene.rigids.bodies[(size_t)scene.objects[name].rigid].vel.y += 1.5f; // gets them off the boards
    }
}

void brew_t::blast(V centre, gl::f32 radius, gl::f32 strength)
{
    scene.particles.sparks(centre, (int)(7.0f * strength), 0.7f * strength, V(1.0f, 0.55f, 0.15f));
    scene.particles.smoke(centre, (int)(strength * 0.8f), V(0.8f, 0.6f, 0.45f));
    for (const std::string& name : loose()) {
        phys::rigid_t& r = scene.rigids.bodies[(size_t)scene.objects[name].rigid];
        const V d = r.pos - centre;
        const gl::f32 dist = vec3::len(d);
        if (dist >= radius) continue;
        const gl::f32 k = strength * (1.0f - dist / radius);
        const V dir = vec3::norm(d + V(0.0f, 0.3f, 0.0f));
        r.vel    += dir * k + V(0.0f, 0.5f * k, 0.0f);
        r.angvel += V(rand(-1.0f, 1.0f), rand(-1.0f, 1.0f), rand(-1.0f, 1.0f)) * (0.5f * k);
    }
    // the player only gets thrown up: the horizontal speed is decided by the keys
    if (player.mode == cam_mode_t::fps) {
        phys::body_t& p = player.get_body();
        const gl::f32 dist = vec3::len(p.pos - centre);
        if (dist < radius) p.vel.y += 0.4f * strength * (1.0f - dist / radius);
    }
}

void brew_t::shake(gl::f32 seconds, gl::f32 amplitude)
{
    shake_left = shake_total = seconds;
    shake_amp  = amplitude;
}

void brew_t::set_restitution(gl::f32 value)
{
    for (const std::string& name : loose()) {
        scene.rigids.bodies[(size_t)scene.objects[name].rigid].restitution = value;
    }
}

// sizes, positions, velocities: back to the start
void brew_t::restore_objects()
{
    scene.rigids.gravity = V(0.0f, -9.81f, 0.0f);
    for (const std::string& name : loose()) {
        object_t& o = scene.objects[name];
        const home_t& h = home[name];
        phys::rigid_t& r = scene.rigids.bodies[(size_t)o.rigid];
        r.half        = h.half;
        r.mass        = h.mass;
        r.restitution = h.restitution;
        r.pos    = h.pos;
        r.orient = h.orient;
        r.vel = r.angvel = V();
        o.pos    = h.pos;
        o.orient = h.orient;
        o.scale  = h.scale;
        if (const auto parts = parts_of.find(name); parts != parts_of.end()) {
            for (const std::string& part : parts->second) {
                if (object_t *p = scene.get(part)) p->scale = home[part].scale;
            }
        }
    }
}

void brew_t::reset_player()
{
    player.set_scale(1.0f);
    player.speed_mult = 1.0f;
    player.jump_mult  = 1.0f;
}

void brew_t::restore_all()
{
    cancel_timers();
    shake_left = 0.0f;
    reset_player();
    restore_objects();
}

void brew_t::add_timer(timer_t timer)
{
    timers.push_back(std::move(timer));
}

void brew_t::cancel_timers()
{
    std::vector<timer_t> old = std::move(timers);
    timers.clear();
    for (timer_t& t : old) if (t.on_end) t.on_end();
}

void brew_t::update(gl::f32 dt)
{
    std::vector<timer_t> finished;
    for (size_t i = 0; i < timers.size();) {
        timer_t& t = timers[i];
        const gl::f32 step = std::min(dt, t.left);
        if (t.on_frame) t.on_frame(step);
        t.left -= step;
        t.acc  += step;
        while (t.period > 0.0f && t.acc >= t.period) {
            t.acc -= t.period;
            if (t.on_tick) t.on_tick(t.ticks);
            ++t.ticks;
        }
        if (t.left <= 0.0f) {
            finished.push_back(std::move(t));
            timers.erase(timers.begin() + (std::ptrdiff_t)i);
        } else {
            ++i;
        }
    }
    for (timer_t& t : finished) if (t.on_end) t.on_end();

    if (shake_left > 0.0f) shake_left = std::max(0.0f, shake_left - dt);
    update_lights(dt);
    update_particles(dt);
}

void brew_t::update_particles(gl::f32 dt)
{
    particles_t& ps = scene.particles;
    const bool full = bowl != nullptr && !bowl->inside.empty();

    // the cauldron: thin steam always, bubbles that pop on the surface when there is something in it
    if (bowl != nullptr) {
        const V surface = V(bowl->pos.x, bowl->pos.y + bowl->half.y * 0.4f, bowl->pos.z);
        steam_acc += dt * (full ? 40.0f : 14.0f);
        for (; steam_acc >= 1.0f; steam_acc -= 1.0f) {
            particle_t p;
            p.pos = surface + V(rand(-0.22f, 0.22f), 0.0f, rand(-0.22f, 0.22f));
            p.vel = V(rand(-0.05f, 0.05f), rand(0.3f, 0.6f), rand(-0.05f, 0.05f));
            p.color = bowl_tint * 0.7f;
            p.life = rand(1.6f, 2.8f);
            p.size0 = 0.06f; p.size1 = 0.28f;
            p.alpha = 0.16f; p.drag = 0.3f;
            ps.emit(p);
            if (full && rand(0.0f, 1.0f) < 0.4f) { // a bubble: small, bright, short-lived
                particle_t b = p;
                b.pos = surface + V(rand(-0.2f, 0.2f), 0.02f, rand(-0.2f, 0.2f));
                b.vel = V(0.0f, rand(0.1f, 0.3f), 0.0f);
                b.color = bowl_tint * 1.2f;
                b.life = rand(0.3f, 0.7f);
                b.size0 = 0.03f; b.size1 = 0.05f;
                b.alpha = 0.8f; b.drag = 0.0f;
                ps.emit(b);
            }
        }
    }

    // embers rising from every lit candle
    ember_acc += dt * 3.0f;
    for (; ember_acc >= 1.0f; ember_acc -= 1.0f) {
        for (const V& top : flames) {
            particle_t p;
            p.pos = top + V(rand(-0.01f, 0.01f), 0.0f, rand(-0.01f, 0.01f));
            p.vel = V(rand(-0.06f, 0.06f), rand(0.2f, 0.4f), rand(-0.06f, 0.06f));
            p.color = V(1.0f, 0.55f, 0.2f);
            p.life = rand(0.8f, 1.5f);
            p.size0 = 0.014f; p.size1 = 0.004f;
            p.alpha = 0.9f; p.drag = 0.4f;
            ps.emit(p);
        }
    }

    // dust drifting in the light of the lamp
    dust_acc += dt * 8.0f;
    const V lamp = scene.lamp.to_cart_y_up();
    for (; dust_acc >= 1.0f; dust_acc -= 1.0f) {
        particle_t p;
        p.pos = V(std::clamp(lamp.x + rand(-3.0f, 3.0f), -ROOM_X + 0.3f, ROOM_X - 0.3f),
                  std::clamp(lamp.y + rand(-2.2f, 0.5f), 0.3f, ROOM_H - 0.3f),
                  std::clamp(lamp.z + rand(-3.0f, 3.0f), -ROOM_Z + 0.3f, ROOM_Z - 0.3f));
        p.vel = V(rand(-0.04f, 0.04f), rand(-0.02f, 0.05f), rand(-0.04f, 0.04f));
        p.color = V(1.0f, 0.85f, 0.6f);
        p.life = rand(5.0f, 9.0f);
        p.size0 = p.size1 = 0.012f;
        p.alpha = 0.5f;
        ps.emit(p);
    }
}

void brew_t::update_lights(gl::f32 dt)
{
    flames.clear();
    // every lit candle is a small flickering light
    for (const auto& [name, o] : scene.objects) {
        if (!name.starts_with("candle") || name.contains('/')) continue;
        const std::string key = "candle:" + name;
        const object_t *flame = scene.get(name + "/Flame_Core");
        const phys::rigid_t *r = scene.rigid_of(o);
        if (flame == nullptr || !flame->visible || r == nullptr) { scene.lights.erase(key); continue; }

        const gl::f32 phase = (gl::f32)(std::hash<std::string>{}(name) % 628) / 100.0f;
        const gl::f32 flick = 0.85f + 0.10f * std::sin(scene.time * 17.0f + phase) + 0.05f * std::sin(scene.time * 29.0f + 2.0f * phase);
        const V top = o.pos + V(0.0f, r->half.y + 0.05f, 0.0f);
        flames.push_back(top);
        scene.lights[key] = point_light_t{.pos = top, .color = V(1.0f, 0.55f, 0.2f) * (0.9f * flick), .radius = 3.0f};
    }

    // the cauldron: a faint glow, the colour of the ingredients inside it, a flash after a brew
    if (bowl == nullptr) return;
    V tint(0.15f, 0.5f, 0.45f);
    gl::f32 power = 0.35f;
    if (!bowl->inside.empty()) {
        V sum;
        int n = 0;
        for (const std::string& name : bowl->inside) {
            const auto it = ingredient_of.find(name);
            if (it == ingredient_of.end()) continue;
            const auto& c = alchemy::ingredient_colors[(size_t)it->second];
            sum += V(c[0], c[1], c[2]);
            ++n;
        }
        if (n > 0) { tint = sum * (1.0f / (gl::f32)n); power = 1.0f; }
    }
    bowl_tint = tint;
    flash = std::max(0.0f, flash - dt / 1.2f);
    tint  = tint + (flash_color - tint) * flash;
    power *= (1.0f + 0.1f * std::sin(scene.time * 3.0f)) + 3.0f * flash;
    scene.lights["cauldron"] = point_light_t{.pos = bowl->pos + V(0.0f, 0.15f, 0.0f), .color = tint * power, .radius = 3.5f};
}

void brew_t::shake_begin()
{
    shake_offset = V();
    if (shake_left <= 0.0f) return;
    const gl::f32 a = shake_amp * (shake_left / shake_total);
    shake_offset = V(rand(-a, a), rand(-a, a), rand(-a, a));
    scene.cam.pos += shake_offset;
}

void brew_t::shake_end()
{
    scene.cam.pos -= shake_offset;
    shake_offset = V();
}

void brew_t::apply(const alchemy::recipe_t& recipe)
{
    // a new potion ends what the last one was doing
    cancel_timers();
    const std::string_view id = recipe.id;
    const auto all = [](gl::f32 s) { return [s](const std::string&) { return s; }; };

    // base potions
    if (id == "growth")       player.set_scale(2.0f);
    else if (id == "shrink")  player.set_scale(0.5f);
    else if (id == "speed")   player.speed_mult = 2.0f;
    else if (id == "gravity") set_float(true);
    else if (id == "explosion") blast(bowl_centre, 4.0f, 8.0f);
    else if (id == "restore") restore_all();

    // two ingredients
    else if (id == "unstable_size") {
        const auto resize = [this, all](bool big) {
            const gl::f32 s = big ? 2.0f : 0.5f;
            player.set_scale(s);
            scale_loose(all(s));
        };
        resize(true);
        add_timer({.left = 15.0f, .period = 3.0f,
                   .on_tick = [resize, big = true](int) mutable { big = !big; resize(big); },
                   .on_end  = [this, all] { player.set_scale(1.0f); scale_loose(all(1.0f)); }});
    }
    else if (id == "giant_run") {
        player.set_scale(2.0f);
        player.speed_mult = 1.5f;
    }
    else if (id == "giant_room") {
        scale_loose(all(3.0f));
        set_float(true);
    }
    else if (id == "mega_explosion") {
        scale_loose(all(1.5f));
        blast(bowl_centre, 7.0f, 16.0f);
    }
    else if (id == "tiny_ninja") {
        player.set_scale(0.25f);
        player.speed_mult = 3.0f;
    }
    else if (id == "micro_world") {
        scale_loose(all(0.2f));
        set_float(true);
    }
    else if (id == "mini_bombs") {
        for (int i = 0; i < 5; ++i) blast(V(rand(-5.0f, 5.0f), 0.3f, rand(-4.0f, 4.0f)), 2.5f, 6.0f);
    }
    else if (id == "super_jump") {
        player.speed_mult = 2.0f;
        player.jump_mult  = 4.0f;
    }
    else if (id == "chaotic_acceleration") {
        player.speed_mult = 2.0f;
        set_restitution(0.95f);
        add_timer({.left = 10.0f, .period = 0.5f,
                   .on_tick = [this](int) {
                       for (const std::string& name : loose()) {
                           phys::rigid_t& r = scene.rigids.bodies[(size_t)scene.objects[name].rigid];
                           r.vel += V(rand(-4.0f, 4.0f), rand(1.0f, 5.0f), rand(-4.0f, 4.0f));
                       }
                   },
                   .on_end = [this] {
                       player.speed_mult = 1.0f;
                       for (const std::string& name : loose()) {
                           scene.rigids.bodies[(size_t)scene.objects[name].rigid].restitution = home[name].restitution;
                       }
                   }});
    }
    else if (id == "reverse_acceleration") {
        player.speed_mult = 0.3f;
        add_timer({.left = 15.0f, .period = 0.0f, .on_end = [this] { player.speed_mult = 1.0f; }});
    }
    else if (id == "anti_gravity_wave") {
        scene.rigids.gravity = V(0.0f, WAVE_GRAVITY, 0.0f);
        for (const std::string& name : loose()) scene.rigids.bodies[(size_t)scene.objects[name].rigid].vel.y += 3.0f;
        add_timer({.left = 4.0f, .period = 0.0f, .on_end = [this] { scene.rigids.gravity = V(0.0f, -9.81f, 0.0f); }});
    }
    else if (id == "reverse_explosion") {
        add_timer({.left = 2.5f, .period = 0.0f,
                   .on_frame = [this](gl::f32) {
                       int i = 0;
                       for (const std::string& name : loose()) {
                           phys::rigid_t& r = scene.rigids.bodies[(size_t)scene.objects[name].rigid];
                           const gl::f32 a = 2.4f * (gl::f32)i;
                           const V target = bowl_centre + V(0.15f * std::cos(a), 0.1f * (gl::f32)(i % 5), 0.15f * std::sin(a));
                           V v = (target - r.pos) * 4.0f;
                           if (vec3::len(v) > 10.0f) v = vec3::norm(v) * 10.0f;
                           r.vel = v;
                           ++i;
                       }
                   }});
    }

    // three ingredients
    else if (id == "room_chaos") {
        const auto shuffle = [this] {
            std::map<std::string, gl::f32> sizes;
            for (const std::string& name : loose()) sizes[name] = rand(0.4f, 2.0f);
            scale_loose([&sizes](const std::string& name) { return sizes[name]; });
            scatter_loose(6.0f, 0.6f, 2.5f, 4.0f);
        };
        shuffle();
        add_timer({.left = 20.0f, .period = 2.0f,
                   .on_tick = [shuffle](int) { shuffle(); },
                   .on_end  = [this, all] { scale_loose(all(1.0f)); }});
    }
    else if (id == "apocalypse") {
        scale_loose(all(2.0f));
        blast(bowl_centre, 10.0f, 22.0f);
        shake(3.0f, 0.12f);
    }
    else if (id == "time_rewind") {
        restore_all();
        player.teleport(spawn);
    }
    // the neutralizations (failure) do nothing on purpose
}
