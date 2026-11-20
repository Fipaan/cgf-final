#include "alchemy.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <utility>

namespace {
    constexpr color_t ink(0.025f, 0.060f, 0.059f, 0.98f);
    constexpr color_t panel(0.040f, 0.085f, 0.080f, 0.96f);
    constexpr color_t tile(0.065f, 0.125f, 0.112f, 1.0f);
    constexpr color_t gold(0.83f, 0.70f, 0.44f, 1.0f);
    constexpr color_t cream(0.94f, 0.92f, 0.84f, 1.0f);
    constexpr color_t muted(0.61f, 0.72f, 0.66f, 1.0f);
    constexpr color_t emerald(0.12f, 0.30f, 0.24f, 1.0f);
    constexpr color_t quiet(0.07f, 0.10f, 0.095f, 1.0f);
    constexpr color_t danger(0.20f, 0.105f, 0.095f, 1.0f);
    constexpr color_t edge(0.68f, 0.59f, 0.38f, 0.30f);
    constexpr int codex_per_page = 6;
    constexpr int codex_pages = int((alchemy::recipes.size() + codex_per_page - 1) / codex_per_page);

    std::filesystem::path discoveries_path()
    {
        const char *dir = std::getenv("LOCALAPPDATA");
        if (!dir) dir = std::getenv("HOME");
        return dir ? std::filesystem::path(dir) / "cgf_alchemy_codex.txt" : std::filesystem::path("cgf_alchemy_codex.txt");
    }

    color_t recipe_color(alchemy::result_kind kind)
    {
        switch (kind) {
        case alchemy::result_kind::basic:   return color_t(0.48f, 0.74f, 0.65f, 1);
        case alchemy::result_kind::success: return color_t(0.52f, 0.83f, 0.54f, 1);
        case alchemy::result_kind::danger:  return color_t(0.95f, 0.59f, 0.36f, 1);
        case alchemy::result_kind::failure: return color_t(0.75f, 0.53f, 0.49f, 1);
        case alchemy::result_kind::secret:  return color_t(0.77f, 0.64f, 0.95f, 1);
        }
        return gold;
    }

    void bottle(gl::gui_t& gui, gl::f32 x, gl::f32 y, gl::f32 size, color_t color, bool filled)
    {
        gui.panel({x + size * 0.31f, y + size * 0.04f, size * 0.38f, size * 0.15f},
                  gold, size * 0.035f, edge, gui.scale);
        gui.panel({x + size * 0.35f, y + size * 0.19f, size * 0.30f, size * 0.28f},
                  ink, size * 0.04f, color, gui.scale);
        gui.panel({x + size * 0.16f, y + size * 0.42f, size * 0.68f, size * 0.51f},
                  ink, size * 0.15f, color, 1.3f * gui.scale);
        if (filled) {
            color.a = 0.70f;
            gui.panel({x + size * 0.23f, y + size * 0.61f, size * 0.54f, size * 0.24f},
                      color, size * 0.08f, color_t(0, 0, 0, 0), 0);
        }
        gui.fill({x + size * 0.28f, y + size * 0.51f, size * 0.04f, size * 0.18f},
                 color_t(0.84f, 0.94f, 0.88f, 0.22f));
    }
}

