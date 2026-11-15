/*
 * gl/input.hpp -
 *     Keyboard/mouse state on top of GLFW. `update` takes a snapshot once per frame, so edge queries
 *     (`key_hit`, `mouse_hit`) can be asked any number of times, from any place, in the same frame.
 *     `ctx_t` owns one and calls `update` in `ctx_t::start_loop`.
 */
#pragma once

#include <core/gl/const.hpp>
#include <2d/math.hpp>

namespace gl {
    struct input_t {
        GLFWwindow *window;
        bool   mouse_captured;
        bool   _mouse_primed;
        double _mouse_x, _mouse_y;
        bool   _key_now[GLFW_KEY_LAST + 1], _key_prev[GLFW_KEY_LAST + 1];
        bool   _mouse_now[GLFW_MOUSE_BUTTON_LAST + 1], _mouse_prev[GLFW_MOUSE_BUTTON_LAST + 1];

        input_t();

        void init(GLFWwindow *window);
        // once per frame
        void update();

        // raw polling, ignores the snapshot
        kb_key_state_t get_key(kb_key_t key) const;
        kb_key_state_t get_mouse_button(mouse_button_t button) const;

        bool key_held(kb_key_t key) const;
        // went down this frame
        bool key_hit(kb_key_t key) const;
        // went up this frame
        bool key_released(kb_key_t key) const;

        bool mouse_held(mouse_button_t button) const;
        bool mouse_hit(mouse_button_t button) const;
        bool mouse_released(mouse_button_t button) const;

        // hides the cursor and gives unbounded movement (mouse look)
        void set_mouse_capture(bool capture);
        // cursor movement in pixels since the previous call (0 on the first call after capture changes)
        vec2_t<f32> mouse_delta();
        // in window units, origin is top-left
        vec2_t<double> cursor_position() const;
    };
}
