/*
 * ui/alchemy.hpp -
 *     Screens of the Alchemy Room (main menu, settings, HUD, pause, recipe codex) built from `gl::gui_t`,
 *     and the discovered recipes storage.
 */
#pragma once

#include <array>
#include <string>

#include <gl/gui.hpp>
#include <ui/alchemy_recipes.hpp>

/* Usage:
```cpp
alchemy::ui_t alchemy_ui(gui);
if (!alchemy_ui.init()) return 1;

// every frame
alchemy_ui.update();
if (alchemy_ui.game_interactive()) {
    // ... game input ...
}
alchemy_ui.draw();
```
*/
namespace alchemy {
    enum class screen_t { main_menu, settings, controls, game, pause, codex };

    // TODO: text is too small
    struct ui_t {
        bool initialized;
        gl::gui_t& gui;

        screen_t screen        = screen_t::main_menu;
        screen_t return_screen = screen_t::main_menu; // `settings` goes back here
        screen_t codex_return  = screen_t::game;
        bool vsync     = true;
        bool limit_120 = false;
        int  codex_page = 0;
        std::array<bool, recipes.size()> discovered = {};
        std::string status = "Find ingredients and use the cauldron to brew potions.";

        ui_t(gl::gui_t& gui);

        bool init();
        void update();
        void draw();

        bool game_visible() const;
        bool game_interactive() const;
        void record_recipe(const recipe_t& recipe);
        void set_status(std::string message);

        void menu();
        void settings();
    void controls();
        void hud();
        void paused();
        void codex();

        void load_discoveries();
        void save_discoveries() const;
    };
}
