#include <algorithm>
#include <string>
#include <vector>

#include <basic.hpp>
#include <gl/first.hpp>
#include <math/3d.hpp>
#include <gl/mesh.hpp>
#include <gl/ctx.hpp>
#include <ft.hpp>
#include <ui/alchemy.hpp>
#include <camera.hpp>
#include <scene.hpp>

constexpr color_t light_ambient (0.2f, 0.2f, 0.2f, 1.0f);
constexpr color_t light_diffuse (0.6f, 0.6f, 0.6f, 1.0f);
constexpr color_t light_specular(1.0f, 1.0f, 1.0f, 1.0f);

// light 1 lives in camera space, slightly off the eye point
constexpr vec3_t<GLfloat> headlight_eye(0.3f, 0.3f, 0.0f);

enum class shading_t : int { normals = 0, gouraud = 1, phong = 2 };
enum class cam_mode_t { fly, fps };

// player collider (AABB), eye sits `EYE_HEIGHT` above its centre
constexpr vec3_t<GLfloat> PLAYER_HALF(0.3f, 0.9f, 0.3f);
constexpr GLfloat EYE_HEIGHT  = 0.7f;
constexpr GLfloat WALK_SPEED  = 5.0f;
constexpr GLfloat FLY_SPEED   = 10.0f;
constexpr GLfloat SPRINT_MULT = 2.0f;
constexpr GLfloat JUMP_SPEED  = 5.5f;

// room interior: x in [-ROOM_X, ROOM_X], z in [-ROOM_Z, ROOM_Z], y in [0, ROOM_H]
constexpr GLfloat ROOM_X = 8.0f;
constexpr GLfloat ROOM_Z = 6.0f;
constexpr GLfloat ROOM_H = 4.0f;
constexpr GLfloat WALL_T = 0.25f; // half thickness

// Cube shelf unit: a grid of open cells made of boards, built from `scene_t` objects.
struct shelf_t {
    vec3_t<GLfloat> origin; // centre of the back side, on the floor
    vec3_t<GLfloat> facing; // unit axis the open side looks at, (±1,0,0) or (0,0,±1)
    int     cols = 1, rows = 1;
    GLfloat cell  = 0.9f;   // inner cell size
    GLfloat board = 0.06f;  // board thickness
    GLfloat depth = 0.45f;

    GLfloat width()  const { return cols * cell + (cols + 1) * board; }
    GLfloat height() const { return rows * cell + (rows + 1) * board; }

    // local (x right, y up, z out of the back) -> world
    vec3_t<GLfloat> to_world(GLfloat lx, GLfloat ly, GLfloat lz) const
    {
        vec3_t<GLfloat> right(facing.z, 0.0f, -facing.x);
        return origin + right * lx + vec3_t<GLfloat>(0.0f, ly, 0.0f) + facing * lz;
    }
    // local half extents -> world half extents
    vec3_t<GLfloat> to_world_half(GLfloat hx, GLfloat hy, GLfloat hz) const
    {
        return std::fabs(facing.x) > 0.5f ? vec3_t<GLfloat>(hz, hy, hx) : vec3_t<GLfloat>(hx, hy, hz);
    }
    // middle of the floor of a cell
    vec3_t<GLfloat> cell_floor(int col, int row) const
    {
        GLfloat lx = -width() / 2.0f + board + cell / 2.0f + col * (cell + board);
        GLfloat ly = board + row * (cell + board);
        return to_world(lx, ly, depth / 2.0f);
    }
};

struct app_t {
    gl::glfw_ctx_t& glfw_ctx;
    gl::ctx_t&      ctx;
    alchemy::ui_t   alchemy_ui;

    ft::ft_lib_t  ft_lib;
    ft::ft_face_t ft_face;
    gl::shader_t  text_shader;

    FT_UInt       text_pixel_sizes;
    FT_UInt       active_text_size;

    bool update_font_cache()
    {
        return ctx.set_pixel_sizes(ft_face, vec2_t(text_pixel_sizes));
    }

    mesh_t       mesh; // loaded model
    gl::shader_t shader;

    scene_t    scene;
    size_t     player_body = 0;
    camera_t   cam;
    cam_mode_t cam_mode = cam_mode_t::fps;

    // `M` cycles the loaded model through these
    std::vector<std::string> model_materials = {"red plastic", "gold metal", "matte clay", "glossy blue ceramic"};
    size_t model_material = 0;

    cyl_t lamp {0.8f, 3.5f, 3.0f}; // light 0, world coordinates, Y up
    shading_t shading = shading_t::phong;
    bool hud_visible  = true;

