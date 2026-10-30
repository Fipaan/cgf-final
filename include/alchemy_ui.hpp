#pragma once

#include <array>
#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "alchemy_recipes.hpp"

namespace gl {
class ctx_t;
class shader_t;
}
namespace ft {
class ft_face_t;
}

namespace alchemy {

class ui_t {
public:
    std::function<void(const recipe_t&)> on_apply;

    ui_t();
    ~ui_t();
    ui_t(const ui_t&) = delete;
    ui_t& operator=(const ui_t&) = delete;

    void draw(gl::ctx_t& ctx, ft::ft_face_t& face, gl::shader_t& text_shader);
    void clear_selection();
    int selection_size() const;
    const recipe_t* prepared_potion() const;

private:
    struct rect_t {
        float x, y, w, h;
    };
    struct color_t {
        float r, g, b, a;
    };
    struct label_t {
        std::string value;
        float x, y, size;
        color_t color;
    };

    unsigned int quad_program_ = 0;
    unsigned int quad_vao_ = 0;
    unsigned int quad_vbo_ = 0;
    bool graphics_ready_ = false;
    bool previous_mouse_ = false;
    bool book_open_ = false;
    int book_page_ = 0;
    float scale_ = 1.0f;
    float viewport_width_ = 1200.0f;
    float viewport_height_ = 900.0f;
    float cursor_x_ = -1.0f;
    float cursor_y_ = -1.0f;
    bool clicked_ = false;
    std::array<int, 3> selection_ = {-1, -1, -1};
    int selection_count_ = 0;
    std::optional<std::size_t> ready_recipe_;
    std::array<bool, recipes.size()> discovered_ = {};
    std::string message_ = "Choose 1-3 essences to begin.";
    color_t message_color_ = {0.83f, 0.78f, 0.66f, 1.0f};
    std::vector<float> vertices_;
    std::vector<label_t> labels_;

    void initialize_graphics();
    void read_mouse();
    bool contains(rect_t rect) const;
    void rect(rect_t rect, color_t color);
    void line(float x, float y, float w, color_t color);
    void text(std::string value, float x, float y, float size, color_t color);
    bool button(rect_t area, std::string label, color_t color, bool enabled = true);
    void layout();
    void draw_book(float x);
    void add_ingredient(int value);
    void brew();
    void apply_potion();
    void load_discoveries();
    void save_discoveries() const;
    static color_t kind_color(result_kind kind);
    static color_t pigment(int ingredient);
};

}
