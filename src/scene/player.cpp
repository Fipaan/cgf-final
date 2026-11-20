#include "player.hpp"

using namespace math;

player_t::player_t(scene_t& scene) :
    scene(scene)
{}

phys::body_t& player_t::get_body()
{
    return scene.world.bodies[body];
}

void player_t::spawn(vec3_t<gl::f32> feet)
{
    body = scene.world.add(phys::body_t{
        .pos = feet + vec3_t<gl::f32>(0, PLAYER_HALF.y + 0.01f, 0), .half = PLAYER_HALF,
        .friction = 0.0f, // velocity is set directly by input
        .mass = PLAYER_MASS,
    });
    scene.player_body = (int)body;
    scene.cam.pos = get_body().pos + vec3_t<gl::f32>(0, EYE_HEIGHT, 0);
}

void player_t::set_mode(cam_mode_t mode)
{
    phys::body_t& p = get_body();
    if (mode == cam_mode_t::fps) {
        p.pos     = scene.cam.pos - vec3_t<gl::f32>(0, EYE_HEIGHT * scale, 0);
        p.vel     = vec3_t<gl::f32>();
        p.enabled = true;
    } else {
        p.enabled = false;
    }
    this->mode = mode;
}

void player_t::set_scale(gl::f32 new_scale)
{
    phys::body_t& p = get_body();
    const gl::f32 feet = p.pos.y - p.half.y;
    scale  = new_scale;
    p.half = PLAYER_HALF * new_scale;
    p.mass = PLAYER_MASS * new_scale * new_scale * new_scale;
    p.pos.y = feet + p.half.y + 0.01f;
    if (mode == cam_mode_t::fly) scene.cam.pos.y = p.pos.y + EYE_HEIGHT * scale; // the body follows the camera when it is back
}

void player_t::teleport(vec3_t<gl::f32> feet)
{
    phys::body_t& p = get_body();
    p.pos = feet + vec3_t<gl::f32>(0, p.half.y + 0.01f, 0);
    p.vel = vec3_t<gl::f32>();
    scene.cam.pos = p.pos + vec3_t<gl::f32>(0, EYE_HEIGHT * scale, 0);
}

void player_t::input(gl::input_t& input, gl::f32 dt, bool look)
{
    camera_t& cam = scene.cam;

    if (input.mouse_captured && look) {
        vec2_t<gl::f32> d = input.mouse_delta();
        cam.look(d.x, d.y);
    }

    if (input.key_hit(gl::kb_key_t::f2)) {
        set_mode(mode == cam_mode_t::fps ? cam_mode_t::fly : cam_mode_t::fps);
    }

    const bool fps = mode == cam_mode_t::fps;
    const vec3_t<gl::f32> up(0, 1, 0);
    const vec3_t<gl::f32> fwd   = fps ? cam.flat_forward() : cam.forward();
    const vec3_t<gl::f32> right = cam.right();

    vec3_t<gl::f32> wish;
    if (input.key_held(gl::kb_key_t::w)) wish += fwd;
    if (input.key_held(gl::kb_key_t::s)) wish -= fwd;
    if (input.key_held(gl::kb_key_t::d)) wish += right;
    if (input.key_held(gl::kb_key_t::a)) wish -= right;
    if (!fps) {
        if (input.key_held(gl::kb_key_t::space))        wish += up;
        if (input.key_held(gl::kb_key_t::left_control)) wish -= up;
    }
    if (vec3::dot(wish, wish) > 1e-6f) wish = vec3::norm(wish);

    gl::f32 speed = (fps ? WALK_SPEED : FLY_SPEED) * speed_mult;
    if (input.key_held(gl::kb_key_t::left_shift)) speed *= SPRINT_MULT;

    if (fps) {
        phys::body_t& p = get_body();
        p.vel.x = wish.x * speed;
        p.vel.z = wish.z * speed;
        if (input.key_held(gl::kb_key_t::space) && p.on_ground) p.vel.y = JUMP_SPEED * jump_mult;
    } else {
        cam.pos += wish * (speed * dt);
    }
}

void player_t::update()
{
    if (mode == cam_mode_t::fps) {
        scene.cam.pos = get_body().pos + vec3_t<gl::f32>(0, EYE_HEIGHT * scale, 0);
    }
}
