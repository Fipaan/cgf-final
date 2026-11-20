/*
 * scene/alchemy.hpp -
 *     The alchemy gameplay inside the 3D world. Potion bottles are ingredients (each one is tinted with the
 *     colour of its ingredient), whatever falls into the cauldron is mixed with E, and the recipe found by
 *     `alchemy::find_recipe` decides what happens to the player and to the loose objects of the room.
 */
#pragma once

#include <functional>
#include <map>
#include <random>
#include <string>
#include <vector>

#include <scene/scene.hpp>
#include <scene/player.hpp>
#include <ui/alchemy.hpp>

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

    trigger_t *bowl = nullptr; // over the inside of the cauldron
    math::vec3_t<gl::f32> spawn;
    math::vec3_t<gl::f32> bowl_centre; // where explosions and the reverse explosion are centred

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

    struct timer_t {
        gl::f32 left, period, acc = 0.0f;
        int ticks = 0;
        std::function<void(int tick)>     on_tick;  // every `period` seconds
        std::function<void(gl::f32 dt)>   on_frame; // every frame
        std::function<void()>             on_end;
    };
    std::vector<timer_t> timers;

    std::mt19937 rng{1234};
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

    // E on the cauldron or on a bottle inside it
    void brew();
    // candle flames and the cauldron glow as scene lights, every frame
    void update_lights(gl::f32 dt);
    void apply(const alchemy::recipe_t& recipe);
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
