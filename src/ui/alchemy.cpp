#include "alchemy.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <utility>
#include <vector>

constexpr color_t alchemy_bg    (0.055f, 0.063f, 0.083f, 1.0f);
constexpr color_t alchemy_panel (0.095f, 0.112f, 0.143f, 1.0f);
constexpr color_t alchemy_tile  (0.16f,  0.19f,  0.23f,  1.0f);
constexpr color_t alchemy_accent(0.84f,  0.64f,  0.35f,  1.0f);
constexpr color_t alchemy_cream (0.94f,  0.91f,  0.84f,  1.0f);
constexpr color_t alchemy_muted (0.62f,  0.67f,  0.71f,  1.0f);

constexpr color_t alchemy_green(0.23f, 0.43f, 0.37f, 1.0f);
constexpr color_t alchemy_red  (0.32f, 0.24f, 0.27f, 1.0f);
constexpr color_t alchemy_grey (0.32f, 0.30f, 0.30f, 1.0f);

constexpr int codex_per_page = 6;
constexpr int codex_pages    = (int)((alchemy::recipes.size() + codex_per_page - 1) / codex_per_page);

struct control_t {
    const char *keys, *action;
};
struct control_section_t {
    const char *title;
    std::vector<control_t> controls;
};
static const std::vector<control_section_t> control_sections = {
    {"MOVEMENT", {
        {"W A S D",   "Move"},
        {"SHIFT",     "Sprint"},
        {"SPACE",     "Jump (first-person) / fly up"},
        {"CTRL",      "Fly down"},
        {"F2",        "Switch between fly and first-person"},
    }},
    {"OBJECTS", {
        {"HOLD LMB",  "Grab and drag an object, release to throw it"},
        {"LMB + RMB", "Rotate the held object with the mouse"},
        {"WALK INTO", "Push objects around"},
        {"E",         "Use the object under the crosshair"},
    }},
    {"GENERAL", {
        {"CLICK",     "Capture the mouse"},
        {"ESC",       "Pause"},
        {"TAB",       "Recipe book"},
        {"1  2  3",   "Shading: normals / Gouraud / Phong"},
        {"M",         "Cycle the model material"},
        {"ARROWS  , .", "Move the light"},
        {"F1",        "Hide debug info"},
        {"X  C",      "Debug text size"},
    }},
};

static std::filesystem::path discoveries_path()
{
    const char *dir = std::getenv("LOCALAPPDATA");
    if (!dir) dir = std::getenv("HOME");
    return dir ? std::filesystem::path(dir) / "cgf_alchemy_codex.txt" : std::filesystem::path("cgf_alchemy_codex.txt");
}

alchemy::ui_t::ui_t(gl::gui_t& gui) :
    gui(gui)
{
    initialized = false;
    load_discoveries();
}

bool alchemy::ui_t::init()
{
    assert(!initialized);
    if (!gui.init("shaders/ui.vert", "shaders/ui.frag")) return false;
    initialized = true;
    return true;
}

bool alchemy::ui_t::game_visible() const
{
    return screen == screen_t::game || screen == screen_t::pause ||
           ((screen == screen_t::settings || screen == screen_t::controls) && return_screen == screen_t::pause) ||
           screen == screen_t::codex;
}

bool alchemy::ui_t::game_interactive() const
{
    return screen == screen_t::game;
}

void alchemy::ui_t::set_status(std::string message)
{
    status = std::move(message);
}

void alchemy::ui_t::record_recipe(const recipe_t& recipe)
{
    for (std::size_t i = 0; i < recipes.size(); ++i) {
        if (recipes[i].id != recipe.id) continue;
        if (!discovered[i]) {
            discovered[i] = true;
            save_discoveries();
        }
        status = std::string(recipe.name) + " - " + std::string(recipe.description);
        return;
    }
}

