#include "room.hpp"

#include <cmath>
#include <format>

const std::vector<std::string> room::prop_materials = {"red plastic", "gold metal", "matte clay", "glossy blue ceramic"};

gl::f32 room::shelf_t::width() const
{
    return cols * cell + (cols + 1) * board;
}

gl::f32 room::shelf_t::height() const
{
    return rows * cell + (rows + 1) * board;
}

vec3_t<gl::f32> room::shelf_t::to_world(gl::f32 lx, gl::f32 ly, gl::f32 lz) const
{
    vec3_t<gl::f32> right(facing.z, 0.0f, -facing.x);
    return origin + right * lx + vec3_t<gl::f32>(0.0f, ly, 0.0f) + facing * lz;
}

vec3_t<gl::f32> room::shelf_t::to_world_half(gl::f32 hx, gl::f32 hy, gl::f32 hz) const
{
    return std::fabs(facing.x) > 0.5f ? vec3_t<gl::f32>(hz, hy, hx) : vec3_t<gl::f32>(hx, hy, hz);
}

vec3_t<gl::f32> room::shelf_t::cell_floor(int col, int row) const
{
    gl::f32 lx = -width() / 2.0f + board + cell / 2.0f + col * (cell + board);
    gl::f32 ly = board + row * (cell + board);
    return to_world(lx, ly, depth / 2.0f);
}

void room::add_materials(scene_t& scene)
{
    // from the assignment
    scene.add_material("red plastic", material_t{
        .ambient   = color_t(0.6f, 0.2f, 0.2f, 1.0f),
        .diffuse   = color_t(0.9f, 0.1f, 0.1f, 1.0f),
        .specular  = color_t(0.8f, 0.8f, 0.8f, 1.0f),
        .shininess = 80.0f,
    });
    // tinted specular, broad highlight
    scene.add_material("gold metal", material_t{
        .ambient   = color_t(0.25f, 0.20f, 0.07f, 1.0f),
        .diffuse   = color_t(0.75f, 0.61f, 0.23f, 1.0f),
        .specular  = color_t(0.63f, 0.56f, 0.37f, 1.0f),
        .shininess = 51.0f,
    });
    // almost no specular
    scene.add_material("matte clay", material_t{
        .ambient   = color_t(0.25f, 0.20f, 0.15f, 1.0f),
        .diffuse   = color_t(0.65f, 0.50f, 0.35f, 1.0f),
        .specular  = color_t(0.05f, 0.05f, 0.05f, 1.0f),
        .shininess = 4.0f,
    });
    // tiny, bright white highlight
    scene.add_material("glossy blue ceramic", material_t{
        .ambient   = color_t(0.05f, 0.10f, 0.30f, 1.0f),
        .diffuse   = color_t(0.10f, 0.25f, 0.85f, 1.0f),
        .specular  = color_t(1.00f, 1.00f, 1.00f, 1.0f),
        .shininess = 200.0f,
    });

    // room
    scene.add_material("wall", material_t{
        .ambient   = color_t(0.30f, 0.29f, 0.26f, 1.0f),
        .diffuse   = color_t(0.72f, 0.70f, 0.64f, 1.0f),
        .specular  = color_t(0.02f, 0.02f, 0.02f, 1.0f),
        .shininess = 4.0f,
    });
    scene.add_material("floor", material_t{
        .ambient   = color_t(0.18f, 0.12f, 0.07f, 1.0f),
        .diffuse   = color_t(0.45f, 0.30f, 0.17f, 1.0f),
        .specular  = color_t(0.15f, 0.15f, 0.15f, 1.0f),
        .shininess = 20.0f,
    });
    scene.add_material("wood", material_t{
        .ambient   = color_t(0.22f, 0.14f, 0.07f, 1.0f),
        .diffuse   = color_t(0.62f, 0.40f, 0.20f, 1.0f),
        .specular  = color_t(0.10f, 0.10f, 0.10f, 1.0f),
        .shininess = 10.0f,
    });
    scene.add_material("plywood", material_t{
        .ambient   = color_t(0.20f, 0.17f, 0.12f, 1.0f),
        .diffuse   = color_t(0.50f, 0.42f, 0.30f, 1.0f),
        .specular  = color_t(0.03f, 0.03f, 0.03f, 1.0f),
        .shininess = 4.0f,
    });
}

void room::add_box(scene_t& scene, const std::string& name, const char *material, vec3_t<gl::f32> pos, vec3_t<gl::f32> half)
{
    scene.add(name, {
        .mesh = "cube", .material = material, .pos = pos, .scale = half,
        .body = body_kind_t::fixed,
    });
}

