#include "alchemy_ui.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string_view>
#include <utility>

#include "gl_ctx.hpp"
#include "ft.hpp"

namespace alchemy {
namespace {

unsigned int compile_shader(unsigned int type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

std::filesystem::path save_path() {
    const char* home = std::getenv("LOCALAPPDATA");
    if (!home) home = std::getenv("HOME");
    if (home) return std::filesystem::path(home) / "cgf_alchemy_codex.txt";
    return "cgf_alchemy_codex.txt";
}

}

ui_t::ui_t() {
    load_discoveries();
}

ui_t::~ui_t() {
    if (quad_vbo_) glDeleteBuffers(1, &quad_vbo_);
    if (quad_vao_) glDeleteVertexArrays(1, &quad_vao_);
    if (quad_program_) glDeleteProgram(quad_program_);
}

void ui_t::initialize_graphics() {
    if (graphics_ready_) return;
    static constexpr const char* vertex = R"(#version 330 core
layout(location = 0) in vec2 position;
layout(location = 1) in vec4 color;
out vec4 tint;
void main() {
    gl_Position = vec4(position, 0.0, 1.0);
    tint = color;
}
)";
    static constexpr const char* fragment = R"(#version 330 core
in vec4 tint;
out vec4 frag;
void main() {
    frag = tint;
}
)";

    const unsigned int vs = compile_shader(GL_VERTEX_SHADER, vertex);
    const unsigned int fs = compile_shader(GL_FRAGMENT_SHADER, fragment);
    if (!vs || !fs) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return;
    }
    quad_program_ = glCreateProgram();
    glAttachShader(quad_program_, vs);
    glAttachShader(quad_program_, fs);
    glLinkProgram(quad_program_);
    glDeleteShader(vs);
    glDeleteShader(fs);
    int success = 0;
    glGetProgramiv(quad_program_, GL_LINK_STATUS, &success);
    if (!success) {
        glDeleteProgram(quad_program_);
        quad_program_ = 0;
        return;
    }
    glGenVertexArrays(1, &quad_vao_);
    glGenBuffers(1, &quad_vbo_);
    glBindVertexArray(quad_vao_);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo_);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    graphics_ready_ = true;
}

void ui_t::read_mouse() {
    GLFWwindow* window = glfwGetCurrentContext();
    clicked_ = false;
    if (!window) return;
    int window_width = 0;
    int window_height = 0;
    glfwGetWindowSize(window, &window_width, &window_height);
    double mx = 0.0;
    double my = 0.0;
    glfwGetCursorPos(window, &mx, &my);
    cursor_x_ = static_cast<float>(mx) * viewport_width_ / static_cast<float>(std::max(window_width, 1));
    cursor_y_ = static_cast<float>(my) * viewport_height_ / static_cast<float>(std::max(window_height, 1));
    const bool current = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    clicked_ = current && !previous_mouse_;
    previous_mouse_ = current;
}

bool ui_t::contains(rect_t r) const {
    return cursor_x_ >= r.x && cursor_x_ <= r.x + r.w && cursor_y_ >= r.y && cursor_y_ <= r.y + r.h;
}

void ui_t::rect(rect_t r, color_t c) {
    if (r.w <= 0.0f || r.h <= 0.0f) return;
    const float l = 2.0f * r.x / viewport_width_ - 1.0f;
    const float t = 1.0f - 2.0f * r.y / viewport_height_;
    const float right = 2.0f * (r.x + r.w) / viewport_width_ - 1.0f;
    const float bottom = 1.0f - 2.0f * (r.y + r.h) / viewport_height_;
    const std::array<std::array<float, 2>, 6> positions = {{{l,t}, {l,bottom}, {right,bottom}, {l,t}, {right,bottom}, {right,t}}};
    for (const auto& p : positions) {
        vertices_.insert(vertices_.end(), {p[0], p[1], c.r, c.g, c.b, c.a});
    }
}

void ui_t::line(float x, float y, float w, color_t c) {
    rect({x, y, w, 1.0f * scale_}, c);
}

