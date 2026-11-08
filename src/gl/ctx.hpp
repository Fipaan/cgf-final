/*
 * gl/ctx.hpp -
 *     GL/GLFW context and custom library one. It provides a way to manipulate GL/GLFW from in
 *     unified place.
 */
#pragma once

#include <gl/first.hpp>
#include <math/2d.hpp>
#include <gl/const.hpp>
#include <gl/shader.hpp>
#include <gl/2d.hpp>
#include <ft.hpp>

/* Usage:
```cpp
int main()
{
    gl::glfw_ctx_t glfw_ctx;
    if (!glfw_ctx.init()) return 1;
    gl::ctx_t ctx(glfw_ctx);
    if (!ctx.create_window(800, 600, "Hello, OpenGL!")) return 1;

    ctx.set_fps(60.0f);
    ctx.before_loop();
    while (!ctx.window_should_close()) {
        ctx.start_loop();
        if (ctx.on_resize()) {
            // ... handle resizing ...
        }
        // ... main logic ...
        ctx.end_loop();
    }

    return 0;
}
*/
namespace gl {
    struct glfw_ctx_t {
        bool initialized;

        glfw_ctx_t();
        ~glfw_ctx_t();

        bool init();
    };

    struct ctx_t {
        bool initialized;
        glfw_ctx_t& glfw_ctx;
        std::vector<vec2_t<GLfloat>> vertices;
        std::vector<GLfloat> vertices_flat, alphas, colors_flat;

        vao_t vao;
        vbo_t vbo_pos, vbo_alpha, vbo_color, vbo_xyuv;

        enum ctx_vbo_kind_t {
            pos   = 0,
            alpha = 1,
            color = 2,
            xyuv  = 3,
        };

        GLFWwindow *window;
        bool   mouse_captured;
        bool   _mouse_primed;
        double _mouse_x, _mouse_y;
        // NOTE: wscreen_size is size in window units
        vec2_t<int> screen_size_prev, screen_size, wscreen_size;
        vec2_t<GLfloat> DPI;

        GLboolean overlay_depth = GL_FALSE, overlay_cull = GL_FALSE, overlay_blend = GL_FALSE;
        GLfloat    target_fps, actual_fps;
        duration   frame_time;
        time_point _start_frame, _end_frame;
        time_point next_frame;
        GLfloat    last_frame;

        ctx_t(glfw_ctx_t& glfw_ctx);
        ~ctx_t();
        bool create_window(int width, int height, const char *title, GLFWmonitor *monitor = nullptr, GLFWwindow *share = nullptr);
 
        void set_fps(GLfloat fps);
        void before_loop();

        void start_loop();
        bool window_should_close(void);
        // required for proper inner state, returns true if resize happened
        bool on_resize();
        void end_loop();

        void to_ndc(GLfloat& x, GLfloat& y);
        void to_ndc(vec2_t<GLfloat>& v);
        void from_ndc(GLfloat& x, GLfloat& y);
        void from_ndc(vec2_t<GLfloat>& v);

        bool draw_circle(vec2_t<GLfloat> c, GLfloat r, std::size_t num_segments);
        bool draw_circle_grad(vec2_t<GLfloat> c, GLfloat r, GLfloat shift, std::size_t num_segments);
        bool draw_ellipse(vec2_t<GLfloat> c, vec2_t<GLfloat> r, std::size_t num_segments);
        void draw_rect(rect_t rect);
        bool draw_text(ft::ft_face_t& ft_face, std::string text, vec2_t<GLfloat> pos, GLfloat height);

        kb_key_state_t get_key(kb_key_t key);

        kb_key_state_t get_mouse_button(mouse_button_t button);
        // hides the cursor and gives unbounded movement (mouse look)
        void set_mouse_capture(bool capture);
        // cursor movement in pixels since the previous call (0 on the first call after capture changes)
        vec2_t<GLfloat> mouse_delta();

        void resize(int width, int height);
        void force_close(glfw_bool_t close = glfw_bool::TRUE);
        vec2_t<GLfloat> cursor_position() const;
        bool left_button_down() const;
        void set_vsync(bool enabled);
        void begin_overlay();
        void end_overlay();

        void clear_color(color_t color);
        void clear_color(uint32_t rgba);

        // calls ft::ft_face_t::set_pixel_sizes
        bool set_pixel_sizes(ft::ft_face_t& ft_face, vec2_t<FT_UInt> pixel_size);

        // used internally, not really meant to use outside
        void _resize_cache();
    };
}
