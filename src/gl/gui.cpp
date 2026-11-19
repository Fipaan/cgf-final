#include "gui.hpp"

#include <algorithm>

using namespace math;

gl::gui_t::gui_t(ctx_t& ctx, draw2d_t& draw2d, text_t& text) :
    ctx(ctx),
    draw2d(draw2d),
    text(text)
{
    initialized = false;
    width   = ref_size.x;
    height  = ref_size.y;
    scale   = 1.0f;
    clicked = false;
}

bool gl::gui_t::init(const char *vertex_path, const char *fragment_path)
{
    assert(!initialized);
    if (!panel_shader.from_files(vertex_path, fragment_path)) return false;
    initialized = true;
    return true;
}

void gl::gui_t::update()
{
    assert(initialized);
    width   = (f32)std::max(ctx.screen_size.x, 1);
    height  = (f32)std::max(ctx.screen_size.y, 1);
    mouse   = ctx.cursor_position();
    clicked = ctx.input.mouse_hit(mouse_left);
    scale   = std::clamp(std::min(width / ref_size.x, height / ref_size.y), scale_min, scale_max);
}

bool gl::gui_t::hovered(box_t box) const
{
    return mouse.x >= box.x && mouse.x <= box.x + box.w && mouse.y >= box.y && mouse.y <= box.y + box.h;
}

void gl::gui_t::fill(box_t box, color_t color)
{
    assert(initialized);
    if (box.w <= 0 || box.h <= 0) return;
    rect_t shape(box.x, height - box.y - box.h, box.w, box.h);
    for (auto& c : shape.colors) c = color;
    panel_shader.use();
    draw2d.draw_rect(shape);
}

void gl::gui_t::label(const std::string& value, f32 x, f32 y, f32 size, color_t color)
{
    assert(initialized);
    text.draw(value, vec2_t<f32>(x, height - y), size, color);
}

bool gl::gui_t::button(box_t box, const std::string& value, color_t color, color_t text_color)
{
    assert(initialized);
    const bool hover = hovered(box);
    if (hover) {
        color.r = std::min(color.r + 0.08f, 1.0f);
        color.g = std::min(color.g + 0.08f, 1.0f);
        color.b = std::min(color.b + 0.08f, 1.0f);
    }
    fill(box, color);
    label(value, box.x + 18.0f * scale, box.y + (box.h - 20.0f * scale) * 0.5f, 20.0f * scale, text_color);
    return clicked && hover;
}