alchemy::ui_t::ui_t(gl::gui_t& gui) : gui(gui)
{
    initialized = false;
    load_discoveries();
    discovery_progress = float(std::count(discovered.begin(), discovered.end(), true)) / recipes.size();
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

void alchemy::ui_t::set_screen(screen_t next)
{
    if (screen == next) return;
    screen = next;
    screen_age = 0.0f;
    gui.ctx.input.set_mouse_capture(next == screen_t::game);
}

void alchemy::ui_t::set_status(std::string message)
{
    if (status == message) return;
    status = std::move(message);
    status_age = 0.0f;
}

void alchemy::ui_t::record_recipe(const recipe_t& recipe)
{
    for (std::size_t i = 0; i < recipes.size(); ++i) {
        if (recipes[i].id != recipe.id) continue;
        if (!discovered[i]) {
            discovered[i] = true;
            discovery_title = std::string(recipe.name);
            discovery_age = 0.0f;
            save_discoveries();
        }
        set_status(std::string(recipe.name) + " - " + std::string(recipe.description));
        return;
    }
}

void alchemy::ui_t::update()
{
    assert(initialized);
    gui.reduced_motion = reduced_motion;
    gui.update();
    screen_age += gui.delta;
    status_age += gui.delta;
    discovery_age += gui.delta;
    const float target = float(std::count(discovered.begin(), discovered.end(), true)) / recipes.size();
    discovery_progress = reduced_motion ? target : gl::animate_towards(discovery_progress, target, gui.delta, 5.0f);

    gl::input_t& input = gui.ctx.input;
    if (input.key_hit(gl::kb_key_t::escape)) {
        switch (screen) {
        case screen_t::game:     set_screen(screen_t::pause); break;
        case screen_t::pause:    set_screen(screen_t::game); break;
        case screen_t::settings: set_screen(return_screen); break;
        case screen_t::controls: set_screen(screen_t::settings); break;
        case screen_t::codex:    set_screen(codex_return); break;
        default: break;
        }
    } else if (input.key_hit(gl::kb_key_t::tab)) {
        if (screen == screen_t::game) {
            codex_return = screen_t::game;
            set_screen(screen_t::codex);
        } else if (screen == screen_t::codex) set_screen(codex_return);
    }
    if (screen == screen_t::codex) {
        if (input.key_hit(gl::kb_key_t::left)) codex_page = std::max(codex_page - 1, 0);
        if (input.key_hit(gl::kb_key_t::right)) codex_page = std::min(codex_page + 1, codex_pages - 1);
    }
}

gl::box_t alchemy::ui_t::frame(float width, float height) const
{
    const float s = gui.scale;
    const float arrival = reduced_motion ? 1.0f : gl::ease_out(screen_age / 0.26f);
    return {(gui.width - width * s) * 0.5f,
            (gui.height - height * s) * 0.5f + (1.0f - arrival) * 12.0f * s,
            width * s, height * s};
}

void alchemy::ui_t::window(gl::box_t box, const std::string& title, const std::string& subtitle)
{
    const float s = gui.scale;
    gui.panel({box.x - 6 * s, box.y + 8 * s, box.w + 12 * s, box.h + 4 * s},
              color_t(0, 0, 0, 0.22f), 20 * s, color_t(0, 0, 0, 0), 0);
    gui.panel(box, ink, 14 * s, edge, s);
    gui.fill({box.x + 32 * s, box.y, 72 * s, 2 * s}, gold);
    gui.label(title, box.x + 32 * s, box.y + 30 * s, 34 * s, cream);
    gui.label(subtitle, box.x + 34 * s, box.y + 78 * s, 15 * s, muted);
    gui.fill({box.x + 32 * s, box.y + 112 * s, box.w - 64 * s, s}, edge);
}

void alchemy::ui_t::draw()
{
    assert(initialized);
    gui.ctx.begin_overlay();
    const screen_t drawn_screen = screen;
    gui.begin_screen(int(screen), screen != screen_t::codex && screen != screen_t::game);
    gui.opacity = 1.0f;
    const bool menu_background = screen == screen_t::main_menu ||
        ((screen == screen_t::settings || screen == screen_t::controls) && return_screen == screen_t::main_menu);
    if (menu_background) gui.backdrop();
    else if (screen != screen_t::game)
        gui.fill({0, 0, gui.width, gui.height}, color_t(0.008f, 0.018f, 0.017f, 0.72f));
    if (screen != screen_t::game && !reduced_motion)
        gui.opacity = 0.25f + 0.75f * gl::ease_out(screen_age / 0.22f);
    switch (drawn_screen) {
    case screen_t::main_menu: menu(); break;
    case screen_t::settings: settings(); break;
    case screen_t::controls: controls(); break;
    case screen_t::game: hud(); break;
    case screen_t::pause: paused(); break;
    case screen_t::codex: codex(); break;
    }
    gui.opacity = 1.0f;
    gui.end_screen();
    gui.ctx.end_overlay();
}

void alchemy::ui_t::menu()
{
    const float s = gui.scale;
    const auto b = frame(1060, 670);
    const float x = b.x + 50 * s, y = b.y;
    gui.panel(b, color_t(0.025f, 0.065f, 0.058f, 0.70f), 16 * s, edge, s);
    gui.fill({x, y + 48 * s, 32 * s, 2 * s}, gold);
    gui.label("THE ARCANE WORKSHOP", x + 48 * s, y + 40 * s, 16 * s, gold);
    gui.label("ALCHEMY", x - 3 * s, y + 104 * s, 78 * s, cream);
    gui.label("ROOM", x - 3 * s, y + 184 * s, 100 * s, cream);
    gui.paragraph("A quiet room. A curious mind.\nDiscover what happens when essences meet.",
                  {x, y + 311 * s, 430 * s, 60 * s}, 19 * s, muted, 2);

    if (gui.button({x, y + 405 * s, 410 * s, 60 * s}, "ENTER WORKSHOP", emerald, cream, true, "main.start")) {
        set_screen(screen_t::game);
        return;
    }
    if (gui.button({x, y + 479 * s, 410 * s, 56 * s}, "SETTINGS", tile, cream, true, "main.settings")) {
        return_screen = screen_t::main_menu;
        set_screen(screen_t::settings);
        return;
    }
    if (gui.button({x, y + 549 * s, 410 * s, 56 * s}, "LEAVE GAME", quiet, muted, true, "main.exit"))
        gui.ctx.force_close();

    gui.fill({b.x + 516 * s, y + 52 * s, s, 554 * s}, color_t(0.64f, 0.59f, 0.39f, 0.16f));
    const float cx = b.x + 786 * s;
    gui.sigil({cx - 220 * s, y + 85 * s, 440 * s, 440 * s}, color_t(0.72f, 0.69f, 0.43f, 0.88f));
    bottle(gui, cx - 48 * s, y + 250 * s, 96 * s, color_t(0.48f, 0.83f, 0.66f, 1), true);
    for (std::size_t i = 0; i < ingredient_colors.size(); ++i) {
        const auto& c = ingredient_colors[i];
        const float px = cx - 95 * s + float(i) * 38 * s;
        gui.panel({px, y + 494 * s, 12 * s, 12 * s}, color_t(c[0], c[1], c[2], 0.85f),
                  6 * s, color_t(1, 1, 1, 0.15f), s);
    }
    const int known = int(std::count(discovered.begin(), discovered.end(), true));
    gui.label("THE EXPERIMENT CONTINUES", cx - 150 * s, y + 542 * s, 16 * s, gold);
    gui.label(std::to_string(known) + " / " + std::to_string(recipes.size()) + " formulas discovered",
              cx - 150 * s, y + 575 * s, 17 * s, muted);
    gui.progress({cx - 150 * s, y + 610 * s, 300 * s, 4 * s}, discovery_progress, gold);
    gui.label("ARROWS / TAB  Navigate     ENTER  Select", b.x + 4 * s, b.y + b.h + 22 * s, 14 * s, muted);
}

void alchemy::ui_t::settings()
{
    const float s = gui.scale;
    const auto b = frame(690, 700);
    window(b, "SETTINGS", "Make the workshop feel right.");
    const float x = b.x + 34 * s, w = b.w - 68 * s;
    gui.label("DISPLAY", x, b.y + 137 * s, 15 * s, gold);
    if (gui.button({x, b.y + 169 * s, w, 56 * s}, vsync ? "VSYNC                       ON" : "VSYNC                      OFF",
                   tile, cream, true, "settings.vsync")) {
        vsync = !vsync;
        gui.ctx.set_vsync(vsync);
    }
    if (gui.button({x, b.y + 237 * s, w, 56 * s}, limit_120 ? "FRAME LIMIT            120 FPS" : "FRAME LIMIT             60 FPS",
                   tile, cream, true, "settings.fps")) {
        limit_120 = !limit_120;
        gui.ctx.set_fps(limit_120 ? 120.0f : 60.0f);
    }
    gui.label("COMFORT", x, b.y + 324 * s, 15 * s, gold);
    if (gui.button({x, b.y + 356 * s, w, 56 * s},
                   reduced_motion ? "REDUCED MOTION              ON" : "REDUCED MOTION             OFF",
                   tile, cream, true, "settings.motion")) {
        reduced_motion = !reduced_motion;
        gui.reduced_motion = reduced_motion;
    }
    gui.paragraph("Stops decorative motion and makes interface transitions instant.",
                  {x + 4 * s, b.y + 426 * s, w - 8 * s, 50 * s}, 15 * s, muted, 2);
    if (gui.button({x, b.y + 509 * s, w, 56 * s}, "KEYBOARD & MOUSE", tile, cream, true, "settings.controls")) {
        set_screen(screen_t::controls);
        return;
    }
    if (gui.button({x, b.y + 617 * s, w, 52 * s}, "BACK", quiet, cream, true, "settings.back"))
        set_screen(return_screen);
}

void alchemy::ui_t::controls()
{
    const float s = gui.scale;
    const auto b = frame(1080, 740);
    window(b, "KEYBOARD & MOUSE", "Everything you need to explore the workshop.");
    const float left = b.x + 34 * s, right = b.x + 563 * s;
    auto heading = [&](const char *title, float x, float y) {
        gui.label(title, x, b.y + y * s, 16 * s, gold);
    };
    auto row = [&](const char *key, const char *action, float x, float y) {
        const float top = b.y + y * s;
        gui.panel({x, top, 128 * s, 30 * s}, quiet, 5 * s, edge, s);
        gui.fitted_label(key, {x + 10 * s, top + 7 * s, 110 * s, 20 * s}, 14 * s, cream);
        gui.paragraph(action, {x + 145 * s, top + 4 * s, 332 * s, 42 * s}, 16 * s, muted, 2);
    };
    heading("MOVEMENT", left, 140);
    row("W A S D", "Move around the room", left, 175);
    row("SHIFT", "Sprint", left, 222);
    row("SPACE", "Jump / fly up", left, 269);
    row("CTRL", "Fly down", left, 316);
    row("F2", "First-person / free-fly camera", left, 363);
    heading("OBJECTS", left, 428);
    row("HOLD LMB", "Grab; release to throw", left, 463);
    row("LMB + RMB", "Rotate the held object", left, 510);
    row("E", "Use the object in front of you", left, 557);
    row("WALK INTO", "Push loose objects", left, 604);

    heading("WORKSHOP & VIEW", right, 140);
    row("ESC", "Pause / return", right, 175);
    row("TAB", "Open / close the recipe book", right, 222);
    row("CLICK", "Capture the mouse in the room", right, 269);
    row("1 / 2 / 3", "Normals / Gouraud / Phong", right, 316);
    row("M", "Cycle the model material", right, 363);
    row("ARROWS , .", "Move the scene light", right, 410);
    row("F1", "Show / hide debug information", right, 457);
    row("X / C", "Change debug text size", right, 504);
    heading("IN MENUS", right, 570);
    gui.paragraph("Arrows or Tab to move focus. Enter / Space to select. Left / Right to turn codex pages.",
                  {right, b.y + 601 * s, 472 * s, 55 * s}, 15 * s, muted, 2);
    if (gui.button({b.x + b.w - 250 * s, b.y + b.h - 66 * s, 216 * s, 44 * s},
                   "BACK", quiet, cream, true, "controls.back")) set_screen(screen_t::settings);
}

void alchemy::ui_t::hud()
{
    const float s = gui.scale;
    const int known = int(std::count(discovered.begin(), discovered.end(), true));
    gui.panel({18 * s, 18 * s, 354 * s, 100 * s}, color_t(0.02f, 0.055f, 0.049f, 0.90f), 10 * s, edge, s);
    gui.label("ALCHEMY ROOM", 36 * s, 31 * s, 20 * s, cream);
    gui.label("ESC Pause   TAB Recipe book", 36 * s, 63 * s, 14 * s, muted);
    gui.progress({36 * s, 96 * s, 214 * s, 3 * s}, discovery_progress, gold);
    gui.label(std::to_string(known) + " / " + std::to_string(recipes.size()), 272 * s, 87 * s, 14 * s, gold);

    const float w = std::min(gui.width - 36 * s, 780 * s);
    const float y = gui.height - 116 * s;
    gui.panel({18 * s, y, w, 98 * s}, color_t(0.02f, 0.055f, 0.049f, 0.92f), 10 * s, edge, s);
    gui.fill({18 * s, y + 20 * s, 3 * s, 56 * s}, gold);
    gui.label(status_age < 4.0f ? "WORKSHOP UPDATE" : "FIELD NOTES", 36 * s, y + 14 * s, 13 * s, gold);
    gui.paragraph(status, {36 * s, y + 39 * s, w - 38 * s, 48 * s}, 17 * s, cream, 2);

    if (discovery_age < 5.0f && !discovery_title.empty()) {
        const float saved_opacity = gui.opacity;
        const float show = reduced_motion ? 1.0f : gl::ease_out(discovery_age / 0.25f) *
                           std::clamp((5.0f - discovery_age) / 0.6f, 0.0f, 1.0f);
        gui.opacity *= show;
        const float x = (gui.width - 420 * s) * 0.5f;
        gui.panel({x, 136 * s, 420 * s, 92 * s}, ink, 10 * s, gold, s);
        gui.label("NEW FORMULA DISCOVERED", x + 22 * s, 151 * s, 14 * s, gold);
        gui.fitted_label(discovery_title, {x + 22 * s, 181 * s, 376 * s, 30 * s}, 22 * s, cream);
        gui.opacity = saved_opacity;
    }
}

void alchemy::ui_t::paused()
{
    const float s = gui.scale;
    const auto b = frame(500, 650);
    window(b, "TAKE A BREATHER", "Return whenever you are ready.");
    const float x = b.x + 34 * s, w = b.w - 68 * s;
    if (gui.button({x, b.y + 150 * s, w, 60 * s}, "RESUME", emerald, cream, true, "pause.resume")) {
        set_screen(screen_t::game);
        return;
    }
    if (gui.button({x, b.y + 226 * s, w, 56 * s}, "RECIPE BOOK", tile, cream, true, "pause.codex")) {
        codex_return = screen_t::pause;
        set_screen(screen_t::codex);
        return;
    }
    if (gui.button({x, b.y + 298 * s, w, 56 * s}, "SETTINGS", tile, cream, true, "pause.settings")) {
        return_screen = screen_t::pause;
        set_screen(screen_t::settings);
        return;
    }
    if (gui.button({x, b.y + 410 * s, w, 56 * s}, "MAIN MENU", quiet, cream, true, "pause.menu")) {
        set_screen(screen_t::main_menu);
        return;
    }
    if (gui.button({x, b.y + 482 * s, w, 56 * s}, "LEAVE GAME", danger, cream, true, "pause.exit"))
        gui.ctx.force_close();
    gui.label("ESC  Return to the workshop", x + 4 * s, b.y + 588 * s, 15 * s, muted);
}

void alchemy::ui_t::codex()
{
    const float s = gui.scale;
    const auto b = frame(1080, 750);
    const int known = int(std::count(discovered.begin(), discovered.end(), true));
    window(b, "THE ALCHEMY CODEX", std::to_string(known) + " / " + std::to_string(recipes.size()) + " formulas discovered");
    gui.progress({b.x + 34 * s, b.y + 108 * s, 280 * s, 3 * s}, discovery_progress, gold);
    if (gui.button({b.x + b.w - 210 * s, b.y + 28 * s, 176 * s, 50 * s},
                   "CLOSE", quiet, cream, true, "codex.close")) {
        set_screen(codex_return);
        return;
    }
    for (int i = 0; i < codex_per_page; ++i) {
        const std::size_t index = std::size_t(codex_page * codex_per_page + i);
        if (index >= recipes.size()) break;
        const bool known_recipe = discovered[index];
        const auto& recipe = recipes[index];
        const float x = b.x + (34.0f + (i % 2) * 516.0f) * s;
        const float y = b.y + (137.0f + (i / 2) * 163.0f) * s;
        color_t accent = known_recipe ? recipe_color(recipe.kind) : muted;
        gui.panel({x, y, 496 * s, 147 * s}, panel, 8 * s, edge, s);
        gui.fill({x, y + 18 * s, 2 * s, 40 * s}, accent);
        bottle(gui, x + 14 * s, y + 19 * s, 48 * s, accent, known_recipe);
        const std::string number = index + 1 < 10 ? "0" + std::to_string(index + 1) : std::to_string(index + 1);
        gui.label(number, x + 437 * s, y + 16 * s, 14 * s, gold);
        gui.fitted_label(known_recipe ? std::string(recipe.name) : "Uncharted formula",
                         {x + 76 * s, y + 20 * s, 350 * s, 30 * s}, 21 * s, known_recipe ? cream : muted);
        gui.label(known_recipe ? std::string(category_name(recipe.kind)) : "AWAITING DISCOVERY",
                  x + 77 * s, y + 54 * s, 12 * s, accent);
        if (known_recipe) {
            gui.paragraph(std::string(recipe.description), {x + 18 * s, y + 85 * s, 460 * s, 45 * s}, 16 * s, muted, 2);
        } else {
            gui.paragraph("Experiment with essences to reveal this entry.",
                          {x + 18 * s, y + 88 * s, 450 * s, 42 * s}, 16 * s, muted, 2);
        }
    }
    if (gui.button({b.x + 34 * s, b.y + 664 * s, 205 * s, 52 * s}, "PREVIOUS", quiet, cream,
                   codex_page > 0, "codex.previous")) --codex_page;
    if (gui.button({b.x + b.w - 239 * s, b.y + 664 * s, 205 * s, 52 * s}, "NEXT", quiet, cream,
                   codex_page + 1 < codex_pages, "codex.next")) ++codex_page;
    gui.label("PAGE " + std::to_string(codex_page + 1) + " / " + std::to_string(codex_pages),
              b.x + 475 * s, b.y + 667 * s, 16 * s, gold);
    gui.label("LEFT / RIGHT  Turn page", b.x + 412 * s, b.y + 696 * s, 13 * s, muted);
}

void alchemy::ui_t::load_discoveries()
{
    std::ifstream file(discoveries_path());
    std::string id;
    while (file >> id) {
        for (std::size_t i = 0; i < recipes.size(); ++i)
            if (recipes[i].id == id) discovered[i] = true;
    }
}

void alchemy::ui_t::save_discoveries() const
{
    std::ofstream file(discoveries_path());
    if (!file) return;
    for (std::size_t i = 0; i < recipes.size(); ++i)
        if (discovered[i]) file << recipes[i].id << '\n';
}
