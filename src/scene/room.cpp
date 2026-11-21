#include "room.hpp"

#include <cmath>
#include <format>

using namespace math;

const std::vector<std::string> room::model_names   = {"Candle", "Cauldron", "Chair", "Door", "Potion_Blue", "Potion_Green", "Potion_Purple", "Potion_Red", "Shelf", "Table"};
const std::vector<std::string> room::potion_models = {"Potion_Red", "Potion_Blue", "Potion_Green", "Potion_Purple"};

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
    // placeholders, any image works: the colour of the material is multiplied with it, so keep them light
    const bool floor_image = scene.add_texture("floor", "./assets/texture/floor_planks.png");
    const bool wall_image  = scene.add_texture("wall",  "./assets/texture/wall_stone.png");

    scene.add_material("steel", material_t{
        .ambient   = color_t(0.20f, 0.22f, 0.25f, 1.0f),
        .diffuse   = color_t(0.45f, 0.50f, 0.56f, 1.0f),
        .specular  = color_t(0.70f, 0.70f, 0.75f, 1.0f),
        .shininess = 60.0f,
    });

    // room
    scene.add_material("wall", material_t{ // dark stone
        .ambient   = color_t(0.30f, 0.27f, 0.26f, 1.0f),
        .diffuse   = color_t(0.34f, 0.30f, 0.29f, 1.0f),
        .specular  = color_t(0.02f, 0.02f, 0.02f, 1.0f),
        .shininess = 4.0f,
        .texture   = wall_image ? "wall" : "",
        .tile      = 1.5f, // metres per repeat
    });
    scene.add_material("beam", material_t{ // old ceiling beams
        .ambient   = color_t(0.14f, 0.09f, 0.05f, 1.0f),
        .diffuse   = color_t(0.30f, 0.19f, 0.10f, 1.0f),
        .specular  = color_t(0.04f, 0.04f, 0.04f, 1.0f),
        .shininess = 6.0f,
    });
    scene.add_material("rug", material_t{
        .ambient   = color_t(0.30f, 0.07f, 0.07f, 1.0f),
        .diffuse   = color_t(0.50f, 0.09f, 0.10f, 1.0f),
        .specular  = color_t(0.01f, 0.01f, 0.01f, 1.0f),
        .shininess = 2.0f,
    });
    scene.add_material("floor", material_t{ // dark planks
        .ambient   = color_t(0.20f, 0.13f, 0.08f, 1.0f),
        .diffuse   = color_t(0.30f, 0.20f, 0.12f, 1.0f),
        .specular  = color_t(0.10f, 0.10f, 0.10f, 1.0f),
        .shininess = 18.0f,
        .texture   = floor_image ? "floor" : "",
        .tile      = 2.0f,
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
    object_t *o = scene.add(name, {
        .mesh = "cube", .material = material != nullptr ? material : "wall", .pos = pos, .scale = half,
        .body = body_kind_t::fixed,
    });
    if (o != nullptr && material == nullptr) o->visible = false;
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
    for (int row = 0; row < s.rows; ++row) {
        for (int col = 0; col < s.cols; ++col) {
            const int k = col + row * s.cols;
            if (k % 4 == 3) continue; // leave some cells empty

            const vec3_t<gl::f32> pos = s.cell_floor(col, row) + vec3_t<gl::f32>(0.0f, 0.005f, 0.0f);
            const std::string item = std::format("{}/item{}_{}", name, col, row);
            place_model(scene, potion_models[k % potion_models.size()], item, pos, 0.7f * k, body_kind_t::dynamic);
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

trigger_t *room::add_can(scene_t& scene, const std::string& name, vec3_t<gl::f32> floor_centre, vec2_t<gl::f32> inner_half, gl::f32 height, const char *material)
{
    const gl::f32 w  = 0.04f; // half thickness of the walls
    const gl::f32 cx = floor_centre.x, cz = floor_centre.z;
    const gl::f32 y0 = floor_centre.y;
    const gl::f32 wy = y0 + 2.0f * w + height / 2.0f; // middle of the walls
    const gl::f32 ix = inner_half.x, iz = inner_half.y;

    add_box(scene, name + "/bottom", material, {cx, y0 + w, cz}, {ix + 2 * w, w, iz + 2 * w});
    add_box(scene, name + "/wall_px", material, {cx + ix + w, wy, cz}, {w, height / 2.0f, iz + 2 * w});
    add_box(scene, name + "/wall_nx", material, {cx - ix - w, wy, cz}, {w, height / 2.0f, iz + 2 * w});
    add_box(scene, name + "/wall_pz", material, {cx, wy, cz + iz + w}, {ix, height / 2.0f, w});
    add_box(scene, name + "/wall_nz", material, {cx, wy, cz - iz - w}, {ix, height / 2.0f, w});

    trigger_t trigger;
    trigger.pos  = vec3_t<gl::f32>(cx, wy, cz);
    trigger.half = vec3_t<gl::f32>(ix, height / 2.0f, iz);
    return scene.add_trigger(name, trigger);
}

object_t *room::place_model(scene_t& scene, const std::string& model, const std::string& name, vec3_t<gl::f32> pos, gl::f32 yaw, body_kind_t body)
{
    return scene.place_model(model, name, model_desc_t{.pos = pos, .yaw = yaw, .body = body, .restitution = 0.2f});
}

void room::build_beams(scene_t& scene)
{
    // visual only
    for (int i = -1; i <= 1; ++i) {
        scene.add(std::format("beam{}", i + 1), {
            .mesh = "cube", .material = "beam", .pos = {0.0f, ROOM_H - 0.14f, i * 4.0f}, .scale = {ROOM_X, 0.14f, 0.16f},
        });
    }
    scene.add("rug", {.mesh = "cube", .material = "rug", .pos = {0.0f, 0.01f, 0.0f}, .scale = {2.3f, 0.01f, 2.3f}});
}

// The cauldron in the middle: the model itself has no collider, an open bowl does (a floor and four walls
// around the opening, measured on the model at scale 1 and multiplied by `CAULDRON_SCALE`), with a trigger
// over the inside of the bowl.
trigger_t *room::build_cauldron(scene_t& scene)
{
    constexpr gl::f32 k = CAULDRON_SCALE;
    scene.place_model("Cauldron", "cauldron", model_desc_t{.pos = {0.0f, 0.0f, 0.0f}, .scale = k});
    add_box(scene, "cauldron/base", nullptr, {0.0f, 0.115f * k, 0.0f}, {0.3f * k, 0.115f * k, 0.3f * k});
    return add_can(scene, "cauldron_bowl", {0.0f, 0.23f * k, 0.0f}, {0.3f * k, 0.3f * k}, 0.43f * k, nullptr);
}

void room::build_furniture(scene_t& scene)
{
    constexpr gl::f32 PI = 3.14159265f;
    using V = vec3_t<gl::f32>;
    const body_kind_t fixed = body_kind_t::fixed, dynamic = body_kind_t::dynamic;

    // door in the middle of the back wall
    place_model(scene, "Door", "door", V(0.0f, 0.0f, -ROOM_Z + 0.13f), 0.0f, fixed);

    // east table with chairs
    place_model(scene, "Table", "table_east", V(4.6f, 0.0f, -1.4f), 0.0f, fixed);
    place_model(scene, "Chair", "chair_east_a", V(4.6f, 0.0f, -0.3f), 0.0f, dynamic);
    place_model(scene, "Chair", "chair_east_b", V(2.9f, 0.0f, -1.4f), 1.5f * PI, dynamic);
    place_model(scene, "Potion_Red",    "table_east/red",    V(3.95f, TABLE_TOP + 0.005f, -1.65f), 0.3f, dynamic);
    place_model(scene, "Potion_Blue",   "table_east/blue",   V(4.35f, TABLE_TOP + 0.005f, -1.65f), 1.2f, dynamic);
    place_model(scene, "Potion_Green",  "table_east/green",  V(4.85f, TABLE_TOP + 0.005f, -1.65f), 2.0f, dynamic);
    place_model(scene, "Potion_Purple", "table_east/purple", V(5.25f, TABLE_TOP + 0.005f, -1.55f), 0.8f, dynamic);
    place_model(scene, "Candle",        "candle_table_east", V(3.95f, TABLE_TOP + 0.005f, -1.15f), 0.0f, dynamic);

    // west table (long side along z), one chair
    place_model(scene, "Table", "table_west", V(-4.3f, 0.0f, 0.0f), 0.5f * PI, fixed);
    place_model(scene, "Chair", "chair_west", V(-5.4f, 0.0f, 0.2f), 1.5f * PI, dynamic);
    place_model(scene, "Candle",        "candle_table_west", V(-4.3f, TABLE_TOP + 0.005f, -0.65f), 0.0f, dynamic);
    place_model(scene, "Potion_Green",  "table_west/green",  V(-4.3f, TABLE_TOP + 0.005f, -0.15f), 0.4f, dynamic);
    place_model(scene, "Potion_Red",    "table_west/red",    V(-4.2f, TABLE_TOP + 0.005f,  0.3f), 1.5f, dynamic);

    // wall shelves on both sides of the door, with potions on them
    const gl::f32 shelf_z   = -ROOM_Z + 0.19f;
    const gl::f32 shelf_top = 1.45f + SHELF_TOP;
    place_model(scene, "Shelf", "wall_shelf_left",  V(-2.3f, 1.45f, shelf_z), 0.0f, fixed);
    place_model(scene, "Shelf", "wall_shelf_right", V( 2.3f, 1.45f, shelf_z), 0.0f, fixed);
    place_model(scene, "Potion_Purple", "wall_shelf_left/purple", V(-2.7f, shelf_top + 0.005f, shelf_z), 0.2f, dynamic);
    place_model(scene, "Potion_Blue",   "wall_shelf_left/blue",   V(-2.3f, shelf_top + 0.005f, shelf_z), 1.0f, dynamic);
    place_model(scene, "Candle",        "candle_shelf_left",      V(-1.9f, shelf_top + 0.005f, shelf_z), 0.0f, dynamic);
    place_model(scene, "Potion_Green",  "wall_shelf_right/green", V( 1.9f, shelf_top + 0.005f, shelf_z), 0.6f, dynamic);
    place_model(scene, "Potion_Red",    "wall_shelf_right/red",   V( 2.4f, shelf_top + 0.005f, shelf_z), 2.4f, dynamic);
    place_model(scene, "Potion_Purple", "wall_shelf_right/purple",V( 2.8f, shelf_top + 0.005f, shelf_z), 1.4f, dynamic);

    // candles around the cauldron, on the rug
    for (int i = 0; i < 4; ++i) {
        const gl::f32 a = 0.25f * PI + 0.5f * PI * i;
        place_model(scene, "Candle", std::format("candle_floor{}", i), V(1.5f * std::cos(a), 0.0f, 1.5f * std::sin(a)), a, dynamic);
    }
}

// dark: the lamp is only a faint glow, the candles and the cauldron are what lights the room
// the shelves leave every 4th cell empty, (0, 1) of 3 x 3 is one of them
math::vec3_t<gl::f32> room::vial_position()
{
    const shelf_t shelf{.origin = {-5.5f, 0.0f, -ROOM_Z}, .facing = {0.0f, 0.0f, 1.0f}, .cols = 3, .rows = 3};
    return shelf.cell_floor(0, 1) + math::vec3_t<gl::f32>(0.0f, 0.005f, 0.0f);
}

void room::apply_lighting(scene_t& scene)
{
    scene.lamp = cyl_t{0.0f, 0.4f, 2.4f};
    scene.light_ambient  = color_t(0.025f, 0.022f, 0.032f, 1.0f);
    scene.light_diffuse  = color_t(0.14f, 0.11f, 0.09f, 1.0f);
    scene.light_specular = color_t(0.60f, 0.55f, 0.50f, 1.0f);
}

trigger_t *room::build_lab(scene_t& scene)
{
    add_materials(scene);
    build_room(scene);
    build_beams(scene);
    trigger_t *bowl = build_cauldron(scene);
    build_furniture(scene);
    build_shelves(scene);
    apply_lighting(scene);
    return bowl;
}