void ui_t::text(std::string value, float x, float y, float size, color_t c) {
    labels_.push_back({std::move(value), x, y, size, c});
}

bool ui_t::button(rect_t r, std::string label, color_t c, bool enabled) {
    const bool hover = enabled && contains(r);
    if (hover) {
        c.r = std::min(1.0f, c.r + 0.12f);
        c.g = std::min(1.0f, c.g + 0.12f);
        c.b = std::min(1.0f, c.b + 0.12f);
    }
    if (!enabled) c = {0.21f, 0.20f, 0.24f, 0.70f};
    rect(r, c);
    text(std::move(label), r.x + 14.0f * scale_, r.y + (r.h - 22.0f * scale_) * 0.5f, 18.0f * scale_,
         enabled ? color_t{0.98f, 0.96f, 0.89f, 1.0f} : color_t{0.55f, 0.53f, 0.56f, 1.0f});
    return hover && clicked_;
}

ui_t::color_t ui_t::pigment(int ingredient) {
    const auto& c = ingredient_colors[static_cast<std::size_t>(ingredient)];
    return {c[0], c[1], c[2], 1.0f};
}

ui_t::color_t ui_t::kind_color(result_kind kind) {
    switch (kind) {
        case result_kind::basic: return {0.60f, 0.82f, 0.76f, 1.0f};
        case result_kind::success: return {0.44f, 0.86f, 0.66f, 1.0f};
        case result_kind::danger: return {1.00f, 0.43f, 0.39f, 1.0f};
        case result_kind::failure: return {0.66f, 0.63f, 0.70f, 1.0f};
        case result_kind::secret: return {1.00f, 0.82f, 0.38f, 1.0f};
    }
    return {1.0f, 1.0f, 1.0f, 1.0f};
}

void ui_t::add_ingredient(int value) {
    if (selection_count_ >= 3) {
        message_ = "The cauldron holds only three essences.";
        message_color_ = {1.0f, 0.68f, 0.39f, 1.0f};
        return;
    }
    selection_[static_cast<std::size_t>(selection_count_++)] = value;
    ready_recipe_.reset();
    message_ = "Add another essence or press BREW.";
    message_color_ = {0.83f, 0.78f, 0.66f, 1.0f};
}

void ui_t::clear_selection() {
    selection_count_ = 0;
    selection_ = {-1, -1, -1};
    ready_recipe_.reset();
    message_ = "Cauldron emptied. Choose new essences.";
    message_color_ = {0.83f, 0.78f, 0.66f, 1.0f};
}

void ui_t::brew() {
    if (selection_count_ == 0) return;
    const auto match = find_recipe(selection_, selection_count_);
    selection_count_ = 0;
    selection_ = {-1, -1, -1};
    ready_recipe_ = match;
    if (!match) {
        message_ = "Unknown combination. Ingredients were lost.";
        message_color_ = {0.77f, 0.72f, 0.76f, 1.0f};
        return;
    }
    const recipe_t& result = recipes[*match];
    if (!discovered_[*match]) {
        discovered_[*match] = true;
        save_discoveries();
    }
    message_ = std::string(result.name);
    message_color_ = kind_color(result.kind);
    if (result.kind == result_kind::failure) ready_recipe_.reset();
}

void ui_t::apply_potion() {
    if (!ready_recipe_) return;
    if (!on_apply) {
        message_ = "3D potion effect is not connected yet.";
        message_color_ = {1.0f, 0.75f, 0.43f, 1.0f};
        return;
    }
    const recipe_t& recipe = recipes[*ready_recipe_];
    on_apply(recipe);
    message_ = std::string("Activated: ") + std::string(recipe.name);
    message_color_ = kind_color(recipe.kind);
    ready_recipe_.reset();
}

int ui_t::selection_size() const {
    return selection_count_;
}

const recipe_t* ui_t::prepared_potion() const {
    return ready_recipe_ ? &recipes[*ready_recipe_] : nullptr;
}

void ui_t::load_discoveries() {
    std::ifstream file(save_path());
    std::string id;
    while (file >> id) {
        for (std::size_t i = 0; i < recipes.size(); ++i) {
            if (recipes[i].id == id) discovered_[i] = true;
        }
    }
}