void alchemy::ui_t::update()
{
    assert(initialized);
    gui.update();

    gl::input_t& input = gui.ctx.input;
    if (input.key_hit(gl::kb_key_t::escape)) {
        if (screen == screen_t::game) {
            screen = screen_t::pause;
            input.set_mouse_capture(false);
        }
        else if (screen == screen_t::pause) {
            screen = screen_t::game;
            input.set_mouse_capture(true);
        }
        else if (screen == screen_t::settings) screen = return_screen;
        else if (screen == screen_t::controls) screen = screen_t::settings;
        else if (screen == screen_t::codex)    screen = codex_return;
    }
    if (input.key_hit(gl::kb_key_t::tab)) {
        if (screen == screen_t::game) {
            codex_return = screen_t::game;
            screen       = screen_t::codex;
        }
        else if (screen == screen_t::codex) screen = codex_return;
    }
}

void alchemy::ui_t::draw()
{
    assert(initialized);
    gui.ctx.begin_overlay();
    switch (screen) {
    case screen_t::main_menu: menu();     break;
    case screen_t::settings:  settings(); break;
    case screen_t::controls:  controls(); break;
    case screen_t::game:      hud();      break;
    case screen_t::pause:     paused();   break;
    case screen_t::codex:     codex();    break;
    }
    gui.ctx.end_overlay();
}

void alchemy::ui_t::menu()
{
    const gl::f32 s = gui.scale;
    const gl::f32 x = (gui.width  - 520.0f * s) * 0.5f;
    const gl::f32 y = (gui.height - 590.0f * s) * 0.5f;

    gui.fill({0, 0, gui.width, gui.height}, alchemy_bg);
    gui.fill({x, y, 520.0f * s, 590.0f * s}, alchemy_panel);
    gui.fill({x, y, 520.0f * s, 6.0f * s}, alchemy_accent);
    gui.label("ALCHEMY ROOM", x + 48.0f * s, y + 70.0f * s, 47.0f * s, alchemy_cream);
    gui.label("THE ARCANE WORKSHOP", x + 50.0f * s, y + 142.0f * s, 19.0f * s, alchemy_accent);
    gui.label("Experiment with potions in a 3D laboratory.", x + 50.0f * s, y + 206.0f * s, 17.0f * s, alchemy_muted);

    if (gui.button({x + 50.0f * s, y + 280.0f * s, 420.0f * s, 64.0f * s}, "START GAME", alchemy_green, alchemy_cream)) {
        screen = screen_t::game;
    }
    if (gui.button({x + 50.0f * s, y + 365.0f * s, 420.0f * s, 64.0f * s}, "SETTINGS", alchemy_tile, alchemy_cream)) {
        return_screen = screen_t::main_menu;
        screen        = screen_t::settings;
    }
    if (gui.button({x + 50.0f * s, y + 450.0f * s, 420.0f * s, 64.0f * s}, "EXIT", alchemy_red, alchemy_cream)) {
        gui.ctx.force_close();
    }
}

void alchemy::ui_t::settings()
{
    const gl::f32 s = gui.scale;
    const gl::f32 x = (gui.width  - 540.0f * s) * 0.5f;
    const gl::f32 y = (gui.height - 560.0f * s) * 0.5f;

    if (return_screen == screen_t::main_menu) gui.fill({0, 0, gui.width, gui.height}, alchemy_bg);
    gui.fill({x, y, 540.0f * s, 560.0f * s}, alchemy_panel);
    gui.fill({x, y, 540.0f * s, 5.0f * s}, alchemy_accent);
    gui.label("SETTINGS", x + 44.0f * s, y + 52.0f * s, 39.0f * s, alchemy_cream);
    gui.label("DISPLAY", x + 44.0f * s, y + 133.0f * s, 19.0f * s, alchemy_accent);

    if (gui.button({x + 42.0f * s, y + 183.0f * s, 456.0f * s, 60.0f * s},
                   vsync ? "VSYNC: ON" : "VSYNC: OFF", alchemy_tile, alchemy_cream)) {
        vsync = !vsync;
        gui.ctx.set_vsync(vsync);
    }
    if (gui.button({x + 42.0f * s, y + 260.0f * s, 456.0f * s, 60.0f * s},
                   limit_120 ? "FPS LIMIT: 120" : "FPS LIMIT: 60", alchemy_tile, alchemy_cream)) {
        limit_120 = !limit_120;
        gui.ctx.set_fps(limit_120 ? 120.0f : 60.0f);
    }

    gui.label("CONTROLS", x + 44.0f * s, y + 353.0f * s, 19.0f * s, alchemy_accent);
    if (gui.button({x + 42.0f * s, y + 392.0f * s, 456.0f * s, 60.0f * s}, "SHOW CONTROLS", alchemy_tile, alchemy_cream)) {
        screen = screen_t::controls;
    }
    if (gui.button({x + 42.0f * s, y + 472.0f * s, 456.0f * s, 55.0f * s}, "BACK", alchemy_grey, alchemy_cream)) {
        screen = return_screen;
    }
}

