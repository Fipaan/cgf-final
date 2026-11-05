#include "alchemy.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

namespace alchemy {
namespace {

constexpr color_t bg(0.055f, 0.063f, 0.083f, 1.0f);
constexpr color_t panel(0.095f, 0.112f, 0.143f, 1.0f);
constexpr color_t tile(0.16f, 0.19f, 0.23f, 1.0f);
constexpr color_t accent(0.84f, 0.64f, 0.35f, 1.0f);
constexpr color_t cream(0.94f, 0.91f, 0.84f, 1.0f);
constexpr color_t muted(0.62f, 0.67f, 0.71f, 1.0f);

std::filesystem::path discoveries_path() {
    const char* dir = std::getenv("LOCALAPPDATA");
    if (!dir) dir = std::getenv("HOME");
    return dir ? std::filesystem::path(dir) / "cgf_alchemy_codex.txt" : std::filesystem::path("cgf_alchemy_codex.txt");
}

}

ui_t::ui_t() {
    load_discoveries();
}

bool ui_t::init() {
    return panel_shader_.from_files("shaders/ui.vert", "shaders/ui.frag");
}

bool ui_t::game_visible() const {
    return screen_ == screen_t::game || screen_ == screen_t::pause ||
           (screen_ == screen_t::settings && return_screen_ == screen_t::pause) ||
           screen_ == screen_t::codex;
}

bool ui_t::game_interactive() const {
    return screen_ == screen_t::game;
}

screen_t ui_t::screen() const {
    return screen_;
}

void ui_t::set_status(std::string message) {
    status_ = std::move(message);
}

void ui_t::record_recipe(const recipe_t& recipe) {
    for (std::size_t i = 0; i < recipes.size(); ++i) {
        if (recipes[i].id != recipe.id) continue;
        if (!discovered_[i]) {
            discovered_[i] = true;
            save_discoveries();
        }
        status_ = std::string(recipe.name) + " - " + std::string(recipe.description);
        return;
    }
}

bool ui_t::pressed(gl::ctx_t& ctx, gl::kb_key_t key, bool& previous) {
    const bool now = ctx.get_key(key) == gl::kb_key_state_t::press;
    const bool result = now && !previous;
    previous = now;
    return result;
}

void ui_t::update(gl::ctx_t& ctx) {
    const auto cursor = ctx.cursor_position();
    width_ = static_cast<float>(std::max(ctx.screen_size.x, 1));
    height_ = static_cast<float>(std::max(ctx.screen_size.y, 1));
    mouse_x_ = cursor.x;
    mouse_y_ = cursor.y;
    const bool down = ctx.left_button_down();
    clicked_ = down && !previous_mouse_;
    previous_mouse_ = down;
    scale_ = std::clamp(std::min(width_ / 1200.0f, height_ / 900.0f), 0.55f, 1.35f);

    if (pressed(ctx, gl::kb_key_t::escape, previous_escape_)) {
        if (screen_ == screen_t::game) {
            screen_ = screen_t::pause;
            ctx.set_mouse_capture(false);
        }
        else if (screen_ == screen_t::pause) {
            screen_ = screen_t::game;
            ctx.set_mouse_capture(true);
        }
        else if (screen_ == screen_t::settings) screen_ = return_screen_;
        else if (screen_ == screen_t::codex) screen_ = codex_return_;
    }
    if (pressed(ctx, gl::kb_key_t::tab, previous_tab_)) {
        if (screen_ == screen_t::game) { codex_return_ = screen_t::game; screen_ = screen_t::codex; }
        else if (screen_ == screen_t::codex) screen_ = codex_return_;
    }
}

bool ui_t::hovered(rect_t r) const {
    return mouse_x_ >= r.x && mouse_x_ <= r.x + r.w && mouse_y_ >= r.y && mouse_y_ <= r.y + r.h;
}

void ui_t::fill(gl::ctx_t& ctx, rect_t r, color_t color) {
    if (r.w <= 0 || r.h <= 0) return;
    gl::rect_t shape(r.x, height_ - r.y - r.h, r.w, r.h);
    for (auto& vertex : shape.colors) vertex = color;
    panel_shader_.use();
    ctx.vao.bind();
    ctx.draw_rect(shape);
}

void ui_t::label(gl::ctx_t& ctx, ft::ft_face_t& face, gl::shader_t& text_shader,
                 const std::string& value, float x, float y, float size, color_t color) {
    const float projection[16] = {
        2.0f / width_, 0, 0, 0,
        0, 2.0f / height_, 0, 0,
        0, 0, -1, 0,
        -1, -1, 0, 1
    };
    text_shader.use();
    text_shader.uniform(gl::m_type_t::m4x4, "proj", 1, gl::gl_bool::FALSE, projection);
    text_shader.uniform("glyph", 0);
    text_shader.uniform("color", color.r, color.g, color.b);
    ctx.draw_text(face, value, {x, height_ - y - size}, size);
}

bool ui_t::button(gl::ctx_t& ctx, ft::ft_face_t& face, gl::shader_t& text_shader,
                  rect_t r, const std::string& value, color_t color) {
    if (hovered(r)) {
        color.r = std::min(color.r + 0.08f, 1.0f);
        color.g = std::min(color.g + 0.08f, 1.0f);
        color.b = std::min(color.b + 0.08f, 1.0f);
    }
    fill(ctx, r, color);
    label(ctx, face, text_shader, value, r.x + 18.0f * scale_,
          r.y + (r.h - 20.0f * scale_) * 0.5f, 20.0f * scale_, cream);
    return clicked_ && hovered(r);
}

void ui_t::menu(gl::ctx_t& ctx, ft::ft_face_t& face, gl::shader_t& text_shader) {
    fill(ctx, {0, 0, width_, height_}, bg);
    const float s = scale_;
    const float x = (width_ - 520.0f * s) * 0.5f;
    const float y = (height_ - 590.0f * s) * 0.5f;
    fill(ctx, {x, y, 520.0f * s, 590.0f * s}, panel);
    fill(ctx, {x, y, 520.0f * s, 6.0f * s}, accent);
    label(ctx, face, text_shader, "ALCHEMY ROOM", x + 48.0f * s, y + 70.0f * s, 47.0f * s, cream);
    label(ctx, face, text_shader, "THE ARCANE WORKSHOP", x + 50.0f * s, y + 142.0f * s, 19.0f * s, accent);
    label(ctx, face, text_shader, "Experiment with potions in a 3D laboratory.", x + 50.0f * s, y + 206.0f * s, 17.0f * s, muted);
    if (button(ctx, face, text_shader, {x + 50.0f * s, y + 280.0f * s, 420.0f * s, 64.0f * s}, "START GAME", {0.23f, 0.43f, 0.37f, 1.0f})) screen_ = screen_t::game;
    if (button(ctx, face, text_shader, {x + 50.0f * s, y + 365.0f * s, 420.0f * s, 64.0f * s}, "SETTINGS", tile)) {
        return_screen_ = screen_t::main_menu;
        screen_ = screen_t::settings;
    }
    if (button(ctx, face, text_shader, {x + 50.0f * s, y + 450.0f * s, 420.0f * s, 64.0f * s}, "EXIT", {0.32f, 0.24f, 0.27f, 1.0f})) ctx.force_close();
}

void ui_t::settings(gl::ctx_t& ctx, ft::ft_face_t& face, gl::shader_t& text_shader) {
    if (return_screen_ == screen_t::main_menu) fill(ctx, {0, 0, width_, height_}, bg);
    const float s = scale_;
    const float x = (width_ - 540.0f * s) * 0.5f;
    const float y = (height_ - 560.0f * s) * 0.5f;
    fill(ctx, {x, y, 540.0f * s, 560.0f * s}, panel);
    fill(ctx, {x, y, 540.0f * s, 5.0f * s}, accent);
    label(ctx, face, text_shader, "SETTINGS", x + 44.0f * s, y + 52.0f * s, 39.0f * s, cream);
    label(ctx, face, text_shader, "DISPLAY", x + 44.0f * s, y + 133.0f * s, 19.0f * s, accent);
    if (button(ctx, face, text_shader, {x + 42.0f * s, y + 183.0f * s, 456.0f * s, 60.0f * s},
               vsync_ ? "VSYNC: ON" : "VSYNC: OFF", tile)) {
        vsync_ = !vsync_;
        ctx.set_vsync(vsync_);
    }
    if (button(ctx, face, text_shader, {x + 42.0f * s, y + 260.0f * s, 456.0f * s, 60.0f * s},
               limit_120_ ? "FPS LIMIT: 120" : "FPS LIMIT: 60", tile)) {
        limit_120_ = !limit_120_;
        ctx.set_fps(limit_120_ ? 120.0f : 60.0f);
    }
    label(ctx, face, text_shader, "CONTROLS", x + 44.0f * s, y + 353.0f * s, 19.0f * s, accent);
    label(ctx, face, text_shader, "ESC - Pause   TAB - Recipe book", x + 44.0f * s, y + 391.0f * s, 17.0f * s, muted);
    if (button(ctx, face, text_shader, {x + 42.0f * s, y + 462.0f * s, 456.0f * s, 55.0f * s}, "BACK", {0.32f, 0.30f, 0.30f, 1.0f})) screen_ = return_screen_;
}

void ui_t::hud(gl::ctx_t& ctx, ft::ft_face_t& face, gl::shader_t& text_shader) {
    const float s = scale_;
    fill(ctx, {14.0f * s, 14.0f * s, 390.0f * s, 108.0f * s}, panel);
    fill(ctx, {14.0f * s, 14.0f * s, 5.0f * s, 108.0f * s}, accent);
    label(ctx, face, text_shader, "ALCHEMY ROOM", 32.0f * s, 27.0f * s, 23.0f * s, cream);
    label(ctx, face, text_shader, "ESC  Pause     TAB  Codex", 32.0f * s, 70.0f * s, 17.0f * s, muted);
    const float by = height_ - 96.0f * s;
    fill(ctx, {14.0f * s, by, std::min(width_ - 28.0f * s, 790.0f * s), 78.0f * s}, panel);
    label(ctx, face, text_shader, "STATUS", 30.0f * s, by + 9.0f * s, 14.0f * s, accent);
    label(ctx, face, text_shader, status_.substr(0, 72), 30.0f * s, by + 36.0f * s, 16.0f * s, cream);
}

void ui_t::paused(gl::ctx_t& ctx, ft::ft_face_t& face, gl::shader_t& text_shader) {
    hud(ctx, face, text_shader);
    const float s = scale_;
    const float x = (width_ - 460.0f * s) * 0.5f;
    const float y = (height_ - 565.0f * s) * 0.5f;
    fill(ctx, {x, y, 460.0f * s, 565.0f * s}, bg);
    fill(ctx, {x, y, 460.0f * s, 5.0f * s}, accent);
    label(ctx, face, text_shader, "PAUSED", x + 43.0f * s, y + 42.0f * s, 40.0f * s, cream);
    if (button(ctx, face, text_shader, {x + 40.0f * s, y + 126.0f * s, 380.0f * s, 62.0f * s}, "RESUME", {0.23f, 0.43f, 0.37f, 1.0f})) screen_ = screen_t::game;
    if (button(ctx, face, text_shader, {x + 40.0f * s, y + 207.0f * s, 380.0f * s, 62.0f * s}, "SETTINGS", tile)) {
        return_screen_ = screen_t::pause;
        screen_ = screen_t::settings;
    }
    if (button(ctx, face, text_shader, {x + 40.0f * s, y + 288.0f * s, 380.0f * s, 62.0f * s}, "RECIPE BOOK", tile)) { codex_return_ = screen_t::pause; screen_ = screen_t::codex; }
    if (button(ctx, face, text_shader, {x + 40.0f * s, y + 369.0f * s, 380.0f * s, 62.0f * s}, "MAIN MENU", tile)) screen_ = screen_t::main_menu;
    if (button(ctx, face, text_shader, {x + 40.0f * s, y + 450.0f * s, 380.0f * s, 62.0f * s}, "EXIT", {0.32f, 0.24f, 0.27f, 1.0f})) ctx.force_close();
}

void ui_t::codex(gl::ctx_t& ctx, ft::ft_face_t& face, gl::shader_t& text_shader) {
    const float s = scale_;
    const float x = (width_ - 690.0f * s) * 0.5f;
    const float y = (height_ - 800.0f * s) * 0.5f;
    fill(ctx, {x, y, 690.0f * s, 800.0f * s}, bg);
    fill(ctx, {x, y, 690.0f * s, 5.0f * s}, accent);
    label(ctx, face, text_shader, "THE ALCHEMY CODEX", x + 38.0f * s, y + 31.0f * s, 32.0f * s, cream);
    const int known = static_cast<int>(std::count(discovered_.begin(), discovered_.end(), true));
    label(ctx, face, text_shader, std::to_string(known) + " / 24 discovered", x + 40.0f * s, y + 91.0f * s, 17.0f * s, accent);
    for (int i = 0; i < 6; ++i) {
        const std::size_t index = static_cast<std::size_t>(codex_page_ * 6 + i);
        const float row_y = y + (143.0f + i * 93.0f) * s;
        fill(ctx, {x + 34.0f * s, row_y, 622.0f * s, 80.0f * s}, panel);
        fill(ctx, {x + 34.0f * s, row_y, 4.0f * s, 80.0f * s}, discovered_[index] ? accent : muted);
        label(ctx, face, text_shader, discovered_[index] ? std::string(recipes[index].name) : "???  Undiscovered",
              x + 52.0f * s, row_y + 8.0f * s, 20.0f * s, discovered_[index] ? cream : muted);
        if (discovered_[index]) {
            label(ctx, face, text_shader, std::string(recipes[index].description).substr(0, 70),
                  x + 52.0f * s, row_y + 42.0f * s, 15.0f * s, muted);
        }
    }
    if (button(ctx, face, text_shader, {x + 35.0f * s, y + 716.0f * s, 155.0f * s, 54.0f * s}, "PREV", tile) && codex_page_ > 0) --codex_page_;
    label(ctx, face, text_shader, "PAGE " + std::to_string(codex_page_ + 1) + "/4", x + 272.0f * s, y + 728.0f * s, 17.0f * s, accent);
    if (button(ctx, face, text_shader, {x + 500.0f * s, y + 716.0f * s, 155.0f * s, 54.0f * s}, "NEXT", tile) && codex_page_ < 3) ++codex_page_;
    if (button(ctx, face, text_shader, {x + 500.0f * s, y + 25.0f * s, 155.0f * s, 52.0f * s}, "CLOSE", tile)) screen_ = codex_return_;
}

void ui_t::draw(gl::ctx_t& ctx, ft::ft_face_t& face, gl::shader_t& text_shader) {
    ctx.begin_overlay();
    switch (screen_) {
        case screen_t::main_menu: menu(ctx, face, text_shader); break;
        case screen_t::settings: settings(ctx, face, text_shader); break;
        case screen_t::game: hud(ctx, face, text_shader); break;
        case screen_t::pause: paused(ctx, face, text_shader); break;
        case screen_t::codex: codex(ctx, face, text_shader); break;
    }
    ctx.end_overlay();
}

void ui_t::load_discoveries() {
    std::ifstream file(discoveries_path());
    std::string id;
    while (file >> id) {
        for (std::size_t i = 0; i < recipes.size(); ++i) {
            if (recipes[i].id == id) discovered_[i] = true;
        }
    }
}

void ui_t::save_discoveries() const {
    std::ofstream file(discoveries_path());
    if (!file) return;
    for (std::size_t i = 0; i < recipes.size(); ++i) {
        if (discovered_[i]) file << recipes[i].id << '\n';
    }
}

}
