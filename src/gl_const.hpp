#pragma once

extern "C" {
    #include <GLFW/glfw3.h>
}
#include <cstdint>

namespace gl {
    constexpr auto PI = std::numbers::pi_v<GLfloat>;
    // Convention:
    // GLFW_KEY_X -> x
    // Exceptions:
    //     GLFW_KEY_[0-9]  -> key_[0-9]
    //     GLFW_KEY_DELETE -> key_delete
    enum kb_key_t : std::uint16_t {
        space         = GLFW_KEY_SPACE,
        apostrophe    = GLFW_KEY_APOSTROPHE,
        comma         = GLFW_KEY_COMMA,
        minus         = GLFW_KEY_MINUS,
        period        = GLFW_KEY_PERIOD,
        slash         = GLFW_KEY_SLASH,
        key_0         = GLFW_KEY_0,
        key_1         = GLFW_KEY_1,
        key_2         = GLFW_KEY_2,
        key_3         = GLFW_KEY_3,
        key_4         = GLFW_KEY_4,
        key_5         = GLFW_KEY_5,
        key_6         = GLFW_KEY_6,
        key_7         = GLFW_KEY_7,
        key_8         = GLFW_KEY_8,
        key_9         = GLFW_KEY_9,
        semicolon     = GLFW_KEY_SEMICOLON,
        equal         = GLFW_KEY_EQUAL,
        a             = GLFW_KEY_A,
        b             = GLFW_KEY_B,
        c             = GLFW_KEY_C,
        d             = GLFW_KEY_D,
        e             = GLFW_KEY_E,
        f             = GLFW_KEY_F,
        g             = GLFW_KEY_G,
        h             = GLFW_KEY_H,
        i             = GLFW_KEY_I,
        j             = GLFW_KEY_J,
        k             = GLFW_KEY_K,
        l             = GLFW_KEY_L,
        m             = GLFW_KEY_M,
        n             = GLFW_KEY_N,
        o             = GLFW_KEY_O,
        p             = GLFW_KEY_P,
        q             = GLFW_KEY_Q,
        r             = GLFW_KEY_R,
        s             = GLFW_KEY_S,
        t             = GLFW_KEY_T,
        u             = GLFW_KEY_U,
        v             = GLFW_KEY_V,
        w             = GLFW_KEY_W,
        x             = GLFW_KEY_X,
        y             = GLFW_KEY_Y,
        z             = GLFW_KEY_Z,
        left_bracket  = GLFW_KEY_LEFT_BRACKET,
        backslash     = GLFW_KEY_BACKSLASH,
        right_bracket = GLFW_KEY_RIGHT_BRACKET,
        grave_accent  = GLFW_KEY_GRAVE_ACCENT,
        world_1       = GLFW_KEY_WORLD_1,
        world_2       = GLFW_KEY_WORLD_2,
        escape        = GLFW_KEY_ESCAPE,
        enter         = GLFW_KEY_ENTER,
        tab           = GLFW_KEY_TAB,
        backspace     = GLFW_KEY_BACKSPACE,
        insert        = GLFW_KEY_INSERT,
        key_delete    = GLFW_KEY_DELETE,
        right         = GLFW_KEY_RIGHT,
        left          = GLFW_KEY_LEFT,
        down          = GLFW_KEY_DOWN,
        up            = GLFW_KEY_UP,
        page_up       = GLFW_KEY_PAGE_UP,
        page_down     = GLFW_KEY_PAGE_DOWN,
        home          = GLFW_KEY_HOME,
        end           = GLFW_KEY_END,
        caps_lock     = GLFW_KEY_CAPS_LOCK,
        scroll_lock   = GLFW_KEY_SCROLL_LOCK,
        num_lock      = GLFW_KEY_NUM_LOCK,
        print_screen  = GLFW_KEY_PRINT_SCREEN,
        pause         = GLFW_KEY_PAUSE,
        f1            = GLFW_KEY_F1,
        f2            = GLFW_KEY_F2,
        f3            = GLFW_KEY_F3,
        f4            = GLFW_KEY_F4,
        f5            = GLFW_KEY_F5,
        f6            = GLFW_KEY_F6,
        f7            = GLFW_KEY_F7,
        f8            = GLFW_KEY_F8,
        f9            = GLFW_KEY_F9,
        f10           = GLFW_KEY_F10,
        f11           = GLFW_KEY_F11,
        f12           = GLFW_KEY_F12,
        f13           = GLFW_KEY_F13,
        f14           = GLFW_KEY_F14,
        f15           = GLFW_KEY_F15,
        f16           = GLFW_KEY_F16,
        f17           = GLFW_KEY_F17,
        f18           = GLFW_KEY_F18,
        f19           = GLFW_KEY_F19,
        f20           = GLFW_KEY_F20,
        f21           = GLFW_KEY_F21,
        f22           = GLFW_KEY_F22,
        f23           = GLFW_KEY_F23,
        f24           = GLFW_KEY_F24,
        f25           = GLFW_KEY_F25,
        kp_0          = GLFW_KEY_KP_0,
        kp_1          = GLFW_KEY_KP_1,
        kp_2          = GLFW_KEY_KP_2,
        kp_3          = GLFW_KEY_KP_3,
        kp_4          = GLFW_KEY_KP_4,
        kp_5          = GLFW_KEY_KP_5,
        kp_6          = GLFW_KEY_KP_6,
        kp_7          = GLFW_KEY_KP_7,
        kp_8          = GLFW_KEY_KP_8,
        kp_9          = GLFW_KEY_KP_9,
        kp_decimal    = GLFW_KEY_KP_DECIMAL,
        kp_divide     = GLFW_KEY_KP_DIVIDE,
        kp_multiply   = GLFW_KEY_KP_MULTIPLY,
        kp_subtract   = GLFW_KEY_KP_SUBTRACT,
        kp_add        = GLFW_KEY_KP_ADD,
        kp_enter      = GLFW_KEY_KP_ENTER,
        kp_equal      = GLFW_KEY_KP_EQUAL,
        left_shift    = GLFW_KEY_LEFT_SHIFT,
        left_control  = GLFW_KEY_LEFT_CONTROL,
        left_alt      = GLFW_KEY_LEFT_ALT,
        left_super    = GLFW_KEY_LEFT_SUPER,
        right_shift   = GLFW_KEY_RIGHT_SHIFT,
        right_control = GLFW_KEY_RIGHT_CONTROL,
        right_alt     = GLFW_KEY_RIGHT_ALT,
        right_super   = GLFW_KEY_RIGHT_SUPER,
        menu          = GLFW_KEY_MENU,
        last          = GLFW_KEY_LAST,
    };