    bool prev_key[GLFW_KEY_LAST + 1] = {};

    app_t(gl::glfw_ctx_t& g, gl::ctx_t& c) :
        glfw_ctx(g),
        ctx(c),
        ft_face(ft::ft_face_t(ft_lib))
    {}

    bool held(gl::kb_key_t k)
    {
        return ctx.get_key(k) == gl::kb_key_state_t::press;
    }
    // edge-triggered, call once per frame per key
    bool hit(gl::kb_key_t k)
    {
        bool now = held(k);
        bool r   = now && !prev_key[k];
        prev_key[k] = now;
        return r;
    }

    bool init(const char *smf_path)
    {
        if (!mesh.load_smf(smf_path)) return false;
        mesh.normalize();
        mesh.compute_normals();

        if (!shader.from_files("shaders/mesh.vert", "shaders/mesh.frag")) return false;

        // meshes
        if (!scene.add_mesh("model",  mesh))               return false;
        if (!scene.add_mesh("cube",   mesh_t::cube()))     return false;
        if (!scene.add_mesh("sphere", mesh_t::sphere()))   return false;
        if (!scene.add_mesh("plane",  mesh_t::plane()))    return false;

        add_materials();
        build_room();
        build_shelves();
        build_props();
        build_player();

        if (!ft_lib.init()) return false;
        FT_Long face_index = 0;
        if (!ft_face.init("./assets/fonts/iosevka-regular.ttf", face_index)) return false;
        active_text_size = text_pixel_sizes = 20;
        if (!update_font_cache()) return false;
        if (!text_shader.from_files("./shaders/text.vert", "./shaders/text.frag")) return false;
        if (!alchemy_ui.init()) return false;
        ctx.set_vsync(true);

        glEnable(GL_DEPTH_TEST);
        ctx.clear_color(0x181818FF);
        return true;
    }

    void add_materials()
    {
        // from the assignment
        scene.add_material("red plastic", material_t{
            .ambient   = color_t(0.6f, 0.2f, 0.2f, 1.0f),
            .diffuse   = color_t(0.9f, 0.1f, 0.1f, 1.0f),
            .specular  = color_t(0.8f, 0.8f, 0.8f, 1.0f),
            .shininess = 80.0f,
        });
        // tinted specular, broad highlight
        scene.add_material("gold metal", material_t{
            .ambient   = color_t(0.25f, 0.20f, 0.07f, 1.0f),
            .diffuse   = color_t(0.75f, 0.61f, 0.23f, 1.0f),
            .specular  = color_t(0.63f, 0.56f, 0.37f, 1.0f),
            .shininess = 51.0f,
        });
        // almost no specular
        scene.add_material("matte clay", material_t{
            .ambient   = color_t(0.25f, 0.20f, 0.15f, 1.0f),
            .diffuse   = color_t(0.65f, 0.50f, 0.35f, 1.0f),
            .specular  = color_t(0.05f, 0.05f, 0.05f, 1.0f),
            .shininess = 4.0f,
        });
        // tiny, bright white highlight
        scene.add_material("glossy blue ceramic", material_t{
            .ambient   = color_t(0.05f, 0.10f, 0.30f, 1.0f),
            .diffuse   = color_t(0.10f, 0.25f, 0.85f, 1.0f),
            .specular  = color_t(1.00f, 1.00f, 1.00f, 1.0f),
            .shininess = 200.0f,
        });

        // room
        scene.add_material("wall", material_t{
            .ambient   = color_t(0.30f, 0.29f, 0.26f, 1.0f),
            .diffuse   = color_t(0.72f, 0.70f, 0.64f, 1.0f),
            .specular  = color_t(0.02f, 0.02f, 0.02f, 1.0f),
            .shininess = 4.0f,
        });
        scene.add_material("floor", material_t{
            .ambient   = color_t(0.18f, 0.12f, 0.07f, 1.0f),
            .diffuse   = color_t(0.45f, 0.30f, 0.17f, 1.0f),
            .specular  = color_t(0.15f, 0.15f, 0.15f, 1.0f),
            .shininess = 20.0f,
        });
        scene.add_material("wood", material_t{
            .ambient   = color_t(0.22f, 0.14f, 0.07f, 1.0f),
            .diffuse   = color_t(0.62f, 0.40f, 0.20f, 1.0f),
            .specular  = color_t(0.10f, 0.10f, 0.10f, 1.0f),
            .shininess = 10.0f,
        });
        scene.add_material("plywood", material_t{
            .ambient   = color_t(0.20f, 0.17f, 0.12f, 1.0f),
            .diffuse   = color_t(0.50f, 0.42f, 0.30f, 1.0f),
            .specular  = color_t(0.03f, 0.03f, 0.03f, 1.0f),
            .shininess = 4.0f,
        });
    }

