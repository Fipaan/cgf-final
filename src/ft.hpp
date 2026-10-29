#pragma once

extern "C" {
    #include <ft2build.h>
    #include FT_FREETYPE_H
}
#include "gl_first.hpp"
#include "math.hpp"

namespace ft {
    void log_ft_error(FT_Error err, std::string type);
    bool check_ft_error(FT_Error err, std::string type, bool silent = false);

    // returns codepoint, advances i; invalid bytes become U+FFFD
    char32_t utf8_next(const std::string& s, size_t& i);

    struct ft_lib_t {
        FT_Library library;
        bool initialized;

        ft_lib_t();
        ~ft_lib_t();

        bool init();
    };

    struct ft_face_t {
        ft_lib_t&       lib;
        FT_Face         face;
        std::string     path;
        bool            initialized;
        GLuint          texture;
        vec2_t<FT_UInt> base_size;

        ft_face_t(ft_lib_t& lib);
        ~ft_face_t();

        bool init(std::string path, FT_Long face_index);

        // pixel_size:
        //     units: pixels
        //     width  = 0 -> width  = height
        //     height = 0 -> height = width
        bool set_pixel_sizes(vec2_t<FT_UInt> pixel_size, vec2_t<GLfloat> dpi = vec2_t<GLfloat>::one());

        vec2_t<FT_UInt> measure_text(const std::string& text, GLfloat height, GLfloat spacing = 0);

        // char_size:
        //     units: 1/64 of points
        //     width  = 0 -> width  = height
        //     height = 0 -> height = width
        bool set_char_size(vec2_t<FT_F26Dot6> char_size, vec2_t<FT_UInt> res);

        // retrieve glyph index from character code
        // 0 means undefined character code
        FT_UInt get_char_index(FT_ULong charcode, bool silent = true);

        // load glyph image into the slot (erase previous one)
        bool load_glyph(FT_UInt glyph_index, FT_Int32 load_flags = FT_LOAD_DEFAULT, bool silent = true);

        // `get_char_index` + `load_glyph`
        bool load_char(FT_ULong charcode, FT_Int32 load_flags = FT_LOAD_DEFAULT, bool silent = true);

        // convert to an anti-aliased bitmap
        bool render_glyph(FT_Render_Mode render_mode = FT_RENDER_MODE_NORMAL, bool silent = true);
    };
}
