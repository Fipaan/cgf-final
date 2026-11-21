/*
 * 2d/shape.hpp -
 *     It handles 2D scene related objects, such as triangles, rectangles, and etc.
 */
#pragma once

#include <core/gl/shader.hpp>
#include <2d/vec.hpp>
#include <2d/color.hpp>
#include <gl/flat.hpp>

namespace math {

struct rect_t {
    vec2_t<gl::f32> pos, size;
    color_t colors[4];

    rect_t(vec2_t<gl::f32> pos, vec2_t<gl::f32> size);
    rect_t(gl::f32 x, gl::f32 y, gl::f32 w, gl::f32 h);
    rect_t();

    void spread_pos(gl::flat_t<gl::f32>& container);
    void spread_color(gl::flat_t<gl::f32>& container, gl::vad_cpv_t cpv);
    // raw version of upper function
    //     rgb - specifies order of components (rgb/bgr)
    //     a   - include alpha component or not
    void spread_color(gl::flat_t<gl::f32>& container, bool rgb = true, bool alpha = false);
};

struct triangle_t {
#ifdef _WIN32
    static constexpr double R_to_side_ratio_eq_tr = 0.5773502691896258;
#else
    static constexpr double R_to_side_ratio_eq_tr = 1.0 / std::sqrt(3.0);
#endif // _WIN32

    vec2_t<gl::f32> vertices[3];
    color_t colors[3];

    // Cache
    gl::f32 sides[3]; // 0-1, 1-2, 2-3
    gl::f32 R;
    void update_cache();
    void change_R(gl::f32 R);
    void change_side(gl::f32 side); // NOTE: works only if uniform

    triangle_t(vec2_t<gl::f32> v1, vec2_t<gl::f32> v2, vec2_t<gl::f32> v3);
    triangle_t(vec2_t<gl::f32> center, vec2_t<gl::f32> up, gl::f32 radius);
    triangle_t();

    vec2_t<gl::f32> get_center();
    void set_center(vec2_t<gl::f32> center);
    void set_up(vec2_t<gl::f32> up);
    void move(vec2_t<gl::f32> vec);
    void rot(gl::f32 angle);

    void spread_pos(gl::flat_t<gl::f32>& container);
    void spread_color(gl::flat_t<gl::f32>& container, gl::vad_cpv_t cpv);
    // raw version of upper function
    //     rgb - specifies order of components (rgb/bgr)
    //     a   - include alpha component or not
    void spread_color(gl::flat_t<gl::f32>& container, bool rgb = true, bool alpha = false);
};

} // namespace math