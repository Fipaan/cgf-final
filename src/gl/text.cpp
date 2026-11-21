#include "text.hpp"

#include <algorithm>
#include <span>

#include <core/log.hpp>

using namespace math;

// NOTE: glyphs are never rasterized at an arbitrary size, otherwise every step of
//       a smoothly changing text size would create a new atlas.
constexpr FT_UInt text_pixel_sizes[] = {8, 10, 12, 14, 16, 18, 20, 24, 28, 32, 40, 48, 64, 96, 128};

gl::glyph_atlas_t::glyph_atlas_t()
{
    initialized = false;
    full        = false;
    texture     = 0;
    row_h       = 0;
    pixel_size  = 0;
    ascender = descender = line_h = 0.0f;
}

gl::glyph_atlas_t::~glyph_atlas_t()
{
    if (initialized) {
        glDeleteTextures(1, &texture);
        initialized = false;
    }
}

bool gl::glyph_atlas_t::init(ft::ft_face_t& face, FT_UInt pixel_size, vec2_t<int> size)
{
    assert(!initialized);
    this->pixel_size = pixel_size;
    this->size       = size;

    const FT_Size_Metrics& metrics = face.face->size->metrics;
    ascender  = (f32)(metrics.ascender  >> 6);
    descender = (f32)(metrics.descender >> 6);
    line_h    = (f32)(metrics.height    >> 6);

    std::vector<uint8_t> zeros((std::size_t)size.x * size.y, 0);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, size.x, size.y, 0, GL_RED, GL_UNSIGNED_BYTE, zeros.data());
    initialized = true;

    return check_gl_error("glyph atlas init");
}

void gl::glyph_atlas_t::clear()
{
    glyphs.clear();
    pen   = vec2_t<int>(0, 0);
    row_h = 0;
    full  = false;
}

const gl::glyph_t *gl::glyph_atlas_t::get(ft::ft_face_t& face, char32_t charcode)
{
    assert(initialized);

    auto it = glyphs.find(charcode);
    if (it != glyphs.end()) return &it->second;

    if (!face.load_char(charcode, FT_LOAD_RENDER)) return nullptr;
    FT_GlyphSlot slot = face.face->glyph;
    FT_Bitmap   *bm   = &slot->bitmap;

    glyph_t glyph;
    glyph.size    = vec2_t<f32>((f32)bm->width, (f32)bm->rows);
    glyph.bearing = vec2_t<f32>((f32)slot->bitmap_left, (f32)slot->bitmap_top);
    glyph.advance = (f32)(slot->advance.x >> 6);
    glyph.empty   = bm->width == 0 || bm->rows == 0;

    if (!glyph.empty) {
        constexpr int padding = 1; // keeps linear filtering from pulling in the neighbour glyph
        const int w = (int)bm->width;
        const int h = (int)bm->rows;

        if (pen.x + w + padding > size.x) {
            pen.x  = 0;
            pen.y += row_h + padding;
            row_h  = 0;
        }
        if (w + padding > size.x || pen.y + h + padding > size.y) {
            if (!full) errorfn("Glyph atlas ({}px) is full, it will be rebuilt", pixel_size);
            full = true;
            return nullptr;
        }

        glBindTexture(GL_TEXTURE_2D, texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, bm->pitch);
        glTexSubImage2D(GL_TEXTURE_2D, 0, pen.x, pen.y, w, h, GL_RED, GL_UNSIGNED_BYTE, bm->buffer);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

        glyph.uv0 = vec2_t<f32>((f32)pen.x / size.x,       (f32)pen.y / size.y);
        glyph.uv1 = vec2_t<f32>((f32)(pen.x + w) / size.x, (f32)(pen.y + h) / size.y);

        pen.x += w + padding;
        row_h  = std::max(row_h, h);
    }

    return &glyphs.emplace(charcode, glyph).first->second;
}

gl::text_t::text_t(ctx_t& ctx) :
    ctx(ctx),
    face(lib)
{
    initialized = false;
}

bool gl::text_t::init(const char *font_path, FT_Long face_index, const char *vertex_path, const char *fragment_path)
{
    assert(ctx.initialized);
    assert(!initialized);

    if (!lib.init()) return false;
    if (!face.init(font_path, face_index)) return false;
    if (!shader.from_files(vertex_path, fragment_path)) return false;

    vao.gen_array();
    if (!vao.bind()) return false;
    vbo_xyuv.gen_buffer();
    if (!vbo_xyuv.add_vad(vad_t(3, vad_cpv_t::comp_4, 4, 0))) return false;

    initialized = true;
    return true;
}

FT_UInt gl::text_t::pixel_size_for(f32 height) const
{
    for (FT_UInt pixel_size : text_pixel_sizes) {
        if ((f32)pixel_size >= height) return pixel_size;
    }
    return text_pixel_sizes[std::size(text_pixel_sizes) - 1];
}

