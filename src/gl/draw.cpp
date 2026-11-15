#include "draw.hpp"

#include <span>

gl::draw2d_t::draw2d_t(ctx_t& ctx) :
    ctx(ctx)
{
    initialized  = false;
    _alpha_count = 0;
    _color_count = 0;
}

bool gl::draw2d_t::init()
{
    assert(ctx.initialized);
    assert(!initialized);

    vao.gen_array();
    if (!vao.bind()) return false;
    vbo_pos.gen_buffer();
    if (!vbo_pos.add_vad(vad_t(0, vad_cpv_t::comp_2, 2, 0))) return false;
    vbo_alpha.gen_buffer();
    if (!vbo_alpha.add_vad(vad_t(1, vad_cpv_t::comp_1, 1, 0))) return false;
    vbo_color.gen_buffer();
    if (!vbo_color.add_vad(vad_t(2, vad_cpv_t::comp_3, 3, 0))) return false;

    initialized = true;
    return true;
}

bool gl::draw2d_t::_pad_attribs(std::size_t count, bool has_alpha, bool has_color)
{
    if (!has_alpha && _alpha_count < count) {
        std::vector<f32> zeros(count, 0.0f);
        if (!vbo_alpha.init_buffer(std::span<f32>(zeros), dsu_t::static_draw)) return false;
        _alpha_count = count;
    }
    if (!has_color && _color_count < count) {
        std::vector<f32> zeros(count * 3, 0.0f);
        if (!vbo_color.init_buffer(std::span<f32>(zeros), dsu_t::static_draw)) return false;
        _color_count = count;
    }
    if (has_alpha) _alpha_count = count;
    if (has_color) _color_count = count;
    return true;
}

bool gl::draw2d_t::draw_circle(vec2_t<gl::f32> c, gl::f32 r, std::size_t num_segments)
{
    assert(initialized);

    vertices.resize(num_segments);
    vertices_flat.resize(2*num_segments);

    vec2_t<gl::f32> nc = 2*c/ctx.screen_size - vec2_t<gl::f32>::one();
    vec2_t<gl::f32> nr = 2*r/ctx.screen_size;

    for(std::size_t i = 0; i < num_segments; i++)
    {
        gl::f32 theta = 2.0f * PI * gl::f32(i) / gl::f32(num_segments);

        vertices[i] = nc + nr * vec2_t<gl::f32>::make_angle(theta);
    }

    vec2_t<gl::f32>::spread(vertices_flat, vertices);
    std::span<gl::f32> vertices_span(vertices_flat.data(), num_segments * 2);

    if (!vao.bind()) return false;
    if (!vbo_pos.init_buffer(vertices_span, dsu_t::static_draw)) return false;
    if (!_pad_attribs(num_segments, false, false)) return false;

    glDrawArrays(GL_TRIANGLE_FAN, 0, num_segments);

    return true;
}

bool gl::draw2d_t::draw_circle_grad(vec2_t<gl::f32> c, gl::f32 r, gl::f32 shift, std::size_t num_segments)
{
    assert(initialized);

    vertices.resize(num_segments);
    vertices_flat.resize(2*num_segments);
    alphas.resize(num_segments);

    vec2_t<gl::f32> nc = 2*c/ctx.screen_size - vec2_t<gl::f32>::one();
    vec2_t<gl::f32> nr = 2*r/ctx.screen_size;

    for(std::size_t i = 0; i < num_segments; i++)
    {
        gl::f32 theta = 2.0f * PI * gl::f32(i) / gl::f32(num_segments);

        vertices[i] = nc + nr * vec2_t<gl::f32>::make_angle(theta + shift);
        alphas[i]   = std::fmod(gl::f32(i)/num_segments, 1.0);
    }

    vec2_t<gl::f32>::spread(vertices_flat, vertices);
    std::span<gl::f32> vertices_span(vertices_flat.data(), num_segments * 2);
    std::span<gl::f32> alphas_span(alphas.data(), num_segments);

    if (!vao.bind()) return false;
    if (!vbo_pos.init_buffer(vertices_span, dsu_t::static_draw)) return false;
    if (!vbo_alpha.init_buffer(alphas_span, dsu_t::static_draw)) return false;
    if (!_pad_attribs(num_segments, true, false)) return false;

    glDrawArrays(GL_TRIANGLE_FAN, 0, num_segments);

    return true;
}

bool gl::draw2d_t::draw_ellipse(vec2_t<gl::f32> c, vec2_t<gl::f32> r, std::size_t num_segments)
{
    assert(initialized);

    vertices.resize(num_segments);
    vertices_flat.resize(2*num_segments);
    alphas.resize(num_segments);

    vec2_t<gl::f32> nc = 2*c/ctx.screen_size - vec2_t<gl::f32>::one();
    vec2_t<gl::f32> nr = 2*r/ctx.screen_size;

    for(std::size_t i = 0; i < num_segments; i++)
    {
        gl::f32 theta = 2.0f * PI * gl::f32(i) / gl::f32(num_segments);

        vertices[i] = nc + nr * vec2_t<gl::f32>::make_angle(theta);
        alphas[i]   = std::fmod(gl::f32(i)/num_segments, 1.0);
    }

    vec2_t<gl::f32>::spread(vertices_flat, vertices);
    std::span<gl::f32> vertices_span(vertices_flat.data(), num_segments * 2);
    std::span<gl::f32> alphas_span(alphas.data(), num_segments);

    if (!vao.bind()) return false;
    if (!vbo_pos.init_buffer(vertices_span, dsu_t::static_draw)) return false;
    if (!vbo_alpha.init_buffer(alphas_span, dsu_t::static_draw)) return false;
    if (!_pad_attribs(num_segments, true, false)) return false;

    glDrawArrays(GL_TRIANGLE_FAN, 0, num_segments);

    return true;
}

bool gl::draw2d_t::draw_rect(rect_t rect)
{
    assert(initialized);

    rect.spread_pos(vertices_flat);
    rect.spread_color(colors_flat);
    for (size_t j = 0; j < 4; ++j) {
        ctx.to_ndc(vertices_flat[2*j+0], vertices_flat[2*j+1]);
    }

    if (!vao.bind()) return false;
    if (!vbo_pos.init_buffer(std::span<gl::f32>(vertices_flat.data(), 4*2), dsu_t::static_draw)) return false;
    if (!vbo_color.init_buffer(std::span<gl::f32>(colors_flat.data(), 4*3), dsu_t::static_draw)) return false;
    if (!_pad_attribs(4, false, true)) return false;

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    return true;
}
