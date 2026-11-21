/*
 * 2d/color.hpp -
 *     Color related logic.
 */
#pragma once

#include <vector>
#include <cmath>
#include <span>
#include <algorithm>

#include <core/gl/const.hpp>
#include <gl/flat.hpp>

namespace math {

struct rgba_t {
    gl::f32 r, g, b, a;
};

struct hsva_t {
    gl::f32 h, s, v, a;
};

struct color_t {
    gl::f32 r, g, b, a;

    constexpr color_t(gl::f32 r, gl::f32 g, gl::f32 b, gl::f32 a);
    constexpr color_t(rgba_t rgba);
    constexpr color_t(uint32_t rgba);
    constexpr color_t(hsva_t hsva);

    constexpr rgba_t to_rgba() const;
    constexpr uint32_t to_rgba_hex() const;
    constexpr hsva_t to_hsva() const;

    void spread(gl::flat_t<gl::f32>& container, gl::vad_cpv_t cpv) const;
    // raw version of upper function
    //     rgb - specifies order of components (rgb/bgr)
    //     a   - include alpha component or not
    void spread(gl::flat_t<gl::f32>& container, bool rgb = true, bool alpha = false) const;
};

///// IMPLEMENTATION ///////////////////////////////////////////////////////////

constexpr color_t::color_t(gl::f32 r, gl::f32 g, gl::f32 b, gl::f32 a)
{
    this->r = r;
    this->g = g;
    this->b = b;
    this->a = a;
}

constexpr color_t::color_t(rgba_t rgba)
{
    this->r = rgba.r;
    this->g = rgba.g;
    this->b = rgba.b;
    this->a = rgba.a;
}

constexpr color_t::color_t(uint32_t rgba)
{
    r = ((gl::f32)((rgba >> 24) & 0xFF)) / 0xFF;
    g = ((gl::f32)((rgba >> 16) & 0xFF)) / 0xFF;
    b = ((gl::f32)((rgba >> 8 ) & 0xFF)) / 0xFF;
    a = ((gl::f32)((rgba >> 0 ) & 0xFF)) / 0xFF;
}

constexpr color_t::color_t(hsva_t hsva)
{
    auto& [h, s, v, a] = hsva;
    h = std::fmod(h, 360.0f);
    if (h < 0.0f) h += 360.0f;

    s = std::clamp(s, 0.0f, 1.0f);
    v = std::clamp(v, 0.0f, 1.0f);

    gl::f32 c = v * s;
    gl::f32 x = c * (1.0f - std::fabs(std::fmod(h/60.0f, 2.0f) - 1.0f));
    gl::f32 m = v - c;

    gl::f32 r, g, b;
    if (h < 60.0f) {
        r = c;
        g = x;
        b = 0.0f;
    } else if (h < 120.0f) {
        r = x;
        g = c;
        b = 0.0f;
    } else if (h < 180.0f) {
        r = 0.0f;
        g = c;
        b = x;
    } else if (h < 240.0f) {
        r = 0.0f;
        g = x;
        b = c;
    } else if (h < 300.0f) {
        r = x;
        g = 0.0f;
        b = c;
    } else {
        r = c;
        g = 0.0f;
        b = x;
    }

    this->r = r + m;
    this->g = g + m;
    this->b = b + m;
    this->a = a;
}

constexpr rgba_t color_t::to_rgba() const
{
    return rgba_t(r, g, b, a);
}
constexpr uint32_t color_t::to_rgba_hex() const
{
    uint32_t result = 0;
    result += std::clamp<uint32_t>(r * 0xFF, 0, 0xFF) << 24;
    result += std::clamp<uint32_t>(g * 0xFF, 0, 0xFF) << 16;
    result += std::clamp<uint32_t>(b * 0xFF, 0, 0xFF) << 8;
    result += std::clamp<uint32_t>(a * 0xFF, 0, 0xFF) << 0;
    return result;
}

constexpr hsva_t color_t::to_hsva() const
{
    gl::f32 max = std::max({r, g, b});
    gl::f32 min = std::min({r, g, b});
    gl::f32 delta = max - min;

    hsva_t hsva;
    auto& [h, s, v, a] = hsva;

    v = max;
    s = (max == 0.0f) ? 0.0f : delta / max;

    if (delta == 0.0f) h = 0.0f;
    else if (max == r) {
        h = 60.0f * std::fmod( (g - b) / delta, 6.0f );
    } else if (max == g) {
        h = 60.0f * ( (b - r) / delta + 2.0f );
    } else {
        h = 60.0f * ( (r - g) / delta + 4.0f );
    }
    if (h < 0.0f) h += 360.0f;

    a = this->a;

    return hsva;
}

} // namespace math