void alchemy::ui_t::controls()
{
    const gl::f32 s = gui.scale;
    const gl::f32 w = 700.0f * s, h = 790.0f * s;
    const gl::f32 x = (gui.width  - w) * 0.5f;
    const gl::f32 y = (gui.height - h) * 0.5f;

    if (return_screen == screen_t::main_menu) gui.fill({0, 0, gui.width, gui.height}, alchemy_bg);
    gui.fill({x, y, w, h}, alchemy_panel);
    gui.fill({x, y, w, 5.0f * s}, alchemy_accent);
    gui.label("CONTROLS", x + 44.0f * s, y + 34.0f * s, 36.0f * s, alchemy_cream);

    gl::f32 row_y = y + 98.0f * s;
    for (const control_section_t& section : control_sections) {
        gui.label(section.title, x + 44.0f * s, row_y, 17.0f * s, alchemy_accent);
        row_y += 30.0f * s;
        for (const control_t& control : section.controls) {
            gui.label(control.keys,   x + 44.0f * s,  row_y, 16.0f * s, alchemy_cream);
            gui.label(control.action, x + 210.0f * s, row_y, 16.0f * s, alchemy_muted);
            row_y += 26.0f * s;
        }
        row_y += 14.0f * s;
    }

    if (gui.button({x + 44.0f * s, y + h - 82.0f * s, w - 88.0f * s, 54.0f * s}, "BACK", alchemy_grey, alchemy_cream)) {
        screen = screen_t::settings;
    }
}

void alchemy::ui_t::hud()
{
    const gl::f32 s  = gui.scale;
    const gl::f32 by = gui.height - 96.0f * s;

    gui.fill({14.0f * s, 14.0f * s, 390.0f * s, 108.0f * s}, alchemy_panel);
    gui.fill({14.0f * s, 14.0f * s, 5.0f * s, 108.0f * s}, alchemy_accent);
    gui.label("ALCHEMY ROOM", 32.0f * s, 27.0f * s, 23.0f * s, alchemy_cream);
    gui.label("ESC  Pause     TAB  Codex", 32.0f * s, 70.0f * s, 17.0f * s, alchemy_muted);

    gui.fill({14.0f * s, by, std::min(gui.width - 28.0f * s, 790.0f * s), 78.0f * s}, alchemy_panel);
    gui.label("STATUS", 30.0f * s, by + 9.0f * s, 14.0f * s, alchemy_accent);
    gui.label(status.substr(0, 72), 30.0f * s, by + 36.0f * s, 16.0f * s, alchemy_cream);
}

void alchemy::ui_t::paused()
{
    hud();

    const gl::f32 s = gui.scale;
    const gl::f32 x = (gui.width  - 460.0f * s) * 0.5f;
    const gl::f32 y = (gui.height - 565.0f * s) * 0.5f;

    gui.fill({x, y, 460.0f * s, 565.0f * s}, alchemy_bg);
    gui.fill({x, y, 460.0f * s, 5.0f * s}, alchemy_accent);
    gui.label("PAUSED", x + 43.0f * s, y + 42.0f * s, 40.0f * s, alchemy_cream);

    if (gui.button({x + 40.0f * s, y + 126.0f * s, 380.0f * s, 62.0f * s}, "RESUME", alchemy_green, alchemy_cream)) {
        screen = screen_t::game;
    }
    if (gui.button({x + 40.0f * s, y + 207.0f * s, 380.0f * s, 62.0f * s}, "SETTINGS", alchemy_tile, alchemy_cream)) {
        return_screen = screen_t::pause;
        screen        = screen_t::settings;
    }
    if (gui.button({x + 40.0f * s, y + 288.0f * s, 380.0f * s, 62.0f * s}, "RECIPE BOOK", alchemy_tile, alchemy_cream)) {
        codex_return = screen_t::pause;
        screen       = screen_t::codex;
    }
    if (gui.button({x + 40.0f * s, y + 369.0f * s, 380.0f * s, 62.0f * s}, "MAIN MENU", alchemy_tile, alchemy_cream)) {
        screen = screen_t::main_menu;
    }
    if (gui.button({x + 40.0f * s, y + 450.0f * s, 380.0f * s, 62.0f * s}, "EXIT", alchemy_red, alchemy_cream)) {
        gui.ctx.force_close();
    }
}

