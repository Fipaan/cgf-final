/*
 * scene/scene.hpp -
 *     Everything that exists in the 3D world and is displayed: named meshes, materials and objects on
 *     top of `mesh_t` / `mesh_gpu_t`, the physics world, camera and lights.
 */
#pragma once

#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include <core/gl/first.hpp>
#include <core/gl/shader.hpp>
#include <2d/vec.hpp>
#include <2d/color.hpp>
#include <3d/math.hpp>
#include <3d/mesh.hpp>
#include <3d/obj.hpp>
#include <3d/camera.hpp>
#include <3d/physics.hpp>
#include <3d/rigid.hpp>

struct material_t {
    math::color_t ambient, diffuse, specular;
    gl::f32 shininess;
};

enum class body_kind_t {
    none,    // visual only
    dynamic, // oriented rigid body: gravity, friction, tips over, can be pushed
    fixed,   // axis aligned box, collides, never moves
};

struct scene_t;

// all callbacks get names, not pointers: objects may be added or removed from inside a callback
using interact_fn_t  = std::function<void(scene_t&, const std::string& self)>;
// `other` is the object touched, or "player"; `impulse` tells how hard (about mass * speed)
using collision_fn_t = std::function<void(scene_t&, const std::string& self, const std::string& other, gl::f32 impulse)>;
using trigger_fn_t   = std::function<void(scene_t&, const std::string& trigger, const std::string& object)>;

// matches `shading_mode` in the mesh shader
enum class shading_t : int { normals = 0, gouraud = 1, phong = 2 };

// everything needed to create an object, see `scene_t::add`
struct object_desc_t {
    std::string mesh;     // key from `add_mesh`
    std::string material; // key from `add_material`
    math::vec3_t<gl::f32> pos;
    math::vec3_t<gl::f32> rot;  // euler (see `mat4_t::trs`), radians, the starting orientation
    math::vec3_t<gl::f32> scale = math::vec3_t<gl::f32>(1.0f);
    body_kind_t body = body_kind_t::none;
    // collider half extents, default is mesh bounds * scale
    std::optional<math::vec3_t<gl::f32>> half;
    gl::f32 restitution = 0.3f;
    gl::f32 friction    = 0.6f; // Coulomb coefficient
    // default is the collider volume * `DENSITY`
    std::optional<gl::f32> mass;
    phys::shape_t shape = phys::shape_t::box; // dynamic only, a sphere uses `half.x` as radius
    // see `object_t`
    std::string    prompt;
    interact_fn_t  on_interact;
    collision_fn_t on_collision;
};

// kg/m^3 of dynamic objects without an explicit mass
constexpr gl::f32 DENSITY = 10.0f;

// A model placed in the world: see `scene_t::add_model` and `scene_t::place_model`
struct model_desc_t {
    math::vec3_t<gl::f32> pos;        // where the origin of the model goes (the pack has it at the middle of the base)
    gl::f32 yaw   = 0.0f;       // radians around Y, the model turns around its origin
    gl::f32 scale = 1.0f;
    // the collider is the box around the whole model: an axis aligned one for fixed, an oriented one for dynamic
    body_kind_t body = body_kind_t::none;
    gl::f32 restitution = 0.3f;
    gl::f32 friction    = 0.6f;
    std::optional<gl::f32> mass; // default is the volume of the box * `DENSITY`
    // go to the first part of the model, that is the one with the collider
    std::string    prompt;
    interact_fn_t  on_interact;
    collision_fn_t on_collision;
};

struct object_t {
    std::string mesh, material;
    math::vec3_t<gl::f32> pos, scale;
    // NOTE: for dynamic objects `pos` and `orient` follow the rigid body, for fixed ones the orientation
    //       is visual only (the collider is an axis aligned box)
    math::mat4_t orient = math::mat4_t::id();
    bool visible = true;
    int  body    = -1; // fixed: index into `scene_t::world.bodies`
    int  rigid   = -1; // dynamic: index into `scene_t::rigids.bodies`
    // the part of a placed model: it only has a look, position and orientation are the parent's
    std::string parent;

    // E while looking at it (needs a collider): `scene_t::interact`. `prompt` is what the UI may show.
    std::string   prompt;
    interact_fn_t on_interact;
    // the moment it starts touching something, see `scene_t::update`. Fixed objects work too.
    collision_fn_t on_collision;
};

// A box in the world that tells which dynamic objects are inside (their centre is). Good for "fell into the can".
struct trigger_t {
    math::vec3_t<gl::f32> pos, half;
    // only objects it returns true for count, default is every dynamic object
    std::function<bool(const std::string& name, const object_t& object)> filter;
    trigger_fn_t on_enter, on_exit;
    std::set<std::string> inside; // kept up to date by `scene_t::update`
};

// a small coloured light (a candle, the glow of the cauldron), see `scene_t::lights`
struct point_light_t {
    math::vec3_t<gl::f32> pos;   // world
    math::vec3_t<gl::f32> color; // colour * intensity, may be above 1
    gl::f32 radius = 3.0f;       // reaches this far, fades smoothly
};
constexpr int MAX_POINT_LIGHTS = 8; // the nearest ones to the camera are used

