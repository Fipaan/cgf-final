/*
 * scene/alchemy.hpp -
 *     The alchemy gameplay inside the 3D world. Potion bottles are ingredients (each one is tinted with the
 *     colour of its ingredient), whatever falls into the cauldron is mixed with E, and the recipe found by
 *     `alchemy::find_recipe` decides what happens to the player and to the loose objects of the room.
 */
#pragma once

#include <functional>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <vector>

#include <scene/scene.hpp>
#include <scene/player.hpp>
#include <ui/alchemy.hpp>

namespace audio { struct sfx_t; }

// the hidden vial: not one of the six ingredients, it is not in the recipe book. Whatever mix it goes into,
// the cauldron undoes everything (`brew_t::hard_reset`)
constexpr const char *VIAL_NAME = "reset_vial";
constexpr const char *VIAL_TITLE = "Quicksilver Vial";
// a lit candle that lies on its side on the rug this long sets it alight
constexpr gl::f32 RUG_BURN_SECONDS = 5.0f;

/* Usage:
```cpp
brew_t brew(scene, player, alchemy_ui);
trigger_t *bowl = room::build_lab(scene);
player.spawn(spawn_point);
brew.setup(bowl, spawn_point);

// every frame, after `player.update`
brew.update(ctx.last_frame);
brew.shake_begin();
scene.draw(aspect);
brew.shake_end();
```
*/
struct brew_t {
    scene_t&       scene;
    player_t&      player;
    alchemy::ui_t& ui;
    audio::sfx_t  *sfx = nullptr; // optional

    trigger_t *bowl = nullptr; // over the inside of the cauldron
    math::vec3_t<gl::f32> spawn;
    math::vec3_t<gl::f32> bowl_centre; // where explosions and the reverse explosion are centred

    // what the cauldron has swallowed so far (ingredient ids), and whether a candle has set it alight
    std::vector<int> mix;
    bool primed = false;

    // bottle -> ingredient id (index into `alchemy::ingredients`)
    std::map<std::string, int> ingredient_of;

    // how things were at the start: Restoration brings them back to this
    struct home_t {
        math::vec3_t<gl::f32> pos, scale, half;
        math::mat4_t orient = math::mat4_t::id();
        gl::f32 mass = 1.0f, restitution = 0.3f;
    };
    std::map<std::string, home_t> home;                              // loose objects and their parts
    std::map<std::string, std::vector<std::string>> parts_of;        // loose object -> its parts

    // what the cauldron has swallowed: switched off and invisible (not deleted), `hard_reset` brings them back
    std::set<std::string> eaten;
    // seconds that a lit candle has been lying on the rug, by candle
    std::map<std::string, gl::f32> burn_time;
    gl::f32 smolder_acc = 0.0f, glint_acc = 0.0f;

    struct timer_t {
        gl::f32 left, period, acc = 0.0f;
        int ticks = 0;
        std::function<void(int tick)>     on_tick;  // every `period` seconds
        std::function<void(gl::f32 dt)>   on_frame; // every frame
        std::function<void()>             on_end;
    };
    std::vector<timer_t> timers;

    std::mt19937 rng{1234};
    // what the particles need from the lights: the colour of the cauldron and where the flames are
    math::vec3_t<gl::f32> bowl_tint;
    std::vector<math::vec3_t<gl::f32>> flames;
    gl::f32 steam_acc = 0.0f, dust_acc = 0.0f, ember_acc = 0.0f;
    gl::f32 flash = 0.0f; // glow of the cauldron after a brew, fades out
    math::vec3_t<gl::f32> flash_color;
    gl::f32 shake_left = 0.0f, shake_total = 1.0f, shake_amp = 0.0f;
    math::vec3_t<gl::f32> shake_offset;

    brew_t(scene_t& scene, player_t& player, alchemy::ui_t& ui);

    // tints the bottles, hooks the cauldron and the bottles up, remembers where everything is
    void setup(trigger_t *bowl, math::vec3_t<gl::f32> spawn);
    void update(gl::f32 dt);
    // around `scene_t::draw`: camera shake of the Apocalypse
    void shake_begin();
    void shake_end();

    // a bottle or a candle fell into the cauldron: it is eaten, a bottle goes into the mix, a candle brews it
    void throw_in(const std::string& object);
    void eat(const std::string& name);       // out of the world until the next `hard_reset`
    void set_present(const std::string& name, bool present); // visible and solid, or neither
    bool is_candle(const std::string& name) const;
    math::vec3_t<gl::f32> bowl_surface() const;

    // E on the cauldron, or a candle. Whatever the mix is, the cauldron is empty afterwards
    void brew();
    // candle flames and the cauldron glow as scene lights, every frame
    void update_lights(gl::f32 dt);
    // steam and bubbles over the cauldron, embers over the candles, dust in the lamp light
    void update_particles(gl::f32 dt);
    void apply(const alchemy::recipe_t& recipe);
    // the hidden vial, on its shelf
    void spawn_vial();
    // everything back to how the game started: eaten things, positions, sizes, candles, the rug, the lamp, the
    // shading, the player. Recipes already found stay found
    void hard_reset();
    // the rug of the room burns when a lit candle lies on it
    void update_rug(gl::f32 dt);
    void burn_rug(object_t& rug);
    // a sound of `owner` (the object that makes it), at `at` in the world or at the listener. One-shots only play
    // while the game is running (not in the menus)
    void play(const char *name, const std::string& owner, std::optional<math::vec3_t<gl::f32>> at = std::nullopt,
              gl::f32 gain = 1.0f, gl::f32 min_gap = 0.0f);
    // a bottle hit something
    void clink(const std::string& bottle, gl::f32 impulse);
    std::string contents() const; // "Growth + Speed", empty if nothing is inside

    // effects, `factor` of the scalers is relative to the starting size
    std::vector<std::string> loose() const;
    void scale_loose(const std::function<gl::f32(const std::string& name)>& factor);
    void scatter_loose(gl::f32 half_x, gl::f32 y0, gl::f32 y1, gl::f32 half_z);
    void set_float(bool on);
    void blast(math::vec3_t<gl::f32> centre, gl::f32 radius, gl::f32 strength);
    void shake(gl::f32 seconds, gl::f32 amplitude);
    void set_restitution(gl::f32 value);
    void restore_objects();
    void restore_all();
    void reset_player();

    void add_timer(timer_t timer);
    void cancel_timers();

    gl::f32 rand(gl::f32 lo, gl::f32 hi);
};
