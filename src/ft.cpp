#include "ft.hpp"
#include "basic.hpp"

void ft::log_ft_error(FT_Error err, std::string type)
{
    const char *msg = FT_Error_String(err);
    if (msg == nullptr) msg = "unknown error";
    errorfn("FT failed: {}: {} (code: {})", type, msg, err);
}

bool ft::check_ft_error(FT_Error err, std::string type, bool silent)
{
    bool ok = err == 0;
    if (!ok && !silent) log_ft_error(err, type);
    return ok;
}

ft::ft_lib_t::ft_lib_t()
{
    initialized = false;
}

ft::ft_lib_t::~ft_lib_t()
{
    if (initialized) {
        FT_Done_FreeType(library);
        initialized = false;
    }
}

bool ft::ft_lib_t::init()
{
    assert(!initialized);
    initialized = check_ft_error(
        FT_Init_FreeType(&library),
        "library init"
    );
    return initialized;
}

ft::ft_face_t::ft_face_t(ft_lib_t& lib)
    : lib(lib)
{
    initialized = false;
}

ft::ft_face_t::~ft_face_t()
{
    if (initialized) {
        FT_Done_Face(face);
        initialized = false;
    }
}

bool ft::ft_face_t::init(std::string path, FT_Long face_index)
{
    assert(!initialized);
    this->path = path;
    initialized = check_ft_error(
        FT_New_Face(lib.library, path.data(), face_index, &face),
        "face init"
    );

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return initialized;
}

bool ft::ft_face_t::set_pixel_sizes(vec2_t<FT_UInt> pixel_size, vec2_t<GLfloat> dpi)
{
    assert(initialized);
    base_size = pixel_size;
    pixel_size /= dpi;
    return check_ft_error(
        FT_Set_Pixel_Sizes(face, pixel_size.x, pixel_size.y),
        "set pixel sizes"
    );
}

vec2_t<FT_UInt> ft::ft_face_t::measure_text(const std::string& text, GLfloat height, GLfloat spacing)
{
    const FT_UInt line_h = face->size->metrics.height >> 6;

    FT_UInt pen_x = 0;
    FT_UInt max_w = 0;
    FT_UInt lines = 1;

    for (size_t i = 0; i < text.size();)
    {
        char32_t c = ft::utf8_next(text, i);

        if (c == '\n') {
            max_w = std::max(max_w, pen_x);
            pen_x = 0;
            lines++;
            continue;
        }

        if (!load_char(c)) continue;
        if (pen_x > 0) pen_x += spacing;
        pen_x += face->glyph->advance.x >> 6;
    }
    max_w = std::max(max_w, pen_x);

    FT_Pos asc  = face->size->metrics.ascender  >> 6;
    FT_Pos desc = face->size->metrics.descender >> 6;
    FT_Pos h = (asc - desc) + (lines - 1) * line_h;

    const vec2_t<GLfloat> scale = height / base_size.y;
    return (vec2_t<GLfloat>(max_w, h) * scale).cast<FT_UInt>();
}

bool ft::ft_face_t::set_char_size(vec2_t<FT_F26Dot6> char_size, vec2_t<FT_UInt> res)
{
    assert(initialized);
    return check_ft_error(
        FT_Set_Char_Size(face, char_size.x, char_size.y, res.x, res.y),
        "set char size"
    );
}

FT_UInt ft::ft_face_t::get_char_index(FT_ULong charcode, bool silent)
{
    FT_UInt glyph_index = FT_Get_Char_Index(face, charcode);
    if (!silent && glyph_index == 0) {
        errorfn("FT failed: get char index: undefined character code");
    }
    return glyph_index;
}

bool ft::ft_face_t::load_glyph(FT_UInt glyph_index, FT_Int32 load_flags, bool silent)
{
    return check_ft_error(
        FT_Load_Glyph(face, glyph_index, load_flags),
        "load glyph",
        silent
    );
}

bool ft::ft_face_t::load_char(FT_ULong charcode, FT_Int32 load_flags, bool silent)
{
    return check_ft_error(
        FT_Load_Char(face, charcode, load_flags),
        "load char",
        silent
    );
}

bool ft::ft_face_t::render_glyph(FT_Render_Mode render_mode, bool silent)
{
    return check_ft_error(
        FT_Render_Glyph(face->glyph, render_mode),
        "render glyph",
        silent
    );
}

char32_t ft::utf8_next(const std::string& s, size_t& i)
{
    unsigned char c = s[i++];
    if (c < 0x80) return c;

    int extra;
    char32_t cp;
    if      ((c & 0xE0) == 0xC0) { extra = 1; cp = c & 0x1F; }
    else if ((c & 0xF0) == 0xE0) { extra = 2; cp = c & 0x0F; }
    else if ((c & 0xF8) == 0xF0) { extra = 3; cp = c & 0x07; }
    else return 0xFFFD;

    while (extra--) {
        if (i >= s.size() || (s[i] & 0xC0) != 0x80) return 0xFFFD;
        cp = (cp << 6) | (s[i++] & 0x3F);
    }

    return cp;
}
