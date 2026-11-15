/*
 * scene/scene.hpp -
 *     Everything that exists in the 3D world and is displayed: named meshes, materials and objects on
 *     top of `mesh_t` / `mesh_gpu_t`, the physics world, camera and lights.
 */
#pragma once

#include <map>
#include <optional>
#include <string>

#include <core/gl/first.hpp>
#include <core/gl/shader.hpp>
#include <2d/math.hpp>
#include <3d/math.hpp>
#include <3d/mesh.hpp>
#include <3d/camera.hpp>
#include <3d/physics.hpp>

struct material_t {
    color_t ambient, diffuse, specular;
    gl::f32 shininess;
};

enum class body_kind_t {
    none,    // visual only
    dynamic, // gravity + collisions
    fixed,   // collides, never moves
};

// matches `shading_mode` in the mesh shader
enum class shading_t : int { normals = 0, gouraud = 1, phong = 2 };

// everything needed to create an object, see `scene_t::add`
struct object_desc_t {
    std::string mesh;     // key from `add_mesh`
    std::string material; // key from `add_material`
    vec3_t<gl::f32> pos;
    vec3_t<gl::f32> rot;  // euler XYZ, radians, visual only (colliders are never rotated)
    vec3_t<gl::f32> scale = vec3_t<gl::f32>(1.0f);
    body_kind_t body = body_kind_t::none;
    // collider half extents, default is mesh bounds * scale
    std::optional<vec3_t<gl::f32>> half;
    gl::f32 restitution = 0.0f;
    gl::f32 friction    = 4.0f;
};

struct object_t {
    std::string mesh, material;
    vec3_t<gl::f32> pos, rot, scale;
    bool visible = true;
    int  body    = -1; // index into `scene_t::world.bodies`, -1 = not simulated
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
    phys::world_t world;
    std::map<std::string, material_t> materials;
    std::map<std::string, object_t>   objects;

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

    // nullptr if `name` is taken or the mesh / material is unknown
    object_t *add(const std::string& name, const object_desc_t& desc);
    object_t *get(const std::string& name);
    bool      remove(const std::string& name);

    phys::body_t *body_of(const object_t& object);

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
};
