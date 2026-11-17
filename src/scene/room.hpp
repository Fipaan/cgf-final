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

namespace room {
    // Cube shelf unit: a grid of open cells made of boards.
    struct shelf_t {
        vec3_t<gl::f32> origin; // centre of the back side, on the floor
        vec3_t<gl::f32> facing; // unit axis the open side looks at, (±1,0,0) or (0,0,±1)
        int         cols = 1, rows = 1;
        gl::f32     cell  = 0.9f;   // inner cell size
        gl::f32     board = 0.06f;  // board thickness
        gl::f32     depth = 0.45f;

        gl::f32 width()  const;
        gl::f32 height() const;
        // local (x right, y up, z out of the back) -> world
        vec3_t<gl::f32> to_world(gl::f32 lx, gl::f32 ly, gl::f32 lz) const;
        // local half extents -> world half extents
        vec3_t<gl::f32> to_world_half(gl::f32 hx, gl::f32 hy, gl::f32 hz) const;
        // middle of the floor of a cell
        vec3_t<gl::f32> cell_floor(int col, int row) const;
    };

    // materials from `add_materials` that dynamic props and the loaded model cycle through
    extern const std::vector<std::string> prop_materials;

    // An open box (a can, a bin) made of fixed walls with a trigger of the same name over its inside, so
    // `scene.triggers[name]` tells what fell in. `inner_half` is the inner half size (x, z), `height` the inner height.
    trigger_t *add_can(scene_t& scene, const std::string& name, vec3_t<gl::f32> floor_centre, vec2_t<gl::f32> inner_half, gl::f32 height);

    // needs meshes "cube", "sphere" and "model" in the scene
    void add_materials(scene_t& scene);
    // static box, `half` is both the cube scale and the collider
    void add_box(scene_t& scene, const std::string& name, const char *material, vec3_t<gl::f32> pos, vec3_t<gl::f32> half);
    void build_room(scene_t& scene);
    shelf_t add_shelf(scene_t& scene, const std::string& name, vec3_t<gl::f32> origin, vec3_t<gl::f32> facing, int cols, int rows);
    // a few small objects in the cells, they are dynamic and settle on the boards
    void fill_shelf(scene_t& scene, const std::string& name, const shelf_t& shelf);
    void build_shelves(scene_t& scene);
    // the loaded model ("teapot"), collider from its bounds
    void build_props(scene_t& scene);
}
