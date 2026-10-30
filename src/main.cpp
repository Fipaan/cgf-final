#include <algorithm>
#include <numbers>
#include <optional>
#include <span>
#include <string>
#include <type_traits>
#include <vector>
#include <cstdint>
#include <map>

#include "mesh.hpp"
#include "gl_ctx.hpp"
#include "basic.hpp"
#include "math3d.hpp"
#include "ft.hpp"
#include "alchemy_ui.hpp"

struct material_t {
    color_t ambient, diffuse, specular;
    GLfloat shininess;
};

constexpr color_t light_ambient (0.2f, 0.2f, 0.2f, 1.0f);
constexpr color_t light_diffuse (0.6f, 0.6f, 0.6f, 1.0f);
constexpr color_t light_specular(1.0f, 1.0f, 1.0f, 1.0f);

// light 1 lives in camera space, slightly off the eye point
constexpr vec3_t<GLfloat> headlight_eye(0.3f, 0.3f, 0.0f);

constexpr GLfloat FOVY = 45.0f * std::numbers::pi_v<GLfloat> / 180.0f;

enum class shading_t : int { normals = 0, gouraud = 1, phong = 2 };

struct app_t {
    gl::glfw_ctx_t& glfw_ctx;
    gl::ctx_t&      ctx;

    ft::ft_lib_t  ft_lib;
    ft::ft_face_t ft_face;
    gl::shader_t  text_shader;
    alchemy::ui_t alchemy_ui;

    mesh_t       mesh;
    mesh_gpu_t   gpu;
    gl::shader_t shader;

    using material_map_t = std::map<std::string, material_t>;
    material_map_t materials;
    material_map_t::iterator material_it;

    bool add_material(std::string name, material_t material)
    {
        auto [_it, inserted] = materials.emplace(name, material);
        return inserted;
    }

    cyl_t cam  {0.0f, 3.0f, 1.0f};
    cyl_t lamp {0.8f, 3.0f, 2.0f}; // light 0, object coordinates
    bool  z_up        = false;
    bool  perspective = true;
    bool  auto_orbit  = false;
    shading_t shading = shading_t::normals;

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
        if (!gpu.upload(mesh)) return false;

        // from the assignment
        add_material("red plastic", material_t{
            .ambient   = color_t(0.6f, 0.2f, 0.2f, 1.0f),
            .diffuse   = color_t(0.9f, 0.1f, 0.1f, 1.0f),
            .specular  = color_t(0.8f, 0.8f, 0.8f, 1.0f),
            .shininess = 80.0f,
        });
        // tinted specular, broad highlight
        add_material("gold metal", material_t{
            .ambient   = color_t(0.25f, 0.20f, 0.07f, 1.0f),
            .diffuse   = color_t(0.75f, 0.61f, 0.23f, 1.0f),
            .specular  = color_t(0.63f, 0.56f, 0.37f, 1.0f),
            .shininess = 51.0f,
        });
        // almost no specular
        add_material("matte clay", material_t{
            .ambient   = color_t(0.25f, 0.20f, 0.15f, 1.0f),
            .diffuse   = color_t(0.65f, 0.50f, 0.35f, 1.0f),
            .specular  = color_t(0.05f, 0.05f, 0.05f, 1.0f),
            .shininess = 4.0f,
        });
        // tiny, bright white highlight
        add_material("glossy blue ceramic", material_t{
            .ambient   = color_t(0.05f, 0.10f, 0.30f, 1.0f),
            .diffuse   = color_t(0.10f, 0.25f, 0.85f, 1.0f),
            .specular  = color_t(1.00f, 1.00f, 1.00f, 1.0f),
            .shininess = 200.0f,
        });
        material_it = materials.begin();

        if (!ft_lib.init()) return false;
        FT_Long face_index = 0;
        if (!ft_face.init("./assets/fonts/iosevka-regular.ttf", face_index)) return false;
        if (!ft_face.set_pixel_sizes((vec2_t(100)/ctx.DPI).cast<FT_UInt>())) return false;
        if (!text_shader.from_files("./shaders/text.vert", "./shaders/text.frag")) return false;

