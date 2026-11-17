#include <algorithm>
#include <format>
#include <string>

#include <core/log.hpp>
#include <core/gl/first.hpp>
#include <3d/mesh.hpp>
#include <gl/ctx.hpp>
#include <gl/draw.hpp>
#include <gl/text.hpp>
#include <gl/gui.hpp>
#include <ui/alchemy.hpp>
#include <scene/scene.hpp>
#include <scene/player.hpp>
#include <scene/drag.hpp>
#include <scene/room.hpp>

constexpr gl::f32 INTERACT_REACH = 3.0f; // how far E reaches

struct app_t {
    gl::ctx_t&    ctx;
    gl::draw2d_t  draw2d;
    gl::text_t    text;
    gl::gui_t     gui;
    alchemy::ui_t alchemy_ui;

    scene_t  scene;
    player_t player;
    drag_t   drag;

    // `M` cycles the loaded model through `room::prop_materials`
    size_t model_material = 0;

    // what E would do for the object under the crosshair, empty = nothing
    std::string interact_prompt;
    int cans_filled = 0;

    bool    hud_visible   = true;
    gl::f32 hud_text_size = 20.0f;

    app_t(gl::ctx_t& c) :
        ctx(c),
        draw2d(c),
        text(c),
        gui(c, draw2d, text),
        alchemy_ui(gui),
        player(scene),
        drag(scene)
    {}

    bool init(const char *smf_path)
    {
        mesh_t mesh; // loaded model
        if (!mesh.load_smf(smf_path)) return false;
        mesh.norm();
        mesh.compute_normals();

        if (!scene.init("shaders/mesh.vert", "shaders/mesh.frag")) return false;
        if (!scene.add_mesh("model",  mesh))             return false;
        if (!scene.add_mesh("cube",   mesh_t::cube()))   return false;
        if (!scene.add_mesh("sphere", mesh_t::sphere())) return false;
        if (!scene.add_mesh("plane",  mesh_t::plane()))  return false;

        room::add_materials(scene);
        room::build_room(scene);
        room::build_shelves(scene);
        room::build_props(scene);
        player.spawn(vec3_t<gl::f32>(0, 0, 4));
        setup_interactions();

        if (!draw2d.init()) return false;
        FT_Long face_index = 0;
        if (!text.init("./assets/fonts/iosevka-regular.ttf", face_index, "./shaders/text.vert", "./shaders/text.frag")) return false;
        if (!alchemy_ui.init()) return false;
        ctx.set_vsync(true);

        glEnable(GL_DEPTH_TEST);
        ctx.clear_color(0x181818FF);
        return true;
    }

    // The three ways objects can react, as an example: E on the spheres, a trigger over the can, a collision on the gong.
    void setup_interactions()
    {
        // E: lights the sphere up or off again
        for (auto& [name, o] : scene.objects) {
            if (o.mesh != "sphere" || o.rigid < 0) continue;
            o.prompt = "Activate";
            o.on_interact = [this, original = o.material](scene_t& s, const std::string& self) {
                object_t *o = s.get(self);
                const bool on = o->material != "active glow";
                o->material = on ? "active glow" : original;
                alchemy_ui.set_status(on ? "The sphere hums and lights up." : "The sphere goes dark.");
            };
        }

        // trigger: whatever lands in the can
        trigger_t *can = room::add_can(scene, "can", vec3_t<gl::f32>(3.5f, 0.0f, 2.5f), vec2_t<gl::f32>(0.45f, 0.45f), 0.9f);
        can->on_enter = [this](scene_t&, const std::string&, const std::string& object) {
            ++cans_filled;
            alchemy_ui.set_status(std::format("{} fell into the can ({} so far).", object, cans_filled));
        };
        can->on_exit = [this](scene_t&, const std::string&, const std::string& object) {
            --cans_filled;
            alchemy_ui.set_status(std::format("{} left the can.", object));
        };

        // collision: a gong on the back wall, hit it with something
        scene.add("gong", {
            .mesh = "cube", .material = "gold metal", .pos = {0.0f, 2.2f, -ROOM_Z + 0.07f}, .scale = {0.8f, 0.8f, 0.06f},
            .body = body_kind_t::fixed,
        });
        scene.get("gong")->on_collision = [this](scene_t&, const std::string&, const std::string& other, gl::f32 impulse) {
            if (impulse < 0.5f) return; // a touch is not a hit
            alchemy_ui.set_status(std::format("DONG! {} hit the gong.", other));
        };
    }

