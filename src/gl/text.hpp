/*
 * gl/text.hpp -
 *     Text rendering on top of `ft::ft_face_t`. Glyphs are rasterized once into an atlas texture
 *     (one atlas per pixel size), so a whole string is a single buffer upload and a single draw call.
 *     Attribute location 3 (xyuv) must match the shader used.
 */
#pragma once

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <core/gl/shader.hpp>
#include <core/ft/ft.hpp>
#include <gl/ctx.hpp>

/* Usage:
```cpp
gl::text_t text(ctx);
if (!text.init("./assets/fonts/iosevka-regular.ttf", 0, "./shaders/text.vert", "./shaders/text.frag")) return 1;

// every frame
ctx.begin_overlay();
text.draw("Hello, OpenGL!", vec2_t<gl::f32>(10.0f, ctx.screen_size.y - 10.0f), 20.0f, color_t(1.0f, 1.0f, 1.0f, 1.0f));
ctx.end_overlay();
```
*/
namespace gl {
    struct glyph_t {
        vec2_t<f32> uv0, uv1; // atlas rectangle, 0..1, `uv0` is the top-left corner of the bitmap
        vec2_t<f32> size;     // bitmap size, pixels
        vec2_t<f32> bearing;  // bitmap_left, bitmap_top, pixels
        f32         advance;  // pixels
        bool        empty;    // nothing to draw (e.g. space)
    };

    // Glyph cache for one face at one pixel size. Glyphs are rasterized on first use and packed
    // row by row into a single GL_R8 texture.
    struct glyph_atlas_t {
        bool        initialized;
        GLuint      texture;
        vec2_t<int> size;
        vec2_t<int> pen; // next free pixel
        int         row_h;
        // out of space, `text_t` drops the cache before the next draw
        bool        full;
        FT_UInt     pixel_size;
        // pixels at `pixel_size`
        f32         ascender, descender, line_h;
        std::unordered_map<char32_t, glyph_t> glyphs;

        glyph_atlas_t();
        ~glyph_atlas_t();

        // `face` must already be set to `pixel_size`
        bool init(ft::ft_face_t& face, FT_UInt pixel_size, vec2_t<int> size);
        void clear();

        // `face` must be set to `pixel_size`, nullptr if the glyph can't be loaded or doesn't fit
        const glyph_t *get(ft::ft_face_t& face, char32_t charcode);

        glyph_atlas_t(const glyph_atlas_t&)            = delete;
        glyph_atlas_t& operator=(const glyph_atlas_t&) = delete;
        glyph_atlas_t(glyph_atlas_t&&)                 = delete;
        glyph_atlas_t& operator=(glyph_atlas_t&&)      = delete;
    };

    struct text_t {
        bool initialized;
        ctx_t& ctx;

        ft::ft_lib_t  lib;
        ft::ft_face_t face;
        shader_t      shader;

        vao_t vao;
        vbo_t vbo_xyuv;

        // key is the pixel size glyphs are rasterized at, see `pixel_size_for`
        std::map<FT_UInt, std::unique_ptr<glyph_atlas_t>> atlases;
        std::vector<f32> quads;
        vec2_t<int>      _proj_size;

        text_t(ctx_t& ctx);

        bool init(const char *font_path, FT_Long face_index, const char *vertex_path, const char *fragment_path);

        // requested height -> nearest pixel size (not smaller than the height) glyphs are cached at
        FT_UInt pixel_size_for(f32 height) const;

        // `pos` is the top-left corner of the text, y goes up, `height` is in pixels.
        // Handles `\n` and UTF-8. Enables alpha blending (see `ctx_t::begin_overlay`).
        bool draw(const std::string& text, vec2_t<f32> pos, f32 height, color_t color);

        // size of the box `draw` would fill, pixels
        vec2_t<f32> measure(const std::string& text, f32 height);

        // used internally, not really meant to use outside
        glyph_atlas_t *_atlas_for(f32 height);
        bool _activate(FT_UInt pixel_size);
    };
}
