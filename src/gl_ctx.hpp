#pragma once

#include "gl_first.hpp"
#include "gl_const.hpp"
#include "gl_shader.hpp"
#include "gl_2d.hpp"
#include "math.hpp"
#include "ft.hpp"

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
        vec2_t<int> screen_size_prev, screen_size, sscreen_size;
        vec2_t<GLfloat> DPI;

        GLfloat    target_fps, actual_fps;
        duration   frame_time;
        time_point _start_frame, _end_frame;
        time_point next_frame;
        GLfloat    last_frame;

        ctx_t(glfw_ctx_t& glfw_ctx);
        ~ctx_t();
        bool create_window(int width, int height, const char *title, GLFWmonitor *monitor, GLFWwindow *share);
 
        void set_fps(GLfloat fps);
        void before_loop();

        void start_loop();
        bool window_should_close(void);
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

        void force_close(glfw_bool_t close = glfw_bool::TRUE);

        void clear_color(color_t color);
        void clear_color(uint32_t rgba);
    };
}
