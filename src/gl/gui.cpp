#include "gui.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

gl::gui_t::gui_t(ctx_t& ctx, draw2d_t& draw2d, text_t& text) :
    ctx(ctx), draw2d(draw2d), text(text)
{
    initialized = false;
    width = ref_size.x;
    height = ref_size.y;
    scale = 1.0f;
    mouse = vec2_t<f32>(0.0f, 0.0f);
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
    width = (f32)std::max(ctx.screen_size.x, 1);
    height = (f32)std::max(ctx.screen_size.y, 1);
    const auto previous_mouse = mouse;
    mouse = ctx.cursor_position();
    pointer_moved = std::abs(mouse.x - previous_mouse.x) + std::abs(mouse.y - previous_mouse.y) > 0.5f;
    clicked = !ctx.input.mouse_captured && ctx.input.mouse_hit(mouse_left);
    released = !ctx.input.mouse_captured && ctx.input.mouse_released(mouse_left);
    scale = std::min({width / ref_size.x, height / ref_size.y, scale_max});
    delta = std::clamp(ctx.last_frame, 0.0f, 0.1f);
    elapsed = std::fmod(elapsed + delta, 3600.0f);
}

void gl::gui_t::begin_screen(int scope, bool allow_tab)
{
    const auto& in = ctx.input;
    int direction = 0;
    if (in.key_hit(kb_key_t::down)) direction = 1;
    if (in.key_hit(kb_key_t::up)) direction = -1;
    if (allow_tab && in.key_hit(kb_key_t::tab)) {
        direction = in.key_held(static_cast<kb_key_t>(GLFW_KEY_LEFT_SHIFT)) ||
                    in.key_held(static_cast<kb_key_t>(GLFW_KEY_RIGHT_SHIFT)) ? -1 : 1;
    }
    if (scope != navigation.scope) animations.clear();
    navigation.begin(scope, ctx.input.mouse_captured ? 0 : direction, pointer_moved);
}

void gl::gui_t::end_screen()
{
    navigation.end(released);
}

bool gl::gui_t::hovered(box_t box) const
{
    return !ctx.input.mouse_captured && box.w > 0 && box.h > 0 &&
           mouse.x >= box.x && mouse.x < box.x + box.w &&
           mouse.y >= box.y && mouse.y < box.y + box.h;
}

void gl::gui_t::surface(box_t box, color_t color, f32 radius, color_t border,
                        f32 border_width, int style, f32 amount)
{
    assert(initialized);
    if (box.w <= 0 || box.h <= 0) return;
    rect_t shape(box.x, height - box.y - box.h, box.w, box.h);
    for (auto& c : shape.colors) c = color;
    panel_shader.use();
    panel_shader.uniform("uRect", box.x, height - box.y - box.h, box.w, box.h);
    panel_shader.uniform("uColor", color.r, color.g, color.b, color.a * opacity);
    panel_shader.uniform("uBorder", border.r, border.g, border.b, border.a * opacity);
    panel_shader.uniform("uRadius", radius);
    panel_shader.uniform("uBorderWidth", border_width);
    panel_shader.uniform("uStyle", style);
    panel_shader.uniform("uProgress", amount);
    panel_shader.uniform("uTime", reduced_motion ? 0.0f : elapsed);
    draw2d.draw_rect(shape);
}

void gl::gui_t::fill(box_t box, color_t color)
{
    surface(box, color, 0.0f, color_t(0, 0, 0, 0), 0.0f, 0);
}

void gl::gui_t::panel(box_t box, color_t color, f32 radius, color_t border, f32 border_width)
{
    surface(box, color, radius, border, border_width, 0);
}

void gl::gui_t::backdrop()
{
    surface({0, 0, width, height}, color_t(1, 1, 1, 1), 0, color_t(0, 0, 0, 0), 0, 1);
}

void gl::gui_t::sigil(box_t box, color_t color)
{
    surface(box, color, 0, color_t(0, 0, 0, 0), 0, 2);
}

void gl::gui_t::progress(box_t box, f32 value, color_t color)
{
    panel(box, color_t(0.015f, 0.035f, 0.035f, 1), box.h * 0.5f, color_t(0.5f, 0.65f, 0.55f, 0.15f));
    const f32 fill_width = std::clamp(value, 0.0f, 1.0f) * box.w;
    if (fill_width > 0) panel({box.x, box.y, fill_width, box.h}, color, std::min(box.h, fill_width) * 0.5f,
                              color_t(0, 0, 0, 0), 0);
}