void ui_t::save_discoveries() const {
    std::ofstream file(save_path());
    if (!file) return;
    for (std::size_t i = 0; i < recipes.size(); ++i) {
        if (discovered_[i]) file << recipes[i].id << '\n';
    }
}

void ui_t::draw_book(float x) {
    const float s = scale_;
    const float y = 20.0f * s;
    const float w = 438.0f * s;
    rect({x, y, w, 860.0f * s}, {0.065f, 0.075f, 0.095f, 0.95f});
    rect({x, y, 4.0f * s, 860.0f * s}, {0.75f, 0.56f, 0.27f, 1.0f});
    text("THE ALCHEMY CODEX", x + 25.0f * s, 46.0f * s, 27.0f * s, {0.97f, 0.84f, 0.59f, 1.0f});
    const int learned = static_cast<int>(std::count(discovered_.begin(), discovered_.end(), true));
    text(std::to_string(learned) + " / 24 recipes discovered", x + 25.0f * s, 89.0f * s, 17.0f * s, {0.72f, 0.72f, 0.69f, 1.0f});
    line(x + 25.0f * s, 129.0f * s, 386.0f * s, {0.48f, 0.39f, 0.29f, 0.7f});

    for (int i = 0; i < 6; ++i) {
        const int index = book_page_ * 6 + i;
        const auto& recipe = recipes[static_cast<std::size_t>(index)];
        const float row_y = (152.0f + i * 99.0f) * s;
        rect({x + 22.0f * s, row_y, 395.0f * s, 88.0f * s}, {0.13f, 0.14f, 0.17f, 0.86f});
        const color_t tint = discovered_[static_cast<std::size_t>(index)] ? kind_color(recipe.kind) : color_t{0.36f, 0.36f, 0.40f, 1.0f};
        rect({x + 22.0f * s, row_y, 4.0f * s, 88.0f * s}, tint);
        text(discovered_[static_cast<std::size_t>(index)] ? std::string(recipe.name) : "???  Undiscovered",
             x + 38.0f * s, row_y + 11.0f * s, 19.0f * s, tint);
        if (discovered_[static_cast<std::size_t>(index)]) {
            std::string formula;
            for (int n = 0; n < recipe.count; ++n) {
                if (n) formula += " + ";
                formula += ingredients[static_cast<std::size_t>(recipe.ingredients[static_cast<std::size_t>(n)])];
            }
            text(formula, x + 38.0f * s, row_y + 40.0f * s, 14.0f * s, {0.81f, 0.81f, 0.79f, 1.0f});
            text(std::string(category_name(recipe.kind)), x + 38.0f * s, row_y + 63.0f * s, 12.0f * s, tint);
        } else {
            text("Experiment to unlock this formula.", x + 38.0f * s, row_y + 45.0f * s, 14.0f * s,
                 {0.57f, 0.57f, 0.58f, 1.0f});
        }
    }

    if (button({x + 25.0f * s, 775.0f * s, 120.0f * s, 48.0f * s}, "PREV", {0.31f, 0.29f, 0.32f, 1.0f}, book_page_ > 0)) {
        --book_page_;
    }
    if (button({x + 270.0f * s, 775.0f * s, 142.0f * s, 48.0f * s}, "NEXT", {0.31f, 0.29f, 0.32f, 1.0f}, book_page_ < 3)) {
        ++book_page_;
    }
    text("PAGE " + std::to_string(book_page_ + 1) + " / 4", x + 162.0f * s, 789.0f * s, 15.0f * s,
         {0.86f, 0.75f, 0.52f, 1.0f});
}

