#pragma once

extern "C" {
    #include <glad/glad.h>
    #include <GLFW/glfw3.h>

    #if __EMSCRIPTEN__
    #include <emscripten.h>
    #include <emscripten/html5.h>
    #endif // __EMSCRIPTEN__
}
#include <string_view>
#include <chrono>
#include <cassert>

namespace gl {
    using clock      = std::chrono::steady_clock;
    using time_point = std::chrono::time_point<clock>;
    using duration   = std::chrono::duration<GLfloat>;
#ifdef ABORT_ON_ANY_GL_ERROR
    extern bool any_gl_error;
#endif // ABORT_ON_ANY_GL_ERROR

    bool check_gl_error(std::string_view desc_ctx = "");
    bool check_glfw_error(std::string_view desc_ctx = "");

    // logs current glfw error
    void glfw_errlog(std::string_view funcname);

    // `glfwGetProcAddress` callback but with tracing
    GLFWglproc glfwGetProcAddressTrace(const char *procname);

    // `assertion failed` implies macro mismatch
    void gl_first_state_verify();

    constexpr uint32_t gl_first_state();
    uint32_t gl_first_state_cpp();
}

///// IMPLEMENTATION ///////////////////////////////////////////////////////////

inline void gl::gl_first_state_verify()
{
    assert(gl::gl_first_state() == gl::gl_first_state_cpp());
}

constexpr uint32_t gl::gl_first_state()
{
#ifdef ABORT_ON_ANY_GL_ERROR
    bool abort_on_any_gl_error = true;
#else
    bool abort_on_any_gl_error = false;
#endif // ABORT_ON_ANY_GL_ERROR
    return abort_on_any_gl_error ? 1 : 0;
}
