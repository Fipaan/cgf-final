/*
 * gl/draw.hpp -
 *     Immediate 2D shapes (circles, ellipses, rectangles). Positions are in framebuffer pixels
 *     (origin is bottom-left), the conversion to NDC is done here.
 *     Attribute locations: 0 - position, 1 - alpha, 2 - color, they must match the shader used.
 */
#pragma once

#include <vector>

#include <core/gl/shader.hpp>
#include <2d/shape.hpp>
#include <gl/ctx.hpp>

/* Usage:
```cpp
gl::draw2d_t draw2d(ctx);
if (!draw2d.init()) return 1;

shader.use();
draw2d.draw_circle(vec2_t<gl::f32>(100.0f, 100.0f), 50.0f, 64);
```
*/
namespace gl {
    struct draw2d_t {
        bool initialized;
        ctx_t& ctx;

        std::vector<vec2_t<f32>> vertices;
        std::vector<f32> vertices_flat, alphas, colors_flat;

        vao_t vao;
        vbo_t vbo_pos, vbo_alpha, vbo_color;

        // vertex count each attribute buffer can serve, see `_pad_attribs`
        std::size_t _alpha_count, _color_count;

        enum draw2d_vbo_kind_t {
            pos   = 0,
            alpha = 1,
            color = 2,
        };

        draw2d_t(ctx_t& ctx);

        bool init();

        bool draw_circle(vec2_t<f32> c, f32 r, std::size_t num_segments);
        bool draw_circle_grad(vec2_t<f32> c, f32 r, f32 shift, std::size_t num_segments);
        bool draw_ellipse(vec2_t<f32> c, vec2_t<f32> r, std::size_t num_segments);
        bool draw_rect(rect_t rect);

        // all attributes are enabled, so every buffer must hold at least `count` vertices even if
        // the shape does not provide the data (reading past a buffer is undefined behaviour);
        // `has_*` tells which of them the caller has just uploaded itself
        bool _pad_attribs(std::size_t count, bool has_alpha, bool has_color);
    };
}