void room::build_room(scene_t& scene)
{
    const gl::f32 cy = ROOM_H / 2.0f;
    const gl::f32 hx = ROOM_X + WALL_T, hz = ROOM_Z + WALL_T;
    const gl::f32 hy = ROOM_H / 2.0f + WALL_T;

    add_box(scene, "room/floor",   "floor", {0, -WALL_T, 0},           {hx, WALL_T, hz});
    add_box(scene, "room/ceiling", "wall",  {0, ROOM_H + WALL_T, 0},   {hx, WALL_T, hz});
    add_box(scene, "room/wall_n",  "wall",  {0, cy, -ROOM_Z - WALL_T}, {hx, hy, WALL_T});
    add_box(scene, "room/wall_s",  "wall",  {0, cy,  ROOM_Z + WALL_T}, {hx, hy, WALL_T});
    add_box(scene, "room/wall_w",  "wall",  {-ROOM_X - WALL_T, cy, 0}, {WALL_T, hy, hz});
    add_box(scene, "room/wall_e",  "wall",  { ROOM_X + WALL_T, cy, 0}, {WALL_T, hy, hz});
}

room::shelf_t room::add_shelf(scene_t& scene, const std::string& name, vec3_t<gl::f32> origin, vec3_t<gl::f32> facing, int cols, int rows)
{
    shelf_t s{.origin = origin, .facing = facing, .cols = cols, .rows = rows};
    const gl::f32 W = s.width(), H = s.height();
    const gl::f32 hb = s.board / 2.0f, hd = s.depth / 2.0f;

    for (int i = 0; i <= rows; ++i) {
        gl::f32 ly = hb + i * (s.cell + s.board);
        add_box(scene, std::format("{}/board{}", name, i), "wood", s.to_world(0, ly, hd), s.to_world_half(W / 2, hb, hd));
    }
    for (int j = 0; j <= cols; ++j) {
        gl::f32 lx = -W / 2 + hb + j * (s.cell + s.board);
        add_box(scene, std::format("{}/side{}", name, j), "wood", s.to_world(lx, H / 2, hd), s.to_world_half(hb, H / 2, hd));
    }
    add_box(scene, name + "/back", "plywood", s.to_world(0, H / 2, 0.01f), s.to_world_half(W / 2, H / 2, 0.01f));
    return s;
}

void room::fill_shelf(scene_t& scene, const std::string& name, const shelf_t& s)
{
    const gl::f32 r = 0.2f;

    for (int row = 0; row < s.rows; ++row) {
        for (int col = 0; col < s.cols; ++col) {
            int k = col + row * s.cols;
            if (k % 3 == 2) continue; // leave some cells empty

            vec3_t<gl::f32> pos = s.cell_floor(col, row) + vec3_t<gl::f32>(0.0f, r + 0.01f, 0.0f);
            scene.add(std::format("{}/item{}_{}", name, col, row), {
                .mesh     = k % 3 == 0 ? "cube" : "sphere",
                .material = prop_materials[k % prop_materials.size()],
                .pos      = pos,
                .scale    = vec3_t<gl::f32>(r),
                .body     = body_kind_t::dynamic,
            });
        }
    }
}

void room::build_shelves(scene_t& scene)
{
    struct spec_t { const char *name; vec3_t<gl::f32> origin, facing; int cols, rows; };
    const spec_t specs[] = {
        {"shelf_a", {-5.5f, 0.0f, -ROOM_Z}, { 0, 0,  1}, 3, 3}, // back wall
        {"shelf_b", { 5.0f, 0.0f, -ROOM_Z}, { 0, 0,  1}, 4, 3}, // back wall
        {"shelf_c", {-ROOM_X, 0.0f,  0.0f}, { 1, 0,  0}, 5, 2}, // west wall
        {"shelf_d", { ROOM_X, 0.0f,  1.0f}, {-1, 0,  0}, 4, 4}, // east wall
    };
    for (const spec_t& sp : specs) {
        shelf_t s = add_shelf(scene, sp.name, sp.origin, sp.facing, sp.cols, sp.rows);
        fill_shelf(scene, sp.name, s);
    }
}

void room::build_props(scene_t& scene)
{
    scene.add("teapot", {
        .mesh = "model", .material = prop_materials[0],
        .pos = {0, 2.5f, 0}, .scale = vec3_t<gl::f32>(1.0f),
        .body = body_kind_t::dynamic,
    });
}