        glEnable(GL_DEPTH_TEST);
        ctx.clear_color(0x181818FF);
        return true;
    }

    void input(GLfloat dt)
    {
        const GLfloat k = 1.5f * dt;

        if (held(gl::kb_key_t::left))  cam.angle  -= k;
        if (held(gl::kb_key_t::right)) cam.angle  += k;
        if (held(gl::kb_key_t::up))    cam.height += k;
        if (held(gl::kb_key_t::down))  cam.height -= k;
        if (held(gl::kb_key_t::minus)) cam.radius += k;
        if (held(gl::kb_key_t::equal)) cam.radius -= k;
        cam.radius = std::max(cam.radius, 0.5f);

        if (held(gl::kb_key_t::a)) lamp.angle  -= k;
        if (held(gl::kb_key_t::d)) lamp.angle  += k;
        if (held(gl::kb_key_t::w)) lamp.height += k;
        if (held(gl::kb_key_t::s)) lamp.height -= k;
        if (held(gl::kb_key_t::q)) lamp.radius += k;
        if (held(gl::kb_key_t::e)) lamp.radius -= k;
        lamp.radius = std::max(lamp.radius, 0.1f);

        if (hit(gl::kb_key_t::space)) auto_orbit = !auto_orbit;
        if (auto_orbit) cam.angle += 0.5f * dt;

        if (hit(gl::kb_key_t::key_1)) { shading = shading_t::normals; infofn("Shading: normals as color"); }
        if (hit(gl::kb_key_t::key_2)) { shading = shading_t::gouraud; infofn("Shading: Gouraud"); }
        if (hit(gl::kb_key_t::key_3)) { shading = shading_t::phong;   infofn("Shading: Phong"); }

        if (hit(gl::kb_key_t::m)) {
            if (++material_it == materials.end()) material_it = materials.begin();
            infofn("Material: {}", material_it->first);
        }
        if (hit(gl::kb_key_t::o)) { perspective = false; infofn("Projection: parallel"); }
        if (hit(gl::kb_key_t::p)) { perspective = true;  infofn("Projection: perspective"); }
        if (hit(gl::kb_key_t::z)) { z_up = !z_up;        infofn("Cylinder axis: {}", z_up ? "Z" : "Y"); }

        if (hit(gl::kb_key_t::escape)) ctx.force_close();
    }

    void set_vec4(const char *name, const color_t& c)
    {
        shader.uniform(name, c.r, c.g, c.b, c.a);
    }

    void frame()
    {
        ctx.start_loop();
        ctx.on_resize();

        input(ctx.last_frame);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // matrices
        const mat4_t model = mat4_t::identity(); // mesh is already centred / unit sized
        const vec3_t<GLfloat> up = z_up ? vec3_t<GLfloat>(0, 0, 1) : vec3_t<GLfloat>(0, 1, 0);
        const vec3_t<GLfloat> cam_cart = z_up ? cam.to_cart_z_up() : cam.to_cart_y_up();
        const mat4_t view = mat4_t::look_at(cam_cart, vec3_t<GLfloat>(), up);

        const GLfloat aspect = (GLfloat)ctx.screen_size.x / (GLfloat)std::max(ctx.screen_size.y, 1);
        mat4_t proj;
        if (perspective) {
            proj = mat4_t::perspective(FOVY, aspect, 0.1f, 100.0f);
        } else {
            // same apparent size as perspective at the orbit radius
            GLfloat h = cam.radius * std::tan(FOVY / 2.0f);
            proj = mat4_t::ortho(-h * aspect, h * aspect, -h, h, 0.1f, 100.0f);
        }

        // lights in eye space
        const vec3_t<GLfloat> lamp_cart = z_up ? lamp.to_cart_z_up() : lamp.to_cart_y_up();
        const vec3_t<GLfloat> l0_eye = (view * model).transform_point(lamp_cart);
        const vec3_t<GLfloat> l1_eye = headlight_eye;

        const material_t& material = material_it->second;

        shader.use();
        shader.uniform(gl::m_type_t::m4x4, "model", 1, gl::gl_bool::FALSE, model.m);
        shader.uniform(gl::m_type_t::m4x4, "view",  1, gl::gl_bool::FALSE, view.m);
        shader.uniform(gl::m_type_t::m4x4, "proj",  1, gl::gl_bool::FALSE, proj.m);
        shader.uniform("light0_pos", l0_eye.x, l0_eye.y, l0_eye.z);
        shader.uniform("light1_pos", l1_eye.x, l1_eye.y, l1_eye.z);
        set_vec4("light_ambient",  light_ambient);
        set_vec4("light_diffuse",  light_diffuse);
        set_vec4("light_specular", light_specular);
        set_vec4("material_ambient",  material.ambient);
        set_vec4("material_diffuse",  material.diffuse);
        set_vec4("material_specular", material.specular);
        shader.uniform("material_shininess", material.shininess);
        shader.uniform("shading_mode", (int)shading);

        gpu.draw();

        vec2_t<GLfloat> scale = 2.0f / ctx.screen_size;
        float text_proj[16] = {
            scale.x,       0,  0, 0,
                  0, scale.y,  0, 0,
                  0,       0, -1, 0,
                 -1,      -1,  0, 1,
        };
        text_shader.use();
        text_shader.uniform(gl::m_type_t::m4x4, "proj",  1, gl::gl_bool::FALSE, text_proj);
        text_shader.uniform("color", 0.0f, 1.0f, 0.0f);
        text_shader.uniform("glyph", (int)0);

        static float text_size = 100.0f;
        if (held(gl::kb_key_t::z)) text_size -= 1.0f;
        if (held(gl::kb_key_t::x)) text_size += 1.0f;

        std::string test_text = "Test\ntext";
        vec2_t<FT_UInt> test_text_size = ft_face.measure_text(test_text, text_size);
        vec2_t<GLfloat> margin(100.0f);
        vec2_t<GLfloat> test_text_pos = vec2_t<GLfloat>(
            ctx.screen_size.x - (test_text_size.x + margin.x),
            test_text_size.y + margin.y
        );
        ctx.draw_text(ft_face, test_text, test_text_pos, text_size);

        alchemy_ui.draw(ctx, ft_face, text_shader);

        ctx.end_loop();
    }
};

int main(int argc, char **argv)
{
    if (argc < 2) {
        errorfn("usage: {} model.smf", argv[0]);
        return 1;
    }

    gl::gl_first_state_verify();

    gl::glfw_ctx_t glfw_ctx;
    if (!glfw_ctx.init()) return 1;

    gl::ctx_t ctx(glfw_ctx);
    if (!ctx.create_window(1200, 900, "SMF viewer", nullptr, nullptr)) return 1;
    ctx.set_fps(60.0);

    // declared after ctx: GL objects die before the GLFW context
    app_t app(glfw_ctx, ctx);
    if (!app.init(argv[1])) return 1;

    infofn("1/2/3 shading | M material | O/P projection | Z axis | Space auto-orbit");
    infofn("camera: arrows + -/= | light 0: A/D W/S Q/E");

    ctx.before_loop();
    while (!ctx.window_should_close()) app.frame();

    return 0;
}
