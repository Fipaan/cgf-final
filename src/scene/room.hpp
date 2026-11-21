/*
 * scene/room.hpp -
 *     The Alchemy Room content: materials, walls, cube shelves and props, all built from `scene_t` objects.
 */
#pragma once

#include <string>
#include <vector>

#include <scene/scene.hpp>

// room interior: x in [-ROOM_X, ROOM_X], z in [-ROOM_Z, ROOM_Z], y in [0, ROOM_H]
constexpr gl::f32 ROOM_X = 8.0f;
constexpr gl::f32 ROOM_Z = 6.0f;
constexpr gl::f32 ROOM_H = 4.0f;
constexpr gl::f32 WALL_T = 0.25f; // half thickness

// height of the table top, and of the board of the wall shelf above its origin (both are measured on the models)
constexpr gl::f32 TABLE_TOP = 0.81f;
constexpr gl::f32 SHELF_TOP = 0.43f;

// how much bigger than the model the cauldron is
constexpr gl::f32 CAULDRON_SCALE = 1.5f;

namespace room {
    // Cube shelf unit: a grid of open cells made of boards.
    struct shelf_t {
        math::vec3_t<gl::f32> origin; // centre of the back side, on the floor
        math::vec3_t<gl::f32> facing; // unit axis the open side looks at, (±1,0,0) or (0,0,±1)
        int         cols = 1, rows = 1;
        gl::f32     cell  = 0.9f;   // inner cell size
        gl::f32     board = 0.06f;  // board thickness
        gl::f32     depth = 0.45f;

        gl::f32 width()  const;
        gl::f32 height() const;
        // local (x right, y up, z out of the back) -> world
        math::vec3_t<gl::f32> to_world(gl::f32 lx, gl::f32 ly, gl::f32 lz) const;
        // local half extents -> world half extents
        math::vec3_t<gl::f32> to_world_half(gl::f32 hx, gl::f32 hy, gl::f32 hz) const;
        // middle of the floor of a cell
        math::vec3_t<gl::f32> cell_floor(int col, int row) const;
    };

    // models the lab is made of, registered in the scene (`scene_t::add_model`) before `build_lab`
    extern const std::vector<std::string> model_names;
    extern const std::vector<std::string> potion_models;

    // An open box (a can, a bin) made of fixed walls with a trigger of the same name over its inside, so
    // `scene.triggers[name]` tells what fell in. `inner_half` is the inner half size (x, z), `height` the inner height.
    // `material` null = the walls are not drawn, only their colliders are there
    trigger_t *add_can(scene_t& scene, const std::string& name, math::vec3_t<gl::f32> floor_centre, math::vec2_t<gl::f32> inner_half, gl::f32 height, const char *material = "steel");

    // needs the mesh "cube" in the scene
    void add_materials(scene_t& scene);
    // static box, `half` is both the cube scale and the collider. No `material`: just the collider
    void add_box(scene_t& scene, const std::string& name, const char *material, math::vec3_t<gl::f32> pos, math::vec3_t<gl::f32> half);
    void build_room(scene_t& scene);
    shelf_t add_shelf(scene_t& scene, const std::string& name, math::vec3_t<gl::f32> origin, math::vec3_t<gl::f32> facing, int cols, int rows);
    // a few small objects in the cells, they are dynamic and settle on the boards
    void fill_shelf(scene_t& scene, const std::string& name, const shelf_t& shelf);
    void build_shelves(scene_t& scene);
    object_t *place_model(scene_t& scene, const std::string& model, const std::string& name, math::vec3_t<gl::f32> pos, gl::f32 yaw, body_kind_t body);
    void build_beams(scene_t& scene);
    // the cauldron in the middle: returns the trigger over the inside of the bowl
    trigger_t *build_cauldron(scene_t& scene);
    void build_furniture(scene_t& scene);
    void apply_lighting(scene_t& scene);

    // the whole alchemy lab: shell, cauldron in the middle, tables, chairs, door, shelves with potions.
    // Needs the mesh "cube" and all `model_names` registered. Returns the cauldron trigger.
    trigger_t *build_lab(scene_t& scene);
}