bool gl::text_t::_activate(FT_UInt pixel_size)
{
    if (face.base_size.y == pixel_size) return true;
    return face.set_pixel_sizes(vec2_t<FT_UInt>(pixel_size), ctx.DPI);
}

gl::glyph_atlas_t *gl::text_t::_atlas_for(f32 height)
{
    const FT_UInt pixel_size = pixel_size_for(height);

    auto it = atlases.find(pixel_size);
    if (it != atlases.end()) return it->second.get();

    if (!_activate(pixel_size)) return nullptr;
    auto atlas = std::make_unique<glyph_atlas_t>();
    if (!atlas->init(face, pixel_size, pixel_size <= 48 ? vec2_t<int>(512) : vec2_t<int>(1024))) return nullptr;

    return atlases.emplace(pixel_size, std::move(atlas)).first->second.get();
}

bool gl::text_t::draw(const std::string& text, vec2_t<f32> pos, f32 height, color_t color)
{
    assert(initialized);

    glyph_atlas_t *atlas = _atlas_for(height);
    if (atlas == nullptr) return false;
    if (atlas->full) atlas->clear();
    if (!_activate(atlas->pixel_size)) return false;

    const f32 scale    = height / (f32)atlas->pixel_size;
    const f32 ascender = atlas->ascender * scale;
    const f32 line_h   = atlas->line_h   * scale;

    f32 pen_x = pos.x;
    f32 pen_y = pos.y - ascender;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlas->texture);

    quads.clear();
    for (size_t i = 0; i < text.length();) {
        char32_t c = ft::utf8_next(text, i);

        if (c == '\n') {
            pen_x  = pos.x;
            pen_y -= line_h;
            continue;
        }

        const glyph_t *glyph = atlas->get(face, c);
        if (glyph == nullptr) continue;

        // space has an empty bitmap
        if (!glyph->empty) {
            const f32 x0 = pen_x + glyph->bearing.x * scale;
            const f32 x1 = x0 + glyph->size.x * scale;
            const f32 yt = pen_y + glyph->bearing.y * scale;
            const f32 yb = yt - glyph->size.y * scale;
            const vec2_t<f32>& a = glyph->uv0;
            const vec2_t<f32>& b = glyph->uv1;

            quads.insert(quads.end(), {
                x0, yt, a.x, a.y,
                x1, yt, b.x, a.y,
                x1, yb, b.x, b.y,
                x0, yt, a.x, a.y,
                x1, yb, b.x, b.y,
                x0, yb, a.x, b.y,
            });
        }

        pen_x += glyph->advance * scale;
    }
    if (quads.empty()) return true;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shader.use();
    if (_proj_size.x != ctx.screen_size.x || _proj_size.y != ctx.screen_size.y) {
        _proj_size = ctx.screen_size;
        const vec2_t<f32> proj_scale = 2.0f / _proj_size;
        const f32 proj[16] = {
            proj_scale.x,            0, 0, 0,
                       0, proj_scale.y, 0, 0,
                       0,            0, -1, 0,
                      -1,           -1, 0, 1,
        };
        shader.uniform(m_type_t::m4x4, "proj", 1, gl_bool::FALSE, proj);
        shader.uniform("glyph", (GLint)0);
    }
    shader.uniform("color", color.r, color.g, color.b);

    if (!vao.bind()) return false;
    if (!vbo_xyuv.init_buffer(std::span<f32>(quads), dsu_t::stream_draw)) return false;
    glDrawArrays(GL_TRIANGLES, 0, quads.size() / 4);

    return true;
}

vec2_t<gl::f32> gl::text_t::measure(const std::string& text, f32 height)
{
    assert(initialized);

    glyph_atlas_t *atlas = _atlas_for(height);
    if (atlas == nullptr) return vec2_t<f32>(0.0f, 0.0f);
    if (atlas->full) atlas->clear();
    if (!_activate(atlas->pixel_size)) return vec2_t<f32>(0.0f, 0.0f);

    f32 pen_x = 0.0f;
    f32 max_w = 0.0f;
    f32 lines = 1.0f;

    for (size_t i = 0; i < text.size();) {
        char32_t c = ft::utf8_next(text, i);

        if (c == '\n') {
            max_w = std::max(max_w, pen_x);
            pen_x = 0.0f;
            lines += 1.0f;
            continue;
        }

        const glyph_t *glyph = atlas->get(face, c);
        if (glyph == nullptr) continue;
        pen_x += glyph->advance;
    }
    max_w = std::max(max_w, pen_x);

    const f32 h     = (atlas->ascender - atlas->descender) + (lines - 1.0f) * atlas->line_h;
    const f32 scale = height / (f32)atlas->pixel_size;
    return vec2_t<f32>(max_w, h) * scale;
}