void alchemy::ui_t::codex()
{
    const gl::f32 s = gui.scale;
    const gl::f32 x = (gui.width  - 690.0f * s) * 0.5f;
    const gl::f32 y = (gui.height - 800.0f * s) * 0.5f;
    const int known = (int)std::count(discovered.begin(), discovered.end(), true);

    gui.fill({x, y, 690.0f * s, 800.0f * s}, alchemy_bg);
    gui.fill({x, y, 690.0f * s, 5.0f * s}, alchemy_accent);
    gui.label("THE ALCHEMY CODEX", x + 38.0f * s, y + 31.0f * s, 32.0f * s, alchemy_cream);
    gui.label(std::to_string(known) + " / " + std::to_string(recipes.size()) + " discovered",
              x + 40.0f * s, y + 91.0f * s, 17.0f * s, alchemy_accent);

    for (int i = 0; i < codex_per_page; ++i) {
        const std::size_t index = (std::size_t)(codex_page * codex_per_page + i);
        if (index >= recipes.size()) break;

        const bool    known_recipe = discovered[index];
        const gl::f32 row_y        = y + (143.0f + i * 93.0f) * s;

        gui.fill({x + 34.0f * s, row_y, 622.0f * s, 80.0f * s}, alchemy_panel);
        gui.fill({x + 34.0f * s, row_y, 4.0f * s, 80.0f * s}, known_recipe ? alchemy_accent : alchemy_muted);
        gui.label(known_recipe ? std::string(recipes[index].name) : "???  Undiscovered",
                  x + 52.0f * s, row_y + 8.0f * s, 20.0f * s, known_recipe ? alchemy_cream : alchemy_muted);
        if (known_recipe) {
            gui.label(std::string(recipes[index].description).substr(0, 70),
                      x + 52.0f * s, row_y + 42.0f * s, 15.0f * s, alchemy_muted);
        }
    }

    if (gui.button({x + 35.0f * s, y + 716.0f * s, 155.0f * s, 54.0f * s}, "PREV", alchemy_tile, alchemy_cream) && codex_page > 0) {
        --codex_page;
    }
    gui.label("PAGE " + std::to_string(codex_page + 1) + "/" + std::to_string(codex_pages),
              x + 272.0f * s, y + 728.0f * s, 17.0f * s, alchemy_accent);
    if (gui.button({x + 500.0f * s, y + 716.0f * s, 155.0f * s, 54.0f * s}, "NEXT", alchemy_tile, alchemy_cream) && codex_page < codex_pages - 1) {
        ++codex_page;
    }
    if (gui.button({x + 500.0f * s, y + 25.0f * s, 155.0f * s, 52.0f * s}, "CLOSE", alchemy_tile, alchemy_cream)) {
        screen = codex_return;
    }
}

void alchemy::ui_t::load_discoveries()
{
    std::ifstream file(discoveries_path());
    std::string id;
    while (file >> id) {
        for (std::size_t i = 0; i < recipes.size(); ++i) {
            if (recipes[i].id == id) discovered[i] = true;
        }
    }
}

void alchemy::ui_t::save_discoveries() const
{
    std::ofstream file(discoveries_path());
    if (!file) return;
    for (std::size_t i = 0; i < recipes.size(); ++i) {
        if (discovered[i]) file << recipes[i].id << '\n';
    }
}
