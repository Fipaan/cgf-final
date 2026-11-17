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
#include <2d/math.hpp>
#include <3d/math.hpp>
#include <3d/mesh.hpp>
#include <3d/obj.hpp>
#include <3d/camera.hpp>
#include <3d/physics.hpp>
#include <3d/rigid.hpp>

struct material_t {
    color_t ambient, diffuse, specular;
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
    vec3_t<gl::f32> pos;
    vec3_t<gl::f32> rot;  // euler (see `mat4_t::trs`), radians, the starting orientation
    vec3_t<gl::f32> scale = vec3_t<gl::f32>(1.0f);
    body_kind_t body = body_kind_t::none;
    // collider half extents, default is mesh bounds * scale
    std::optional<vec3_t<gl::f32>> half;
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

struct object_t {
    std::string mesh, material;
    vec3_t<gl::f32> pos, scale;
    // NOTE: for dynamic objects `pos` and `orient` follow the rigid body, for fixed ones the orientation
    //       is visual only (the collider is an axis aligned box)
    mat4_t orient = mat4_t::identity();
    bool visible = true;
    int  body    = -1; // fixed: index into `scene_t::world.bodies`
    int  rigid   = -1; // dynamic: index into `scene_t::rigids.bodies`

    // E while looking at it (needs a collider): `scene_t::interact`. `prompt` is what the UI may show.
    std::string   prompt;
    interact_fn_t on_interact;
    // the moment it starts touching something, see `scene_t::update`. Fixed objects work too.
    collision_fn_t on_collision;
};

// A box in the world that tells which dynamic objects are inside (their centre is). Good for "fell into the can".
struct trigger_t {
    vec3_t<gl::f32> pos, half;
    // only objects it returns true for count, default is every dynamic object
    std::function<bool(const std::string& name, const object_t& object)> filter;
    trigger_fn_t on_enter, on_exit;
    std::set<std::string> inside; // kept up to date by `scene_t::update`
};

// result of `scene_t::pick`, `name` is empty if the ray hit nothing that is an object
struct pick_t {
    std::string name;
    gl::f32 dist = 0.0f;
    vec3_t<gl::f32> point;
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

    camera_t  cam;
    shading_t shading = shading_t::phong;

    // light 0 is `lamp` (world coordinates, Y up), light 1 is a headlight in camera space
    cyl_t lamp {0.8f, 3.5f, 3.0f};
    vec3_t<gl::f32> headlight_eye = vec3_t<gl::f32>(0.3f, 0.3f, 0.0f);
    color_t light_ambient  = color_t(0.2f, 0.2f, 0.2f, 1.0f);
    color_t light_diffuse  = color_t(0.6f, 0.6f, 0.6f, 1.0f);
    color_t light_specular = color_t(1.0f, 1.0f, 1.0f, 1.0f);

    scene_t();

    bool init(const char *vertex_path, const char *fragment_path);

    // uploads the mesh, normals must already be computed
    bool add_mesh(const std::string& name, const mesh_t& mesh);
    bool add_material(const std::string& name, const material_t& material);

    // One visual object per material of the model, each named "<name>/<material>" (with the mesh and the
    // material added under the same name), no colliders. `pos` is where the origin of the model goes.
    // The ambient colour is the diffuse one: exported .mtl files have a grey `Ka` that would wash colours out.
    // false if one of the names is taken.
    bool add_model(const std::string& name, const obj_model_t& model, vec3_t<gl::f32> pos, gl::f32 scale = 1.0f);

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
    pick_t pick(vec3_t<gl::f32> origin, vec3_t<gl::f32> dir, gl::f32 max_dist, int ignore);
    // runs `on_interact` of the nearest object within `reach`, false if there is none
    bool interact(vec3_t<gl::f32> origin, vec3_t<gl::f32> dir, gl::f32 reach, int ignore);

    void update(gl::f32 dt);
    // camera, lights, then every visible object
    bool draw(gl::f32 aspect);

    struct mesh_entry_t {
        mesh_gpu_t gpu;
        vec3_t<gl::f32> half; // half extents of the bounds around the origin
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
