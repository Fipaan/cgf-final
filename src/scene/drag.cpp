#include "drag.hpp"

#include <algorithm>

drag_t::drag_t(scene_t& scene) :
    scene(scene)
{}

bool drag_t::holding() const
{
    return !held.empty();
}

void drag_t::release()
{
    rotating = false;
    if (!holding()) return;
    if (object_t *o = scene.get(held)) {
        if (phys::rigid_t *r = scene.rigid_of(*o)) r->held = false;
    }
    held.clear();
}

void drag_t::input(gl::input_t& input, int ignore)
{
    const camera_t& cam = scene.cam;
    pick_t pick = scene.pick(cam.pos, cam.forward(), DRAG_REACH, ignore);
    hover = pick.movable ? pick.name : "";

    if (!holding() && input.mouse_hit(gl::mouse_left) && pick.movable) {
        phys::rigid_t *r = scene.rigid_of(*scene.get(pick.name));
        held     = pick.name;
        r->held  = true;
        distance = std::max(pick.dist, DRAG_MIN_DIST);
        offset   = r->pos - pick.point;
    }
    if (holding() && !input.mouse_held(gl::mouse_left)) release();
    rotating = holding() && input.mouse_held(gl::mouse_right);
}

void drag_t::update(gl::input_t& input, gl::f32 dt)
{
    if (!holding()) return;

    object_t *o = scene.get(held);
    phys::rigid_t *r = o ? scene.rigid_of(*o) : nullptr;
    if (r == nullptr) {
        release();
        return;
    }

    const vec3_t<gl::f32> target = scene.cam.pos + scene.cam.forward() * distance + offset;
    const vec3_t<gl::f32> delta  = target - r->pos;
    if (vec3::len(delta) > DRAG_LEASH) {
        release();
        return;
    }

    // accelerate (not set) the velocity, so the object lags behind a fast camera and can be thrown
    vec3_t<gl::f32> desired = delta * DRAG_STIFFNESS;
    if (vec3::len(desired) > DRAG_MAX_SPEED) desired = vec3::norm(desired) * DRAG_MAX_SPEED;
    vec3_t<gl::f32> dv = desired - r->vel;
    const gl::f32 max_dv = DRAG_ACCEL * dt;
    if (vec3::len(dv) > max_dv) dv = vec3::norm(dv) * max_dv;
    r->vel += dv;

    if (rotating) {
        // trackball: the side facing the camera follows the mouse
        const vec2_t<gl::f32> d = input.mouse_delta();
        const gl::f32 k = scene.cam.sensitivity * DRAG_ROTATE;
        const vec3_t<gl::f32> right = scene.cam.right();
        const vec3_t<gl::f32> up    = vec3::cross(right, scene.cam.forward());
        r->orient  = phys::axis_angle(up, d.x * k) * phys::axis_angle(right, d.y * k) * r->orient;
        r->angvel  = vec3_t<gl::f32>();
    }
}
