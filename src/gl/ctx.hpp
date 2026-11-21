/*
 * gl/ctx.hpp -
 *     GL/GLFW context. Owns `GLFWwindow *window`, frame loop (timing, resize, DPI) and input state.
 *     Everything that draws (2D shapes, text, GUI) lives in other headers of this module and only
 *     borrows `ctx_t`.
 */
#pragma once

#include <core/gl/const.hpp>
#include <2d/vec.hpp>
#include <2d/color.hpp>
#include <gl/input.hpp>

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
        if (ctx.input.key_hit(gl::kb_key_t::escape)) ctx.force_close();
        // ... main logic ...
        ctx.end_loop();
    }

    return 0;
}
```
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

        GLFWwindow *window;
        input_t     input;
        // NOTE: wscreen_size is size in window units
        math::vec2_t<int> screen_size_prev, screen_size, wscreen_size;
        math::vec2_t<f32> DPI;

        GLboolean overlay_depth = GL_FALSE, overlay_cull = GL_FALSE, overlay_blend = GL_FALSE;
        f32    target_fps, actual_fps;
        duration   frame_time;
        time_point _start_frame, _end_frame;
        time_point next_frame;
        f32    last_frame;

        ctx_t(glfw_ctx_t& glfw_ctx);
        ~ctx_t();
        bool create_window(int width, int height, const char *title, GLFWmonitor *monitor = nullptr, GLFWwindow *share = nullptr);

        void set_fps(f32 fps);
        void before_loop();

        // also refreshes `input`, so `input.key_hit` & co. are valid for the rest of the frame
        void start_loop();
        bool window_should_close(void);
        // required for proper inner state, returns true if resize happened
        bool on_resize();
        void end_loop();

        void to_ndc(f32& x, f32& y);
        void to_ndc(math::vec2_t<f32>& v);
        void from_ndc(f32& x, f32& y);
        void from_ndc(math::vec2_t<f32>& v);

        void resize(int width, int height);
        void force_close(glfw_bool_t close = glfw_bool::TRUE);
        // cursor in framebuffer pixels, origin is top-left (like GLFW)
        math::vec2_t<f32> cursor_position() const;
        void set_vsync(bool enabled);

        // 2D/text drawing state: no depth test, no culling, alpha blending.
        // `end_overlay` restores whatever was enabled before `begin_overlay`.
        void begin_overlay();
        void end_overlay();

        void clear_color(math::color_t color);
        void clear_color(uint32_t rgba);

        // used internally, not really meant to use outside
        void _resize_cache();
    };
}