    // static box, `half` is both the cube scale and the collider
    void add_box(const std::string& name, const char *material, vec3_t<GLfloat> pos, vec3_t<GLfloat> half)
    {
        scene.add(name, {
            .mesh = "cube", .material = material, .pos = pos, .scale = half,
            .body = body_kind_t::fixed,
        });
    }

    void build_room()
    {
        const GLfloat cy = ROOM_H / 2.0f;
        const GLfloat hx = ROOM_X + WALL_T, hz = ROOM_Z + WALL_T;
        const GLfloat hy = ROOM_H / 2.0f + WALL_T;

        add_box("room/floor",   "floor", {0, -WALL_T, 0},           {hx, WALL_T, hz});
        add_box("room/ceiling", "wall",  {0, ROOM_H + WALL_T, 0},   {hx, WALL_T, hz});
        add_box("room/wall_n",  "wall",  {0, cy, -ROOM_Z - WALL_T}, {hx, hy, WALL_T});
        add_box("room/wall_s",  "wall",  {0, cy,  ROOM_Z + WALL_T}, {hx, hy, WALL_T});
        add_box("room/wall_w",  "wall",  {-ROOM_X - WALL_T, cy, 0}, {WALL_T, hy, hz});
        add_box("room/wall_e",  "wall",  { ROOM_X + WALL_T, cy, 0}, {WALL_T, hy, hz});
    }

    shelf_t add_shelf(const std::string& name, vec3_t<GLfloat> origin, vec3_t<GLfloat> facing, int cols, int rows)
    {
        shelf_t s{.origin = origin, .facing = facing, .cols = cols, .rows = rows};
        const GLfloat W = s.width(), H = s.height();
        const GLfloat hb = s.board / 2.0f, hd = s.depth / 2.0f;

        for (int i = 0; i <= rows; ++i) {
            GLfloat ly = hb + i * (s.cell + s.board);
            add_box(std::format("{}/board{}", name, i), "wood", s.to_world(0, ly, hd), s.to_world_half(W / 2, hb, hd));
        }
        for (int j = 0; j <= cols; ++j) {
            GLfloat lx = -W / 2 + hb + j * (s.cell + s.board);
            add_box(std::format("{}/side{}", name, j), "wood", s.to_world(lx, H / 2, hd), s.to_world_half(hb, H / 2, hd));
        }
        add_box(name + "/back", "plywood", s.to_world(0, H / 2, 0.01f), s.to_world_half(W / 2, H / 2, 0.01f));
        return s;
    }

    // a few small objects in the cells, they are dynamic and settle on the boards
    void fill_shelf(const std::string& name, const shelf_t& s)
    {
        static const char *mats[] = {"red plastic", "gold metal", "matte clay", "glossy blue ceramic"};
        const GLfloat r = 0.2f;

        for (int row = 0; row < s.rows; ++row) {
            for (int col = 0; col < s.cols; ++col) {
                int k = col + row * s.cols;
                if (k % 3 == 2) continue; // leave some cells empty

                vec3_t<GLfloat> pos = s.cell_floor(col, row) + vec3_t<GLfloat>(0.0f, r + 0.01f, 0.0f);
                scene.add(std::format("{}/item{}_{}", name, col, row), {
                    .mesh     = k % 3 == 0 ? "cube" : "sphere",
                    .material = mats[k % 4],
                    .pos      = pos,
                    .scale    = vec3_t<GLfloat>(r),
                    .body     = body_kind_t::dynamic,
                });
            }
        }
    }

    void build_shelves()
    {
        struct spec_t { const char *name; vec3_t<GLfloat> origin, facing; int cols, rows; };
        const spec_t specs[] = {
            {"shelf_a", {-5.5f, 0.0f, -ROOM_Z}, { 0, 0,  1}, 3, 3}, // back wall
            {"shelf_b", { 5.0f, 0.0f, -ROOM_Z}, { 0, 0,  1}, 4, 3}, // back wall
            {"shelf_c", {-ROOM_X, 0.0f,  0.0f}, { 1, 0,  0}, 5, 2}, // west wall
            {"shelf_d", { ROOM_X, 0.0f,  1.0f}, {-1, 0,  0}, 4, 4}, // east wall
        };
        for (const spec_t& sp : specs) {
            shelf_t s = add_shelf(sp.name, sp.origin, sp.facing, sp.cols, sp.rows);
            fill_shelf(sp.name, s);
        }
    }