void ui_t::layout() {
    const float s = scale_;
    const float x = 20.0f * s;
    const float panel_width = 452.0f * s;
    rect({x, 20.0f * s, panel_width, 860.0f * s}, {0.065f, 0.075f, 0.095f, 0.94f});
    rect({x, 20.0f * s, 4.0f * s, 860.0f * s}, {0.75f, 0.56f, 0.27f, 1.0f});
    rect({x + 18.0f * s, 39.0f * s, 42.0f * s, 42.0f * s}, {0.45f, 0.33f, 0.17f, 1.0f});
    text("A", x + 30.0f * s, 43.0f * s, 31.0f * s, {1.0f, 0.85f, 0.57f, 1.0f});
    text("ALCHEMY LAB", x + 72.0f * s, 42.0f * s, 30.0f * s, {0.99f, 0.88f, 0.67f, 1.0f});
    text("THE ARCANE WORKSHOP", x + 73.0f * s, 80.0f * s, 15.0f * s, {0.66f, 0.67f, 0.68f, 1.0f});
    line(x + 22.0f * s, 116.0f * s, 408.0f * s, {0.45f, 0.38f, 0.29f, 0.8f});
    text("01  CHOOSE ESSENCES", x + 25.0f * s, 134.0f * s, 20.0f * s, {0.89f, 0.80f, 0.63f, 1.0f});

    for (int i = 0; i < 6; ++i) {
        const int column = i % 2;
        const int row = i / 2;
        const float bx = x + (24.0f + column * 207.0f) * s;
        const float by = (168.0f + row * 71.0f) * s;
        const color_t tint = pigment(i);
        rect({bx, by, 196.0f * s, 59.0f * s}, {0.17f, 0.18f, 0.21f, 0.97f});
        rect({bx, by, 6.0f * s, 59.0f * s}, tint);
        rect({bx + 17.0f * s, by + 14.0f * s, 30.0f * s, 30.0f * s}, {tint.r, tint.g, tint.b, 0.85f});
        if (contains({bx, by, 196.0f * s, 59.0f * s})) {
            rect({bx, by, 196.0f * s, 59.0f * s}, {tint.r, tint.g, tint.b, 0.13f});
            if (clicked_) add_ingredient(i);
        }
        text(std::string(ingredients[static_cast<std::size_t>(i)]), bx + 58.0f * s, by + 18.0f * s, 18.0f * s,
             {0.98f, 0.97f, 0.92f, 1.0f});
    }

    text("02  CAULDRON", x + 25.0f * s, 406.0f * s, 20.0f * s, {0.89f, 0.80f, 0.63f, 1.0f});
    text(std::to_string(selection_count_) + " / 3", x + 361.0f * s, 406.0f * s, 17.0f * s,
         {0.66f, 0.68f, 0.69f, 1.0f});
    for (int i = 0; i < 3; ++i) {
        const float bx = x + (25.0f + i * 139.0f) * s;
        const float by = 443.0f * s;
        rect({bx, by, 129.0f * s, 49.0f * s}, {0.17f, 0.18f, 0.21f, 0.95f});
        if (i < selection_count_) {
            const int value = selection_[static_cast<std::size_t>(i)];
            rect({bx, by, 4.0f * s, 49.0f * s}, pigment(value));
            text(std::string(ingredients[static_cast<std::size_t>(value)]), bx + 11.0f * s,
                 by + 15.0f * s, 16.0f * s, {0.96f, 0.94f, 0.89f, 1.0f});
        } else {
            text("EMPTY", bx + 27.0f * s, by + 15.0f * s, 15.0f * s,
                 {0.48f, 0.49f, 0.52f, 1.0f});
        }
    }

    if (button({x + 25.0f * s, 511.0f * s, 276.0f * s, 50.0f * s}, "BREW POTION",
               {0.42f, 0.33f, 0.19f, 1.0f}, selection_count_ > 0)) brew();
    if (button({x + 313.0f * s, 511.0f * s, 114.0f * s, 50.0f * s}, "CLEAR",
               {0.31f, 0.28f, 0.30f, 1.0f}, selection_count_ > 0 || ready_recipe_.has_value())) clear_selection();

    text("03  RESULT", x + 25.0f * s, 585.0f * s, 20.0f * s, {0.89f, 0.80f, 0.63f, 1.0f});
    rect({x + 25.0f * s, 621.0f * s, 402.0f * s, 105.0f * s}, {0.13f, 0.15f, 0.17f, 0.98f});
    rect({x + 25.0f * s, 621.0f * s, 4.0f * s, 105.0f * s}, message_color_);
    text(message_, x + 42.0f * s, 632.0f * s, 17.0f * s, message_color_);
    if (ready_recipe_) {
        text(std::string(category_name(recipes[*ready_recipe_].kind)), x + 42.0f * s, 660.0f * s,
             13.0f * s, {0.76f, 0.74f, 0.70f, 1.0f});
        text(std::string(recipes[*ready_recipe_].description), x + 42.0f * s, 691.0f * s,
             12.0f * s, {0.90f, 0.90f, 0.86f, 1.0f});
    } else {
        text("Working recipes create a potion you can use.", x + 42.0f * s, 684.0f * s,
             13.0f * s, {0.66f, 0.67f, 0.68f, 1.0f});
    }

    if (button({x + 25.0f * s, 743.0f * s, 402.0f * s, 47.0f * s}, "ACTIVATE POTION",
               {0.25f, 0.43f, 0.35f, 1.0f}, ready_recipe_.has_value())) apply_potion();
    if (button({x + 25.0f * s, 808.0f * s, 402.0f * s, 47.0f * s},
               book_open_ ? "CLOSE RECIPE BOOK" : "OPEN RECIPE BOOK",
               {0.28f, 0.29f, 0.35f, 1.0f})) book_open_ = !book_open_;

    if (book_open_) {
        const float book_x = std::max(x + 470.0f * s, viewport_width_ - 458.0f * s);
        if (book_x + 438.0f * s <= viewport_width_) draw_book(book_x);
        else text("Expand the window to view the codex.", x + 23.0f * s, 879.0f * s, 14.0f * s,
                  {0.99f, 0.76f, 0.52f, 1.0f});
    }
}

