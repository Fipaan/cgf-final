/*
 * gl/gui.hpp -
 *     Immediate mode GUI building blocks (filled boxes, labels, buttons) on top of `draw2d_t` and
 *     `text_t`. Boxes are in framebuffer pixels with the origin at the top-left corner, y goes down.
 *     Screens themselves (see `ui/`) are built from these.
 */
#pragma once

#include <string>

#include <core/gl/shader.hpp>
#include <gl/ctx.hpp>
#include <gl/draw.hpp>
#include <gl/text.hpp>

/* Usage:
```cpp
gl::gui_t gui(ctx, draw2d, text);
if (!gui.init("shaders/ui.vert", "shaders/ui.frag")) return 1;

// every frame
gui.update();
ctx.begin_overlay();
gui.fill({0, 0, 200, 100}, color_t(0.1f, 0.1f, 0.1f, 1.0f));
if (gui.button({10, 10, 180, 40}, "OK", color_t(0.2f, 0.4f, 0.3f, 1.0f), color_t(1.0f, 1.0f, 1.0f, 1.0f))) {
    // clicked
}
ctx.end_overlay();
```
*/
namespace gl {
    struct box_t {
        f32 x, y, w, h;
    };

    struct gui_t {
        bool initialized;
        ctx_t&    ctx;
        draw2d_t& draw2d;
        text_t&   text;

        shader_t panel_shader;

        // all sizes are designed for `ref_size` and multiplied by `scale`
        math::vec2_t<f32> ref_size = math::vec2_t<f32>(1200.0f, 900.0f);
        f32 scale_min = 0.55f, scale_max = 1.35f;

        // refreshed by `update`
        f32 width, height, scale;
        math::vec2_t<f32> mouse;
        bool clicked;

        gui_t(ctx_t& ctx, draw2d_t& draw2d, text_t& text);

        bool init(const char *vertex_path, const char *fragment_path);
        // once per frame, before any widget
        void update();

        bool hovered(box_t box) const;

        void fill(box_t box, math::color_t color);
        // `y` is the top of the text, `size` is the text height
        void label(const std::string& value, f32 x, f32 y, f32 size, math::color_t color);
        // returns true on the frame it was clicked
        bool button(box_t box, const std::string& value, math::color_t color, math::color_t text_color);
    };
}