    void build_props()
    {
        // the loaded model, collider from its bounds
        scene.add("teapot", {
            .mesh = "model", .material = model_materials[0],
            .pos = {0, 2.5f, 0}, .scale = vec3_t<GLfloat>(1.0f),
            .body = body_kind_t::dynamic,
        });
    }

    void build_player()
    {
        player_body = scene.world.add(phys::body_t{
            .pos = {0, PLAYER_HALF.y + 0.01f, 4}, .half = PLAYER_HALF,
            .friction = 0.0f, // velocity is set directly by input
        });
        cam.pos = scene.world.bodies[player_body].pos + vec3_t<GLfloat>(0, EYE_HEIGHT, 0);
    }

    void set_cam_mode(cam_mode_t m)
    {
        phys::body_t& p = scene.world.bodies[player_body];
        if (m == cam_mode_t::fps) {
            p.pos     = cam.pos - vec3_t<GLfloat>(0, EYE_HEIGHT, 0);
            p.vel     = vec3_t<GLfloat>();
            p.enabled = true;
        } else {
            p.enabled = false;
        }
        cam_mode = m;
    }

    void input(GLfloat dt)
    {
        // mouse look: click to capture, Esc to release
        if (!ctx.mouse_captured && ctx.get_mouse_button(gl::mouse_left) == gl::kb_key_state_t::press) {
            ctx.set_mouse_capture(true);
        }
        if (ctx.mouse_captured) {
            vec2_t<GLfloat> d = ctx.mouse_delta();
            cam.look(d.x, d.y);
        }

        if (hit(gl::kb_key_t::tab)) {
            set_cam_mode(cam_mode == cam_mode_t::fps ? cam_mode_t::fly : cam_mode_t::fps);
        }
        if (hit(gl::kb_key_t::f1)) hud_visible = !hud_visible;

        // movement
        const bool fps = cam_mode == cam_mode_t::fps;
        const vec3_t<GLfloat> up(0, 1, 0);
        const vec3_t<GLfloat> fwd = fps ? cam.flat_forward() : cam.forward();
        const vec3_t<GLfloat> right = cam.right();

        vec3_t<GLfloat> wish;
        if (held(gl::kb_key_t::w)) wish += fwd;
        if (held(gl::kb_key_t::s)) wish -= fwd;
        if (held(gl::kb_key_t::d)) wish += right;
        if (held(gl::kb_key_t::a)) wish -= right;
        if (!fps) {
            if (held(gl::kb_key_t::space))        wish += up;
            if (held(gl::kb_key_t::left_control)) wish -= up;
        }
        if (vec3::dot(wish, wish) > 1e-6f) wish = vec3::normalize(wish);

        GLfloat speed = fps ? WALK_SPEED : FLY_SPEED;
        if (held(gl::kb_key_t::left_shift)) speed *= SPRINT_MULT;

        if (fps) {
            phys::body_t& p = scene.world.bodies[player_body];
            p.vel.x = wish.x * speed;
            p.vel.z = wish.z * speed;
            if (held(gl::kb_key_t::space) && p.on_ground) p.vel.y = JUMP_SPEED;
        } else {
            cam.pos += wish * (speed * dt);
        }

        // light 0
        const GLfloat k = 1.5f * dt;
        if (held(gl::kb_key_t::left))   lamp.angle  -= k;
        if (held(gl::kb_key_t::right))  lamp.angle  += k;
        if (held(gl::kb_key_t::up))     lamp.height += k;
        if (held(gl::kb_key_t::down))   lamp.height -= k;
        if (held(gl::kb_key_t::period)) lamp.radius += k;
        if (held(gl::kb_key_t::comma))  lamp.radius -= k;
        lamp.radius = std::max(lamp.radius, 0.1f);

        if (hit(gl::kb_key_t::key_1)) shading = shading_t::normals;
        if (hit(gl::kb_key_t::key_2)) shading = shading_t::gouraud;
        if (hit(gl::kb_key_t::key_3)) shading = shading_t::phong;

        if (hit(gl::kb_key_t::m)) {
            model_material = (model_material + 1) % model_materials.size();
            if (object_t *o = scene.get("teapot")) o->material = model_materials[model_material];
        }
    }

