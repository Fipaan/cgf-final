#include "gl_first.hpp"

#include <cmath>
#include <numbers>
#include <cassert>
#include <algorithm>
#include <vector>
#include <span>
#include "basic.hpp"

#ifdef __set_any_gl_error
    #warn "__set_any_gl_error intrinsic already defined! ABORT_ON_ANY_GL_ERROR may result in incorrect behavior or not compile."
#endif // __set_any_gl_error


#ifdef ABORT_ON_ANY_GL_ERROR
namespace gl {
    bool any_gl_error = false;
    #define __set_any_gl_error(...) gl::any_gl_error = (__VA_ARGS__)
}
#else
    #define __set_any_gl_error(...)
#endif // ABORT_ON_ANY_GL_ERROR

uint32_t gl::gl_first_state_cpp()
{
    // NOTE: `gl::gl_first_state` is constexpr function,
    //       so `gl::gl_first_state_cpp` will be determined
    //       during compilation time, and macro mismatch will
    //       appear immediately.
    constexpr auto state = gl::gl_first_state();
    return state;
}

bool gl::check_gl_error(std::string_view desc_ctx)
{
    GLenum err;
    std::size_t err_count = 0;
    const char *desc = NULL;
    while((err = glGetError()) != GL_NO_ERROR) {
        std::string_view desc;
        switch (err) {
        case GL_INVALID_ENUM:                   desc = "invalid enumeration";                break;
        case GL_INVALID_VALUE:                  desc = "out of range";                       break;
        case GL_INVALID_OPERATION:              desc = "operation not allowed";              break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:  desc = "framebuffer object is not complete"; break;
        case GL_OUT_OF_MEMORY:                  desc = "not enough memory";                  break;
        case GL_STACK_UNDERFLOW:                desc = "stack underflow";                    break;
        case GL_STACK_OVERFLOW:                 desc = "stack overflow";                     break;
        default:                                desc = "unknown error";                      break;
        }
        if (desc_ctx.length() == 0) {
            errorfn("{} ({})", desc, err);
        } else {
            errorfn("{}: {} ({})", desc_ctx, desc, err);
        }
        err_count += 1;
        __set_any_gl_error(true);
    }
    return err_count > 0;
}

bool gl::check_glfw_error(std::string_view desc_ctx)
{
    GLenum err;
    std::size_t err_count = 0;
    const char *desc = NULL;
    while((err = glfwGetError(&desc)) != GLFW_NO_ERROR){
        if (desc_ctx.length() == 0) {
            errorfn("{} ({})", desc, err);
        } else {
            errorfn("{}: {} ({})", desc_ctx, desc, err);
        }
        err_count += 1;
    }
    return err_count > 0;
}

void gl::glfw_errlog(std::string_view funcname)
{
    const char *desc;
    int code = glfwGetError(&desc);
    errorfn("{} failed({}): {}", funcname, code, desc);
}

GLFWglproc gl::glfwGetProcAddressTrace(const char *procname)
{
    GLFWglproc result = glfwGetProcAddress(procname);
    std::uintptr_t result_printable = reinterpret_cast<std::uintptr_t>(result);
    infofn("Loaded {} function at {:#x}", procname, result_printable);

    return result;
}

#undef __set_any_gl_error
