#include "texture.hpp"

#include <utility>

#include <core/log.hpp>
#include <core/stb/image.hpp>

gl::texture_t::~texture_t()
{
    if (id != 0) glDeleteTextures(1, &id);
}

gl::texture_t::texture_t(texture_t&& other) noexcept
    : id(std::exchange(other.id, 0)), width(other.width), height(other.height)
{
}

gl::texture_t& gl::texture_t::operator=(texture_t&& other) noexcept
{
    if (this != &other) {
        if (id != 0) glDeleteTextures(1, &id);
        id     = std::exchange(other.id, 0);
        width  = other.width;
        height = other.height;
    }
    return *this;
}

bool gl::texture_t::load(const char *path)
{
    assert(id == 0);

    int channels = 0;
    stbi_set_flip_vertically_on_load(1); // files are top to bottom, GL wants the bottom row first
    unsigned char *pixels = stbi_load(path, &width, &height, &channels, 4);
    stbi_set_flip_vertically_on_load(0);
    if (pixels == nullptr) {
        errorfn("{}: {}", path, stbi_failure_reason());
        return false;
    }

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(pixels);

    if (!check_gl_error("texture_t::load")) return false;
    infofn("Loaded {}: {}x{}", path, width, height);
    return true;
}

void gl::texture_t::bind(GLuint unit) const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id);
}
