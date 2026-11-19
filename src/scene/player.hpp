/*
 * scene/player.hpp -
 *     Player on top of `scene_t`: an AABB body in the physics world plus the scene camera, either
 *     first-person (walks, jumps, collides) or free-fly (no body).
 */
#pragma once

#include <gl/input.hpp>
#include <scene/scene.hpp>

// player collider (AABB), eye sits `EYE_HEIGHT` above its centre
constexpr math::vec3_t<gl::f32> PLAYER_HALF(0.3f, 0.9f, 0.3f);
constexpr gl::f32 EYE_HEIGHT  = 0.7f;
constexpr gl::f32 WALK_SPEED  = 5.0f;
constexpr gl::f32 FLY_SPEED   = 10.0f;
constexpr gl::f32 SPRINT_MULT = 2.0f;
constexpr gl::f32 JUMP_SPEED  = 5.5f;
constexpr gl::f32 PLAYER_MASS = 70.0f; // pushes objects around

enum class cam_mode_t { fly, fps };

/* Usage:
```cpp
player_t player(scene);
player.spawn(vec3_t<gl::f32>(0.0f, 0.0f, 4.0f));

// every frame
player.input(ctx.input, ctx.last_frame);
scene.update(ctx.last_frame);
player.update();
```
*/
struct player_t {
    scene_t& scene;
    size_t   body = 0; // index into `scene_t::world.bodies`
    cam_mode_t mode = cam_mode_t::fps;

    player_t(scene_t& scene);

    // `feet` is the point on the floor the player stands at
    void spawn(math::vec3_t<gl::f32> feet);
    void set_mode(cam_mode_t mode);

    // mouse look (if the mouse is captured and `look`), mode switch and movement
    void input(gl::input_t& input, gl::f32 dt, bool look = true);
    // call after `scene_t::update`, puts the camera on the body
    void update();

    phys::body_t& get_body();
};