// result of `scene_t::pick`, `name` is empty if the ray hit nothing that is an object
struct pick_t {
    std::string name;
    gl::f32 dist = 0.0f;
    math::vec3_t<gl::f32> point;
    bool movable = false; // a dynamic object, not a wall or a board
};

/* Usage:
```cpp
scene_t scene;
if (!scene.init("shaders/mesh.vert", "shaders/mesh.frag")) return 1;
scene.add_mesh("cube", mesh_t::cube());
scene.add_material("wood", {...});

scene.add("crate", {.mesh = "cube", .material = "wood", .pos = {0, 3, 0}, .scale = vec3_t<gl::f32>(0.5f),
                    .body = body_kind_t::dynamic});
scene.get("crate")->material = "gold";
scene.remove("crate");

// every frame
scene.update(dt);
scene.draw(aspect);
```
*/
struct scene_t {
    bool initialized;

    gl::shader_t shader; // needs `view`, `proj`, `light*`, `shading_mode`, `model` and `material_*` uniforms
    phys::world_t       world;  // static boxes and the player
    phys::rigid_world_t rigids; // everything that moves
    int player_body = -1;       // index into `world.bodies`, rigid bodies are pushed by it
    gl::f32 _step_acc = 0.0f;
    std::map<std::string, material_t> materials;
    std::map<std::string, object_t>   objects;
    std::map<std::string, trigger_t>  triggers;

    struct model_t {
        struct part_t { std::string mesh, material; };
        std::vector<part_t> parts;
        math::vec3_t<gl::f32> center, half; // bounds of the model in its own space, before recentring
    };
    std::map<std::string, model_t> models;

    math::camera_t  cam;
    shading_t shading = shading_t::phong;

    // light 0 is `lamp` (world coordinates, Y up), light 1 is a headlight in camera space
    math::cyl_t lamp {0.8f, 3.5f, 3.0f};
    math::vec3_t<gl::f32> headlight_eye = math::vec3_t<gl::f32>(0.3f, 0.3f, 0.0f);
    math::color_t light_ambient  = math::color_t(0.2f, 0.2f, 0.2f, 1.0f);
    math::color_t light_diffuse  = math::color_t(0.6f, 0.6f, 0.6f, 1.0f);
    math::color_t light_specular = math::color_t(1.0f, 1.0f, 1.0f, 1.0f);
    std::map<std::string, point_light_t> lights; // owned by whoever adds them, in world coordinates

    // look of the whole picture
    gl::f32 time         = 0.0f; // seconds since the start, the lamp flickers with it
    gl::f32 exposure     = 1.35f;
    gl::f32 fog_density  = 0.02f;
    math::vec3_t<gl::f32> fog_color = math::vec3_t<gl::f32>(0.05f, 0.043f, 0.067f);

    scene_t();

    bool init(const char *vertex_path, const char *fragment_path);

    // uploads the mesh, normals must already be computed
    bool add_mesh(const std::string& name, const math::mesh_t& mesh);
    bool add_material(const std::string& name, const material_t& material);

    // Registers a model under `key`: one mesh and one material per .mtl material, named "<key>/<material>",
    // recentred on the middle of its bounds. The ambient colour is diffuse + emission (the .mtl `Ka` is a
    // flat grey that would wash colours out; emission is faked through the ambient light, there is no emissive
    // term in the shader). Placing it is `place_model`, as many times as needed.
    bool add_model(const std::string& key, const math::obj_model_t& model);

    // One object per part of the model `key`: the first one is called `name` and has the collider, the
    // others are "<name>/<material>" and follow it. Returns the first one, nullptr if `key` is not
    // registered or the names are taken.
    object_t *place_model(const std::string& key, const std::string& name, const model_desc_t& desc);

    // nullptr if `name` is taken or the mesh / material is unknown
    object_t *add(const std::string& name, const object_desc_t& desc);
    object_t *get(const std::string& name);
    bool      remove(const std::string& name);

    phys::body_t  *body_of(const object_t& object);
    phys::rigid_t *rigid_of(const object_t& object);

    trigger_t *add_trigger(const std::string& name, const trigger_t& trigger);
    bool       remove_trigger(const std::string& name);

    // nearest object with a collider along the ray, `ignore` (an index into `world.bodies`) is skipped.
    // Walls and boards hide what is behind them
    pick_t pick(math::vec3_t<gl::f32> origin, math::vec3_t<gl::f32> dir, gl::f32 max_dist, int ignore);
    // runs `on_interact` of the nearest object within `reach`, false if there is none
    bool interact(math::vec3_t<gl::f32> origin, math::vec3_t<gl::f32> dir, gl::f32 reach, int ignore);

    void update(gl::f32 dt);
    // camera, lights, then every visible object
    bool draw(gl::f32 aspect);

    struct mesh_entry_t {
        gl::mesh_gpu_t gpu;
        math::vec3_t<gl::f32> half; // half extents of the bounds around the origin
    };
    std::map<std::string, mesh_entry_t> meshes;

    // used internally, not really meant to use outside
    bool _draw_objects();
    struct touch_event_t {
        int a; phys::touch_t::kind_t kind; int b; gl::f32 impulse;
    };
    // (rigid, kind, other) -> steps since it was last touching, a resting contact flickers
    std::map<std::tuple<int, int, int>, int> _touching;
    std::vector<touch_event_t> _events;
    void _collect_touches();
    void _dispatch_events();
};