    namespace gl_bool {
        enum gl_bool_t : std::uint8_t {
            FALSE = GL_FALSE,
            TRUE  = GL_TRUE,
        };
    }
    using gl_bool::gl_bool_t;

    namespace glfw_bool {
        enum glfw_bool_t : std::uint8_t {
            FALSE = GLFW_FALSE,
            TRUE  = GLFW_TRUE,
        };
    }
    using glfw_bool::glfw_bool_t;

    enum kb_key_state_t : std::uint8_t {
        release = GLFW_RELEASE,
        press   = GLFW_PRESS,
    };

    // Convention:
    // [UNSIGNED_]X -> [U]X
    // GL_X -> gl_x
    enum data_kind : std::uint16_t {
        // accepted by:
        //     glVertexAttribPointer
        //     glVertexAttribIPointer
        gl_byte   = GL_BYTE,
        gl_ubyte  = GL_UNSIGNED_BYTE,
        gl_short  = GL_SHORT,
        gl_ushort = GL_UNSIGNED_SHORT,
        gl_int    = GL_INT,
        gl_uint   = GL_UNSIGNED_INT,
        // accepted by:
        //     glVertexAttribPointer
        gl_half_float = GL_HALF_FLOAT,
        gl_float      = GL_FLOAT,
        gl_double     = GL_DOUBLE, // + accepted by: glVertexAttribLPointer
        gl_int_2_10_10_10_rev   = GL_INT_2_10_10_10_REV,
        gl_uint_2_10_10_10_rev  = GL_UNSIGNED_INT_2_10_10_10_REV,
        gl_uint_10f_11f_11f_rev = GL_UNSIGNED_INT_10F_11F_11F_REV,
    };

    // Convention:
    // GL_X_BUFFER -> x
    enum buffer_kind : std::uint16_t {
        array              = GL_ARRAY_BUFFER,
        copy_read          = GL_COPY_READ_BUFFER,
        copy_write         = GL_COPY_WRITE_BUFFER,
        element_array      = GL_ELEMENT_ARRAY_BUFFER,
        pixel_pack         = GL_PIXEL_PACK_BUFFER,
        pixel_unpack       = GL_PIXEL_UNPACK_BUFFER,
        texture            = GL_TEXTURE_BUFFER,
        transform_feedback = GL_TRANSFORM_FEEDBACK_BUFFER,
        uniform            = GL_UNIFORM_BUFFER,
    };

    // Components Per Vertex
    enum vad_cpv_t : std::uint16_t {
        comp_1    = 1,
        comp_2    = 2,
        comp_3    = 3,
        comp_4    = 4,
        comp_bgra = GL_BGRA,
    };

    // Data Store Usage
    // Convention:
    // GL_X -> x
    enum dsu_t : std::uint16_t {
        stream_draw  = GL_STREAM_DRAW,
        stream_read  = GL_STREAM_READ,
        stream_copy  = GL_STREAM_COPY,
        static_draw  = GL_STATIC_DRAW,
        static_read  = GL_STATIC_READ,
        static_copy  = GL_STATIC_COPY,
        dynamic_draw = GL_DYNAMIC_DRAW,
        dynamic_read = GL_DYNAMIC_READ,
        dynamic_copy = GL_DYNAMIC_COPY,
    };

    enum v_type_t : std::uint8_t {
        v1 = 1,
        v2 = 2,
        v3 = 3,
        v4 = 4,
    };

    enum m_type_t : std::uint8_t {
        m2x2,
        m2x3,
        m2x4,
        m3x2,
        m3x3,
        m3x4,
        m4x2,
        m4x3,
        m4x4,
    };
}