void gl::gui_t::label(const std::string& value, f32 x, f32 y, f32 size, color_t color)
{
    assert(initialized);
    color.a *= opacity;
    if (size <= 0 || color.a <= 0) return;
    text.draw(value, vec2_t<f32>(x, height - y), size, color);
}

void gl::gui_t::fitted_label(const std::string& value, box_t box, f32 size, color_t color)
{
    std::string visible = value;
    const f32 available = std::max(box.w, 0.0f);
    if (text.measure(visible, size).x > available) {
        while (!visible.empty() && text.measure(visible + "...", size).x > available) {
            std::size_t last = visible.size() - 1;
            while (last > 0 && (static_cast<unsigned char>(visible[last]) & 0xc0) == 0x80) --last;
            visible.resize(last);
        }
        visible += "...";
        if (text.measure(visible, size).x > available) return;
    }
    label(visible, box.x, box.y, size, color);
}

gl::f32 gl::gui_t::paragraph(const std::string& value, box_t box, f32 size, color_t color, int max_lines)
{
    if (box.w <= 0 || size <= 0) return 0;
    const f32 line_height = size * 1.4f;
    const int limit = std::min(max_lines, static_cast<int>(box.h / line_height));
    if (limit <= 0) return 0;
    std::istringstream words(value);
    std::string line, word;
    int rows = 0;
    while (words >> word) {
        std::string candidate = line.empty() ? word : line + " " + word;
        if (!line.empty() && text.measure(candidate, size).x > box.w) {
            if (rows + 1 >= limit) {
                fitted_label(candidate + " ...", {box.x, box.y + rows * line_height, box.w, line_height}, size, color);
                return (rows + 1) * line_height;
            }
            fitted_label(line, {box.x, box.y + rows * line_height, box.w, line_height}, size, color);
            ++rows;
            line = word;
        } else line = std::move(candidate);
    }
    if (!line.empty() && rows < limit) {
        fitted_label(line, {box.x, box.y + rows * line_height, box.w, line_height}, size, color);
        ++rows;
    }
    return rows * line_height;
}

bool gl::gui_t::button(box_t box, const std::string& value, color_t color, color_t text_color,
                      bool enabled, const char *id)
{
    const std::string key = id ? id : value;
    const bool hover = enabled && hovered(box);
    const auto& in = ctx.input;
    const bool activate = !in.mouse_captured && (in.key_hit(static_cast<kb_key_t>(GLFW_KEY_ENTER)) ||
        in.key_hit(static_cast<kb_key_t>(GLFW_KEY_KP_ENTER)) || in.key_hit(static_cast<kb_key_t>(GLFW_KEY_SPACE)));
    const bool result = navigation.button(key, enabled, hover, clicked, released, activate);
    const bool focus = enabled && navigation.keyboard && navigation.focused == key;
    const bool active = navigation.armed == key && hover && in.mouse_held(mouse_left);
    auto& animation = animations[key];
    const f32 target = enabled && (focus || (!navigation.keyboard && hover)) ? 1.0f : 0.0f;
    animation.hover = reduced_motion ? target : animate_towards(animation.hover, target, delta, 12.0f);
    animation.press = reduced_motion ? (active ? 1.0f : 0.0f) : animate_towards(animation.press, active ? 1.0f : 0.0f, delta, 22.0f);
    const f32 light = animation.hover;
    const f32 radius = 8.0f * scale;
    if (light > 0.01f) {
        panel({box.x - 3 * scale, box.y - 3 * scale, box.w + 6 * scale, box.h + 6 * scale},
              color_t(0.35f, 0.68f, 0.52f, light * 0.07f), radius + 3 * scale,
              color_t(0.60f, 0.82f, 0.65f, light * 0.12f), scale);
    }
    box.y += animation.press * 1.5f * scale;
    if (!enabled) { color.a *= 0.36f; text_color.a *= 0.40f; }
    const color_t border(0.73f, 0.68f + light * 0.13f, 0.45f + light * 0.20f,
                         enabled ? 0.20f + light * 0.65f : 0.10f);
    surface(box, color, radius, border, scale, 3, light);
    const f32 font_size = 19.0f * scale;
    const f32 left = box.x + (22.0f + light * 3.0f) * scale;
    fitted_label(value, {left, box.y + (box.h - font_size) * 0.5f, box.w - 65.0f * scale, box.h}, font_size, text_color);
    if (enabled) label(">", box.x + box.w - 32.0f * scale, box.y + (box.h - font_size) * 0.5f,
                       font_size, color_t(0.84f, 0.79f, 0.60f, 0.40f + 0.60f * light));
    return result;
}
