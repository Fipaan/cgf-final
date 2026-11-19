#include "ctx.hpp"

#include <algorithm>
#include <thread>

#include <core/log.hpp>

using namespace math;

gl::glfw_ctx_t::glfw_ctx_t()
{
    initialized = false;
}

gl::glfw_ctx_t::~glfw_ctx_t()
{
    if (initialized) {
        glfwTerminate();
        initialized = false;
    }
}

bool gl::glfw_ctx_t::init()
{
    if (!glfwInit()) {
        glfw_errlog("glfwInit");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE);

    initialized = true;

    return true;
}

gl::ctx_t::ctx_t(glfw_ctx_t& glfw_ctx) :
    glfw_ctx(glfw_ctx)
{
    initialized = false;
    window      = nullptr;
    target_fps  = 0.0f;
    actual_fps  = 0.0f;
    last_frame  = 0.0f;
    DPI         = vec2_t<f32>::one();
}

gl::ctx_t::~ctx_t()
{
    if (window != nullptr) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    initialized = false;
}

bool gl::ctx_t::create_window(int width, int height, const char *title, GLFWmonitor *monitor, GLFWwindow *share)
{
    assert(glfw_ctx.initialized);

    window = glfwCreateWindow(width, height, title, monitor, share);
    if (window == nullptr) {
        glfw_errlog("glfwCreateWindow");
        return false;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfw_errlog("gladLoadGLLoader");
        return false;
    }
    initialized = true;
    input.init(window);

    // Platform may automatically scale window according to DPI
    glfwGetWindowContentScale(window, &DPI.x, &DPI.y);
    infofn("DPI: {:.2f}x{:.2f}", DPI.x, DPI.y);
    resize(width, height);
    // NOTE: hack on a first frame, because glfwGetWindowSize
    //       may return cache value, e.g. on X11 it sends size
    //       to X server (XResizeWindow), that later will retrieve
    //       it using XGetWindowAttributes: https://github.com/glfw/glfw/blob/3.5.1/src/x11_window.c
    glfwPollEvents();
    _resize_cache();

    return true;
}

void gl::ctx_t::set_fps(gl::f32 fps)
{
    target_fps = fps;
    frame_time = duration(1.0 / target_fps);
}

void gl::ctx_t::resize(int width, int height)
{
    assert(initialized);
    screen_size.x = width;
    screen_size.y = height;
    glfwSetWindowSize(window,
        (int)std::round(width/DPI.x),
        (int)std::round(height/DPI.y)
    );
}

void gl::ctx_t::_resize_cache()
{
    assert(initialized);
    glfwGetWindowSize(window, &wscreen_size.x, &wscreen_size.y);
    glViewport(0, 0, screen_size.x, screen_size.y);
}

void gl::ctx_t::before_loop()
{
    last_frame = frame_time.count();
    next_frame = clock::now();
}

void gl::ctx_t::start_loop()
{
    // NOTE: un-comment once emscripten become a target
//    // emscripten maintains its own main loop
//#ifdef __EMSCRIPTEN__
//    if (window_should_close()) {
//        emscripten_cancel_main_loop();
//        return;
//    }
//#endif // __EMSCRIPTEN__
    _start_frame = clock::now();
    input.update();
}

bool gl::ctx_t::on_resize()
{
    assert(initialized);
    screen_size_prev = screen_size;
    glfwGetFramebufferSize(window, &screen_size.x, &screen_size.y);
    bool resized = vec2::dst_sqr(screen_size_prev, screen_size) > 0.0;
    if (resized) _resize_cache();
    return resized;
}

void gl::ctx_t::end_loop()
{
    assert(initialized);
#ifdef ABORT_ON_ANY_GL_ERROR
    if (any_gl_error) force_close();
#endif // ABORT_ON_ANY_GL_ERROR

    glfwSwapBuffers(window);
    glfwPollEvents();

    if (target_fps > 0.0) {
        next_frame += std::chrono::duration_cast<clock::duration>(frame_time);
        std::this_thread::sleep_until(next_frame);
    }
    _end_frame = clock::now();
    last_frame = duration(_end_frame - _start_frame).count();
    actual_fps = 1.0 / last_frame;
}

void gl::ctx_t::to_ndc(gl::f32& x, gl::f32& y)
{
    x = 2 * x / screen_size.x - 1;
    y = 2 * y / screen_size.y - 1;
}
void gl::ctx_t::to_ndc(vec2_t<gl::f32>& v)
{
    to_ndc(v.x, v.y);
}
void gl::ctx_t::from_ndc(gl::f32& x, gl::f32& y)
{
    x = (x + 1) * screen_size.x / 2;
    y = (y + 1) * screen_size.y / 2;
}
void gl::ctx_t::from_ndc(vec2_t<gl::f32>& v)
{
    from_ndc(v.x, v.y);
}

bool gl::ctx_t::window_should_close(void)
{
    assert(initialized);
    return glfwWindowShouldClose(window);
}

void gl::ctx_t::force_close(glfw_bool_t close)
{
    assert(initialized);
    glfwSetWindowShouldClose(window, close);
}

vec2_t<gl::f32> gl::ctx_t::cursor_position() const
{
    vec2_t<double> cursor = input.cursor_position();
    return vec2_t<gl::f32>(
        static_cast<gl::f32>(cursor.x * screen_size.x / std::max(wscreen_size.x, 1)),
        static_cast<gl::f32>(cursor.y * screen_size.y / std::max(wscreen_size.y, 1))
    );
}

void gl::ctx_t::set_vsync(bool enabled)
{
    glfwSwapInterval(enabled ? 1 : 0);
}

void gl::ctx_t::begin_overlay()
{
    overlay_depth = glIsEnabled(GL_DEPTH_TEST);
    overlay_cull  = glIsEnabled(GL_CULL_FACE);
    overlay_blend = glIsEnabled(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void gl::ctx_t::end_overlay()
{
    if (overlay_depth) glEnable(GL_DEPTH_TEST);
    else glDisable(GL_DEPTH_TEST);
    if (overlay_cull) glEnable(GL_CULL_FACE);
    else glDisable(GL_CULL_FACE);
    if (overlay_blend) glEnable(GL_BLEND);
    else glDisable(GL_BLEND);
}

void gl::ctx_t::clear_color(color_t color)
{
    glClearColor(color.r, color.g, color.b, color.a);
}

void gl::ctx_t::clear_color(uint32_t rgba)
{
    clear_color(color_t(rgba));
}
