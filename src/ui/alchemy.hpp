#pragma once

#include <array>
#include <cstddef>
#include <string>

#include <gl/ctx.hpp>
#include <gl/shader.hpp>
#include <ui/alchemy_recipes.hpp>

namespace alchemy {

enum class screen_t { main_menu, settings, game, pause, codex };

// TODO: text is too small
class ui_t {
public:
    ui_t();
    bool init();
    void update(gl::ctx_t& ctx);
    void draw(gl::ctx_t& ctx, ft::ft_face_t& face, gl::shader_t& text_shader);
    bool game_visible() const;
    bool game_interactive() const;
    screen_t screen() const;
    void record_recipe(const recipe_t& recipe);
    void set_status(std::string message);

private:
    struct rect_t {
        float x, y, w, h;
    };

    gl::shader_t panel_shader_;
    screen_t screen_ = screen_t::main_menu;
    screen_t return_screen_ = screen_t::main_menu;
    screen_t codex_return_ = screen_t::game;
    bool vsync_ = true;
    bool limit_120_ = false;
    bool previous_mouse_ = false;
    bool previous_escape_ = false;
    bool previous_tab_ = false;
    bool clicked_ = false;
    float mouse_x_ = -1.0f;
    float mouse_y_ = -1.0f;
    float width_ = 1200.0f;
    float height_ = 900.0f;
    float scale_ = 1.0f;
    int codex_page_ = 0;
    std::array<bool, recipes.size()> discovered_ = {};
    std::string status_ = "Find ingredients and use the cauldron to brew potions.";

    void fill(gl::ctx_t& ctx, rect_t r, color_t color);
    void label(gl::ctx_t& ctx, ft::ft_face_t& face, gl::shader_t& text_shader,
               const std::string& value, float x, float y, float size, color_t color);
    bool button(gl::ctx_t& ctx, ft::ft_face_t& face, gl::shader_t& text_shader,
                rect_t r, const std::string& value, color_t color);
    bool hovered(rect_t r) const;
    bool pressed(gl::ctx_t& ctx, gl::kb_key_t key, bool& previous);
    void menu(gl::ctx_t& ctx, ft::ft_face_t& face, gl::shader_t& text_shader);
    void settings(gl::ctx_t& ctx, ft::ft_face_t& face, gl::shader_t& text_shader);
    void hud(gl::ctx_t& ctx, ft::ft_face_t& face, gl::shader_t& text_shader);
    void paused(gl::ctx_t& ctx, ft::ft_face_t& face, gl::shader_t& text_shader);
    void codex(gl::ctx_t& ctx, ft::ft_face_t& face, gl::shader_t& text_shader);
    void load_discoveries();
    void save_discoveries() const;
};

}
