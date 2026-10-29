#include "gl_ctx.hpp"

#include "basic.hpp"
#include <thread>

////////////////////////////////////////////////////////////////////////////////
// TODO: too many OpenGL error checks, maybe cache something and treat
//       them as assertions.
////////////////////////////////////////////////////////////////////////////////

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

static void glad_noop(const char *name, void *funcptr, int len_args, ...) {}

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

    glad_set_post_callback(glad_noop);

    initialized = true;

    return true;
}

gl::ctx_t::ctx_t(glfw_ctx_t& glfw_ctx) :
    glfw_ctx(glfw_ctx)
{
    initialized = false;
}

gl::ctx_t::~ctx_t() {}

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

    vao.gen_array();
    if (!vao.bind()) return false;
    vbo_pos.gen_buffer();
    if (!vbo_pos.add_vad(vad_t(0, vad_cpv_t::comp_2, 2, 0))) return false;
    vbo_alpha.gen_buffer();
    if (!vbo_alpha.add_vad(vad_t(1, vad_cpv_t::comp_1, 1, 0))) return false;
    vbo_color.gen_buffer();
    if (!vbo_color.add_vad(vad_t(2, vad_cpv_t::comp_3, 3, 0))) return false;
    vbo_xyuv.gen_buffer();
    if (!vbo_xyuv.add_vad(vad_t(3, vad_cpv_t::comp_4, 4, 0))) return false;

    sscreen_size.x = width;
    sscreen_size.y = height;
    glfwGetFramebufferSize(window, &screen_size.x, &screen_size.y);
    DPI = screen_size.cast<GLfloat>() / sscreen_size;
    infofn("DPI: {:.2f}x{:.2f}", DPI.x, DPI.y);

    // OpenGL automatically scales window according to DPI
    sscreen_size /= DPI;
    glfwSetWindowSize(window, sscreen_size.x, sscreen_size.y);
    glfwGetFramebufferSize(window, &screen_size.x, &screen_size.y);
    glViewport(0, 0, screen_size.x, screen_size.y);
    infofn("Screen Size: {}x{}", sscreen_size.x, sscreen_size.y);
    infofn("Window Size: {}x{}", screen_size.x, screen_size.y);

    initialized = true;

    return true;
}

void gl::ctx_t::set_fps(GLfloat fps)
{
    target_fps       = fps;
    frame_time       = duration(1.0 / target_fps);
}

void gl::ctx_t::before_loop()
{
    last_frame = frame_time.count();
    next_frame = clock::now();
}

void gl::ctx_t::start_loop()
{
#ifdef __EMSCRIPTEN__
    if (window_should_close()) {
        emscripten_cancel_main_loop();
        return;
    }
#endif // __EMSCRIPTEN__
    _start_frame = clock::now();
}