void ui_t::draw(gl::ctx_t& ctx, ft::ft_face_t& face, gl::shader_t& text_shader) {
    viewport_width_ = static_cast<float>(std::max(ctx.screen_size.x, 1));
    viewport_height_ = static_cast<float>(std::max(ctx.screen_size.y, 1));
    scale_ = std::min({viewport_width_ / 1180.0f, viewport_height_ / 900.0f, 1.25f});
    read_mouse();
    vertices_.clear();
    labels_.clear();
    layout();
    initialize_graphics();
    if (!graphics_ready_) return;

    int previous_program = 0;
    int previous_vao = 0;
    int previous_vbo = 0;
    const bool depth = glIsEnabled(GL_DEPTH_TEST) == GL_TRUE;
    const bool blend = glIsEnabled(GL_BLEND) == GL_TRUE;
    const bool cull = glIsEnabled(GL_CULL_FACE) == GL_TRUE;
    glGetIntegerv(GL_CURRENT_PROGRAM, &previous_program);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previous_vao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previous_vbo);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(quad_program_);
    glBindVertexArray(quad_vao_);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices_.size() * sizeof(float)), vertices_.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices_.size() / 6));

    const float projection[16] = {
        2.0f / viewport_width_, 0, 0, 0,
        0, 2.0f / viewport_height_, 0, 0,
        0, 0, -1, 0,
        -1, -1, 0, 1
    };
    text_shader.use();
    text_shader.uniform(gl::m_type_t::m4x4, "proj", 1, gl::gl_bool::FALSE, projection);
    text_shader.uniform("glyph", 0);
    for (const label_t& label : labels_) {
        text_shader.use();
        text_shader.uniform("color", label.color.r, label.color.g, label.color.b);
        ctx.draw_text(face, label.value, vec2_t<GLfloat>(label.x, viewport_height_ - label.y - label.size), label.size);
    }

    glBindVertexArray(previous_vao);
    glBindBuffer(GL_ARRAY_BUFFER, previous_vbo);
    glUseProgram(previous_program);
    if (depth) glEnable(GL_DEPTH_TEST);
    else glDisable(GL_DEPTH_TEST);
    if (cull) glEnable(GL_CULL_FACE);
    else glDisable(GL_CULL_FACE);
    if (blend) glEnable(GL_BLEND);
    else glDisable(GL_BLEND);
}

}
