#include "input.hpp"

#include <cstring>

gl::input_t::input_t()
{
    window         = nullptr;
    mouse_captured = false;
    _mouse_primed  = false;
    _mouse_x = _mouse_y = 0.0;
    std::memset(_key_now,    0, sizeof(_key_now));
    std::memset(_key_prev,   0, sizeof(_key_prev));
    std::memset(_mouse_now,  0, sizeof(_mouse_now));
    std::memset(_mouse_prev, 0, sizeof(_mouse_prev));
}

void gl::input_t::init(GLFWwindow *window)
{
    this->window = window;
}

void gl::input_t::update()
{
    assert(window != nullptr);
    std::memcpy(_key_prev,   _key_now,   sizeof(_key_now));
    std::memcpy(_mouse_prev, _mouse_now, sizeof(_mouse_now));

    for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key) {
        _key_now[key] = glfwGetKey(window, key) == GLFW_PRESS;
    }
    for (int button = 0; button <= GLFW_MOUSE_BUTTON_LAST; ++button) {
        _mouse_now[button] = glfwGetMouseButton(window, button) == GLFW_PRESS;
    }
}

gl::kb_key_state_t gl::input_t::get_key(kb_key_t key) const
{
    assert(window != nullptr);
    return kb_key_state_t(glfwGetKey(window, key));
}

gl::kb_key_state_t gl::input_t::get_mouse_button(mouse_button_t button) const
{
    assert(window != nullptr);
    return glfwGetMouseButton(window, button) == kb_key_state_t::press ?
           kb_key_state_t::press : kb_key_state_t::release;
}

bool gl::input_t::key_held(kb_key_t key) const
{
    return _key_now[key];
}

bool gl::input_t::key_hit(kb_key_t key) const
{
    return _key_now[key] && !_key_prev[key];
}

bool gl::input_t::key_released(kb_key_t key) const
{
    return !_key_now[key] && _key_prev[key];
}

bool gl::input_t::mouse_held(mouse_button_t button) const
{
    return _mouse_now[button];
}

bool gl::input_t::mouse_hit(mouse_button_t button) const
{
    return _mouse_now[button] && !_mouse_prev[button];
}

bool gl::input_t::mouse_released(mouse_button_t button) const
{
    return !_mouse_now[button] && _mouse_prev[button];
}

void gl::input_t::set_mouse_capture(bool capture)
{
    assert(window != nullptr);
    // TODO: GLFW_CURSOR, GLFW_CURSOR_*, GLFW_RAW_MOUSE_MOTION under custom enums
    glfwSetInputMode(window, GLFW_CURSOR, capture ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    if (capture && glfwRawMouseMotionSupported()) {
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, glfw_bool::TRUE);
    }
    mouse_captured = capture;
    _mouse_primed  = false;
}

vec2_t<gl::f32> gl::input_t::mouse_delta()
{
    assert(window != nullptr);
    double x, y;
    glfwGetCursorPos(window, &x, &y);

    vec2_t<gl::f32> d(0.0f, 0.0f);
    if (_mouse_primed) d = vec2_t<gl::f32>((gl::f32)(x - _mouse_x), (gl::f32)(y - _mouse_y));
    _mouse_x = x;
    _mouse_y = y;
    _mouse_primed = true;
    return d;
}

vec2_t<double> gl::input_t::cursor_position() const
{
    assert(window != nullptr);
    double x = 0.0;
    double y = 0.0;
    glfwGetCursorPos(window, &x, &y);
    return vec2_t<double>(x, y);
}
