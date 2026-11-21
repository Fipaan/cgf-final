/*
 * core/fast_obj/obj.hpp -
 *     fast_obj (https://github.com/thisistherk/fast_obj, MIT): Wavefront .obj / .mtl reader. Positions,
 *     texture coordinates and normals come as flat arrays, indices start from 1 (0 = not present).
 *     Straight from the library, the implementation is compiled in obj.cpp.
 */
#pragma once

extern "C" {
    #include "fast_obj/fast_obj.h"
}
