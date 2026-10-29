#pragma once

#include "gl_first.hpp"
#include "gl_shader.hpp"
#include "math.hpp"

namespace gl {
    struct rect_t {
        vec2_t<GLfloat> pos, size;
        color_t colors[4];

        rect_t(vec2_t<GLfloat> pos, vec2_t<GLfloat> size);
        rect_t(GLfloat x, GLfloat y, GLfloat w, GLfloat h);
        rect_t();
    
        void spread_pos(std::vector<GLfloat>& storage);
        void spread_color(std::vector<GLfloat>& storage, gl::vad_cpv_t cpv = gl::vad_cpv_t::comp_3);
    };

    struct triangle_t {
        static constexpr double R_to_side_ratio_eq_tr = 1.0 / std::sqrt(3.0);

        vec2_t<GLfloat> vertices[3];
        color_t colors[3];

        // Cache
        GLfloat sides[3]; // 0-1, 1-2, 2-3
        GLfloat R;
        void update_cache();
        void change_R(GLfloat R);
        void change_side(GLfloat side); // NOTE: works only if uniform

        triangle_t(vec2_t<GLfloat> v1, vec2_t<GLfloat> v2, vec2_t<GLfloat> v3);
        triangle_t(vec2_t<GLfloat> center, vec2_t<GLfloat> up, GLfloat radius);
        triangle_t();

        vec2_t<GLfloat> get_center();
        void set_center(vec2_t<GLfloat> center);
        void set_up(vec2_t<GLfloat> up);
        void move(vec2_t<GLfloat> vec);
        void rot(GLfloat angle);
    
        void spread_pos(std::vector<GLfloat>& storage);
        void spread_color(std::vector<GLfloat>& storage, gl::vad_cpv_t cpv = gl::vad_cpv_t::comp_3);
    };
}