    void input()
    {
        gl::input_t& in = ctx.input;

        // before the capture click below, so that click doesn't also grab
        if (in.mouse_captured) drag.input(in, (int)player.body);

        // mouse look: click to capture, Esc (pause) to release
        if (!in.mouse_captured && in.mouse_hit(gl::mouse_left)) in.set_mouse_capture(true);

        player.input(in, ctx.last_frame, !drag.rotating);

        if (in.key_hit(gl::kb_key_t::f1)) hud_visible = !hud_visible;

        // E: use what is under the crosshair
        const pick_t look = scene.pick(scene.cam.pos, scene.cam.forward(), INTERACT_REACH, (int)player.body);
        interact_prompt.clear();
        if (!drag.holding() && !look.name.empty()) {
            const object_t *o = scene.get(look.name);
            if (o != nullptr && o->on_interact) interact_prompt = o->prompt.empty() ? "Use" : o->prompt;
        }
        if (in.key_hit(gl::kb_key_t::e) && !interact_prompt.empty()) {
            scene.interact(scene.cam.pos, scene.cam.forward(), INTERACT_REACH, (int)player.body);
        }

        // light 0
        const gl::f32 k = 1.5f * ctx.last_frame;
        if (in.key_held(gl::kb_key_t::left))   scene.lamp.angle  -= k;
        if (in.key_held(gl::kb_key_t::right))  scene.lamp.angle  += k;
        if (in.key_held(gl::kb_key_t::up))     scene.lamp.height += k;
        if (in.key_held(gl::kb_key_t::down))   scene.lamp.height -= k;
        if (in.key_held(gl::kb_key_t::period)) scene.lamp.radius += k;
        if (in.key_held(gl::kb_key_t::comma))  scene.lamp.radius -= k;
        scene.lamp.radius = std::max(scene.lamp.radius, 0.1f);

        if (in.key_hit(gl::kb_key_t::key_1)) scene.shading = shading_t::normals;
        if (in.key_hit(gl::kb_key_t::key_2)) scene.shading = shading_t::gouraud;
        if (in.key_hit(gl::kb_key_t::key_3)) scene.shading = shading_t::phong;

        if (in.key_hit(gl::kb_key_t::m)) {
            model_material = (model_material + 1) % room::prop_materials.size();
            if (object_t *o = scene.get("teapot")) o->material = room::prop_materials[model_material];
        }

        // HUD text size
        if (in.key_held(gl::kb_key_t::x) && hud_text_size > 0.0f) hud_text_size -= 1.0f;
        if (in.key_held(gl::kb_key_t::c)) hud_text_size += 1.0f;
    }

    std::string hud_text() const
    {
        const char *shading_name[] = {"normals as color", "Gouraud", "Phong"};
        const vec3_t<gl::f32>& pos = scene.cam.pos;
        return std::format(
            "FPS: {:.0f}   {}x{}\n"
            "Camera: {}   pos: {:.1f} {:.1f} {:.1f}\n"
            "Shading: {}   Model material: {}",
            ctx.actual_fps, ctx.screen_size.x, ctx.screen_size.y,
            player.mode == cam_mode_t::fps ? "first-person" : "free-fly",
            pos.x, pos.y, pos.z,
            shading_name[(int)scene.shading], room::prop_materials[model_material]
        );
    }

    // top-right, the top-left corner belongs to the UI
    void draw_hud()
    {
        const gl::f32 margin = 10.0f;
        const std::string str = hud_text();
        const gl::f32 width = text.measure(str, hud_text_size).x;
        const vec2_t<gl::f32> pos(ctx.screen_size.x - margin - width, ctx.screen_size.y - margin);

        ctx.begin_overlay();
        // drop shadow keeps the text readable on any background
        text.draw(str, pos + vec2_t<gl::f32>(1.5f, -1.5f), hud_text_size, color_t(0.0f, 0.0f, 0.0f, 1.0f));
        text.draw(str, pos, hud_text_size, color_t(1.0f, 1.0f, 1.0f, 1.0f));
        ctx.end_overlay();
    }

    void draw_crosshair()
    {
        const gl::f32 cx = gui.width / 2.0f, cy = gui.height / 2.0f;
        const gl::f32 s  = gui.scale;
        const color_t color = drag.holding()            ? color_t(0.35f, 0.85f, 0.55f, 1.0f) :
                              !interact_prompt.empty()  ? color_t(0.45f, 0.80f, 1.00f, 1.0f) :
                              !drag.hover.empty()       ? color_t(0.84f, 0.64f, 0.35f, 1.0f) :
                                                          color_t(1.0f, 1.0f, 1.0f, 1.0f);

        ctx.begin_overlay();
        gui.fill({cx - 8.0f * s, cy - 1.0f * s, 16.0f * s, 2.0f * s}, color);
        gui.fill({cx - 1.0f * s, cy - 8.0f * s, 2.0f * s, 16.0f * s}, color);
        if (!interact_prompt.empty()) {
            const std::string str = "[E] " + interact_prompt;
            const gl::f32 size = 18.0f * s;
            gui.label(str, cx - text.measure(str, size).x / 2.0f, cy + 22.0f * s, size, color);
        }
        ctx.end_overlay();
    }

    bool frame()
    {
        ctx.start_loop();
        ctx.on_resize();

        alchemy_ui.update();
        if (alchemy_ui.game_interactive()) input();
        else { drag.release(); interact_prompt.clear(); }

        drag.update(ctx.input, ctx.last_frame);
        scene.update(ctx.last_frame);
        player.update();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const gl::f32 aspect = (gl::f32)ctx.screen_size.x / (gl::f32)std::max(ctx.screen_size.y, 1);
        if (!scene.draw(aspect)) return false;

        if (hud_visible) draw_hud();
        if (alchemy_ui.game_interactive() && ctx.input.mouse_captured) draw_crosshair();
        alchemy_ui.draw();

        ctx.end_loop();

        return true;
    }
};

int main(int argc, char **argv)
{
    const char *model_path = "./assets/models/teapot.smf";
    if (argc >= 2) model_path = argv[1];

    gl::gl_first_state_verify();

    gl::glfw_ctx_t glfw_ctx;
    if (!glfw_ctx.init()) return 1;

    gl::ctx_t ctx(glfw_ctx);
    if (!ctx.create_window(1200, 900, "Alchemy Room", nullptr, nullptr)) return 1;
    ctx.set_fps(60.0);

    // declared after ctx: GL objects die before the GLFW context
    app_t app(ctx);
    if (!app.init(model_path)) return 1;

    ctx.before_loop();
    while (!ctx.window_should_close()) {
        if (!app.frame()) return 1;
    }

    return 0;
}