bool gl::ctx_t::on_resize()
{
    assert(initialized);
    screen_size_prev = screen_size;
    glfwGetFramebufferSize(window, &screen_size.x, &screen_size.y);
    bool resized = vec2::dst_sqr(screen_size_prev, screen_size) > 0.0;
    if (resized) {
        sscreen_size = screen_size / DPI;
        glViewport(0, 0, screen_size.x, screen_size.y);
    }
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

void gl::ctx_t::to_ndc(GLfloat& x, GLfloat& y)
{
    x = 2 * x / screen_size.x - 1;
    y = 2 * y / screen_size.y - 1;
}
void gl::ctx_t::to_ndc(vec2_t<GLfloat>& v)
{
    to_ndc(v.x, v.y);
}
void gl::ctx_t::from_ndc(GLfloat& x, GLfloat& y)
{
    x = (x + 1) * screen_size.x / 2;
    y = (y + 1) * screen_size.y / 2;
}
void gl::ctx_t::from_ndc(vec2_t<GLfloat>& v)
{
    from_ndc(v.x, v.y);
}

bool gl::ctx_t::window_should_close(void)
{
    assert(initialized);
    return glfwWindowShouldClose(window);
}

bool gl::ctx_t::draw_circle(vec2_t<GLfloat> c, GLfloat r, std::size_t num_segments)
{
    assert(initialized);

    vertices.resize(num_segments);
    vertices_flat.resize(2*num_segments);

    vec2_t<GLfloat> nc = 2*c/screen_size - vec2_t<GLfloat>::one();
    vec2_t<GLfloat> nr = 2*r/screen_size;

    for(std::size_t i = 0; i < num_segments; i++)
    {
        GLfloat theta = 2.0f * PI * GLfloat(i) / GLfloat(num_segments);

        vertices[i] = nc + nr * vec2_t<GLfloat>::make_angle(theta);
    }

    vec2_t<GLfloat>::spread(vertices_flat, vertices);
    std::span<GLfloat> vertices_span(vertices_flat.data(), num_segments * 2);

    if (!vao.bind()) return false;
    if (!vbo_pos.init_buffer(vertices_span, dsu_t::static_draw)) return false;
    
    glDrawArrays(GL_TRIANGLE_FAN, 0, num_segments);

    return true;
}

bool gl::ctx_t::draw_circle_grad(vec2_t<GLfloat> c, GLfloat r, GLfloat shift, std::size_t num_segments)
{
    assert(initialized);

    vertices.resize(num_segments);
    vertices_flat.resize(2*num_segments);
    alphas.resize(num_segments);

    vec2_t<GLfloat> nc = 2*c/screen_size - vec2_t<GLfloat>::one();
    vec2_t<GLfloat> nr = 2*r/screen_size;

    for(std::size_t i = 0; i < num_segments; i++)
    {
        GLfloat theta = 2.0f * PI * GLfloat(i) / GLfloat(num_segments);

        vertices[i] = nc + nr * vec2_t<GLfloat>::make_angle(theta + shift);
        alphas[i]   = std::fmod(GLfloat(i)/num_segments, 1.0);
    }

    vec2_t<GLfloat>::spread(vertices_flat, vertices);
    std::span<GLfloat> vertices_span(vertices_flat.data(), num_segments * 2);
    std::span<GLfloat> alphas_span(alphas.data(), num_segments);

    if (!vao.bind()) return false;
    if (!vbo_pos.init_buffer(vertices_span, dsu_t::static_draw)) return false;
    if (!vbo_alpha.init_buffer(alphas_span, dsu_t::static_draw)) return false;

    glDrawArrays(GL_TRIANGLE_FAN, 0, num_segments);

    return true;
}

bool gl::ctx_t::draw_ellipse(vec2_t<GLfloat> c, vec2_t<GLfloat> r, std::size_t num_segments)
{
    assert(initialized);

    vertices.resize(num_segments);
    vertices_flat.resize(2*num_segments);

    vec2_t<GLfloat> nc = 2*c/screen_size - vec2_t<GLfloat>::one();
    vec2_t<GLfloat> nr = 2*r/screen_size;

    for(std::size_t i = 0; i < num_segments; i++)
    {
        GLfloat theta = 2.0f * PI * GLfloat(i) / GLfloat(num_segments);

        vertices[i] = nc + nr * vec2_t<GLfloat>::make_angle(theta);
        alphas[i]   = std::fmod(GLfloat(i)/num_segments, 1.0);
    }


    vec2_t<GLfloat>::spread(vertices_flat, vertices);
    std::span<GLfloat> vertices_span(vertices_flat.data(), num_segments * 2);
    std::span<GLfloat> alphas_span(alphas.data(), num_segments);

    if (!vao.bind()) return false;
    if (!vbo_pos.init_buffer(vertices_span, dsu_t::static_draw)) return false;
    if (!vbo_alpha.init_buffer(alphas_span, dsu_t::static_draw)) return false;

    glDrawArrays(GL_TRIANGLE_FAN, 0, num_segments);

    return true;
}

void gl::ctx_t::draw_rect(rect_t rect)
{
    rect.spread_pos(vertices_flat);
    rect.spread_color(colors_flat);
    for (size_t j = 0; j < 4; ++j) {
        to_ndc(vertices_flat[2*j+0], vertices_flat[2*j+1]);
    }
    vbo_pos.init_buffer(std::span<GLfloat>(vertices_flat.data(), 4*2), gl::dsu_t::static_draw);
    vbo_color.init_buffer(std::span<GLfloat>(colors_flat.data(), 4*3), gl::dsu_t::static_draw);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

bool gl::ctx_t::draw_text(ft::ft_face_t& ft_face, std::string text, vec2_t<GLfloat> pos, GLfloat height)
{
    const vec2_t<GLfloat> scale = height / ft_face.base_size.y;
    FT_Error error;

    const FT_Size_Metrics& metrics = ft_face.face->size->metrics;
    const FT_Pos ascender = (metrics.ascender >> 6) * scale.y;
    const FT_Pos line_h   = (metrics.height   >> 6) * scale.y;

    FT_Pos pen_x = pos.x;
    FT_Pos pen_y = pos.y - ascender;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ft_face.texture);
    if (!vao.bind()) return false;

    for (size_t i = 0; i < text.length();) {
        char32_t c = ft::utf8_next(text, i);

        if (c == '\n') {
            pen_x  = pos.x;
            pen_y -= line_h;
            continue;
        }

        if (FT_Load_Char(ft_face.face, c, FT_LOAD_RENDER)) continue;
        FT_GlyphSlot slot = ft_face.face->glyph;
        FT_Bitmap *bm = &slot->bitmap;

        // space has an empty bitmap
        if (bm->width && bm->rows) {
            glPixelStorei(GL_UNPACK_ROW_LENGTH, bm->pitch);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, bm->width, bm->rows, 0,
                         GL_RED, GL_UNSIGNED_BYTE, bm->buffer);

            float x0 = pen_x + slot->bitmap_left * scale.x;
            float x1 = x0 + bm->width * scale.x;
            float yt = pen_y + slot->bitmap_top * scale.y;
            float yb = yt - bm->rows * scale.y;

            float quad[] = {
                x0, yt, 0, 0,
                x1, yt, 1, 0,
                x1, yb, 1, 1,
                x0, yt, 0, 0,
                x1, yb, 1, 1,
                x0, yb, 0, 1,
            };
            if (!vbo_xyuv.init_buffer(std::span<GLfloat>(quad), dsu_t::static_draw)) return false;
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        pen_x += (slot->advance.x >> 6) * scale.x;
    }

    return true;
}

gl::kb_key_state_t gl::ctx_t::get_key(kb_key_t key)
{
    assert(initialized);
    return kb_key_state_t(glfwGetKey(window, key));
}

void gl::ctx_t::force_close(glfw_bool_t close)
{
    assert(initialized);
    glfwSetWindowShouldClose(window, close);
}

void gl::ctx_t::clear_color(color_t color)
{
    glClearColor(color.r, color.g, color.b, color.a);
}

void gl::ctx_t::clear_color(uint32_t rgba)
{
    clear_color(color_t::from_hex(rgba));
}
