/*
 * gl/texture.hpp -
 *     2D texture from an image file (PNG, JPEG, ...): decoded by stb_image and uploaded as RGBA8 with a
 *     full mipmap chain, repeating, trilinear filtering.
 */
#pragma once

#include <core/gl/first.hpp>

namespace gl {

struct texture_t {
    GLuint id     = 0;
    int    width  = 0;
    int    height = 0;

    texture_t() = default;
    ~texture_t();
    texture_t(const texture_t&) = delete;
    texture_t& operator=(const texture_t&) = delete;
    texture_t(texture_t&& other) noexcept;
    texture_t& operator=(texture_t&& other) noexcept;

    // the first row of the file ends up at v = 1, so (0, 0) is the bottom left of the image
    bool load(const char *path);
    // makes it the 2D texture of `unit` (0, 1, ...) and leaves that unit active
    void bind(GLuint unit) const;
};

} // namespace gl
