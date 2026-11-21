/*
 * scene/drag.hpp -
 *     Grab and throw dynamic objects with the crosshair. The held object is not teleported: its body is
 *     accelerated towards a point in front of the camera, so it still collides with everything and
 *     keeps its velocity when released, whatever happens after is up to the physics. Holding RMB on top of LMB turns the mouse into
 *     a trackball for the held object, camera look is frozen meanwhile.
 */
#pragma once

#include <functional>
#include <string>

#include <gl/input.hpp>
#include <scene/scene.hpp>

constexpr gl::f32 DRAG_REACH     = 6.0f;  // how far from the camera an object can be grabbed
constexpr gl::f32 DRAG_MIN_DIST  = 1.2f;  // held objects never get closer than this
constexpr gl::f32 DRAG_STIFFNESS = 12.0f; // desired speed per metre of distance to the target, 1/s
constexpr gl::f32 DRAG_MAX_SPEED = 12.0f;
constexpr gl::f32 DRAG_ACCEL     = 80.0f; // m/s^2
constexpr gl::f32 DRAG_LEASH     = 2.5f;  // let go if the object is stuck this far from the target
constexpr gl::f32 DRAG_ROTATE    = 2.0f;  // rotation per camera look, 2 = twice as fast as looking around

/* Usage:
```cpp
drag_t drag(scene);

// every frame, mouse captured
drag.input(ctx.input, player.body);
drag.update(ctx.last_frame); // before scene.update
```
*/
struct drag_t {
    scene_t& scene;
    std::string hover; // object under the crosshair
    std::string held;  // object being dragged, empty = none
    gl::f32 distance = 0.0f; // camera -> grab point
    math::vec3_t<gl::f32> offset;  // grab point -> body centre
    bool rotating = false;   // LMB + RMB, mouse movement turns the held object
    std::function<void(const std::string& name)> on_grab; // called when something got picked up

    drag_t(scene_t& scene);

    bool holding() const;

    // hover, grab on LMB press, release on LMB release, `rotating`. `ignore` is the player body
    void input(gl::input_t& input, int ignore);
    // lets go, the object keeps moving and gets some spin from its velocity
    void release();
    // drives the held body, call before `scene_t::update`
    void update(gl::input_t& input, gl::f32 dt);
};
