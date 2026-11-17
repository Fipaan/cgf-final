/*
 * 3d/obj.hpp -
 *     Wavefront .obj models (with their .mtl): one `mesh_t` per material, normals taken from the file,
 *     so hard edges stay hard. Parsing is done by core/fast_obj.
 */
#pragma once

#include <string>
#include <vector>

#include <core/gl/first.hpp>
#include <2d/math.hpp>
#include <3d/math.hpp>
#include <3d/mesh.hpp>

// Ka, Kd, Ks, Ke and Ns of the .mtl. If there is a `map_Kd` `diffuse` is multiplied by the average colour of
// the image (textures are not drawn yet).
struct obj_material_t {
    std::string name;
    color_t ambient  = color_t(0.2f, 0.2f, 0.2f, 1.0f);
    color_t diffuse  = color_t(0.8f, 0.8f, 0.8f, 1.0f);
    color_t specular = color_t(0.0f, 0.0f, 0.0f, 1.0f);
    color_t emission = color_t(0.0f, 0.0f, 0.0f, 1.0f);
    gl::f32 shininess = 32.0f;
    std::string diffuse_map; // map_Kd as a path that opens from the working directory, empty if there is none
};

struct obj_part_t {
    mesh_t         mesh; // `vertex_normals` and `face_normals` are set
    obj_material_t material;
};

/* Usage:
```cpp
obj_model_t model;
if (!model.load("./assets/models/alchemy_pack/Chair/Chair.obj", true)) return 1;
for (const obj_part_t& part : model.parts) {
    // part.mesh, part.material
}
// or straight into the scene: scene.add_model("chair", model, vec3_t<gl::f32>(0, 0, 0));
```
*/
struct obj_model_t {
    std::vector<obj_part_t> parts;
    // of all vertices, after the axes were converted
    vec3_t<gl::f32> bounds_min, bounds_max;

    // `z_up`: the file has Z up and -Y forward (Blender's usual export), it is turned to Y up and +Z forward.
    // Objects and groups of the file are merged per material.
    bool load(const char *path, bool z_up = false);
};