    std::string hud_text() const
    {
        const char *shading_name[] = {"normals as color", "Gouraud", "Phong"};
        std::string t = std::format(
            "FPS: {:.0f}   {}x{}\n"
            "Camera: {}   pos: {:.1f} {:.1f} {:.1f}\n"
            "Shading: {}   Model material: {}\n"
            "\n"
            "Click: capture mouse   Esc: release / quit   F1: hide this\n"
            "Tab: fly / first-person   WASD: move   Shift: sprint\n"
            "Space: jump (fps) / up (fly)   Ctrl: down (fly)\n"
            "1/2/3: shading   M: material   Light 0: arrows , .",
            ctx.actual_fps, ctx.screen_size.x, ctx.screen_size.y,
            cam_mode == cam_mode_t::fps ? "first-person" : "free-fly",
            cam.pos.x, cam.pos.y, cam.pos.z,
            shading_name[(int)shading], model_materials[model_material]
        );
        return t;
    }

    bool frame()
    {
        ctx.start_loop();
        ctx.on_resize();

        alchemy_ui.update(ctx);
        if (alchemy_ui.game_interactive()) input(ctx.last_frame);

        scene.update(ctx.last_frame);
        if (cam_mode == cam_mode_t::fps) {
            cam.pos = scene.world.bodies[player_body].pos + vec3_t<GLfloat>(0, EYE_HEIGHT, 0);
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // matrices
        const mat4_t view = cam.view();
        const GLfloat aspect = (GLfloat)ctx.screen_size.x / (GLfloat)std::max(ctx.screen_size.y, 1);
        const mat4_t proj = cam.proj(aspect);

        // lights in eye space
        const vec3_t<GLfloat> lamp_cart = lamp.to_cart_y_up();
        const vec3_t<GLfloat> l0_eye = view.transform_point(lamp_cart);
        const vec3_t<GLfloat> l1_eye = headlight_eye;

        shader.use();
        shader.uniform(gl::m_type_t::m4x4, "view",  1, gl::gl_bool::FALSE, view.m);
        shader.uniform(gl::m_type_t::m4x4, "proj",  1, gl::gl_bool::FALSE, proj.m);
        shader.uniform("light0_pos", l0_eye.x, l0_eye.y, l0_eye.z);
        shader.uniform("light1_pos", l1_eye.x, l1_eye.y, l1_eye.z);
        shader.uniform("light_ambient",  light_ambient.r,  light_ambient.g,  light_ambient.b,  light_ambient.a);
        shader.uniform("light_diffuse",  light_diffuse.r,  light_diffuse.g,  light_diffuse.b,  light_diffuse.a);
        shader.uniform("light_specular", light_specular.r, light_specular.g, light_specular.b, light_specular.a);
        shader.uniform("shading_mode", (int)shading);

        if (!scene.draw(shader)) return false;

        vec2_t<GLfloat> scale = 2.0f / ctx.screen_size;
        float text_proj[16] = {
            scale.x,       0,  0, 0,
                  0, scale.y,  0, 0,
                  0,       0, -1, 0,
                 -1,      -1,  0, 1,
        };
        text_shader.use();
        text_shader.uniform(gl::m_type_t::m4x4, "proj",  1, gl::gl_bool::FALSE, text_proj);
        text_shader.uniform("glyph", (int)0);

        if (held(gl::kb_key_t::x) && active_text_size > 0.0f) active_text_size -= 1.0f;
        if (held(gl::kb_key_t::c)) active_text_size += 1.0f;

        if (active_text_size > 10.0f) {
            float delta_scale = active_text_size / text_pixel_sizes - 1.0f;
            // if text is 30% bigger than reference size
            if (delta_scale > 0.3f) {
                text_pixel_sizes = active_text_size;
                if (!update_font_cache()) return false;
            }
        }

        if (hud_visible) {
            const GLfloat margin = 10.0f;
            const std::string text = hud_text();
            const vec2_t<GLfloat> pos(margin, ctx.screen_size.y - margin);
            glDisable(GL_DEPTH_TEST);
            // drop shadow keeps the text readable on any background
            text_shader.uniform("color", 0.0f, 0.0f, 0.0f);
            ctx.draw_text(ft_face, text, pos + vec2_t<GLfloat>(1.5f, -1.5f), active_text_size);
            text_shader.uniform("color", 1.0f, 1.0f, 1.0f);
            ctx.draw_text(ft_face, text, pos, active_text_size);
            glEnable(GL_DEPTH_TEST);
        }

        alchemy_ui.draw(ctx, ft_face, text_shader);

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
    app_t app(glfw_ctx, ctx);
    if (!app.init(model_path)) return 1;

    ctx.before_loop();
    while (!ctx.window_should_close()) {
        if (!app.frame()) return 1;
    }

    return 0;
}
