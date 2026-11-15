#include "shape.hpp"

rect_t::rect_t(vec2_t<gl::f32> pos, vec2_t<gl::f32> size)
    : colors{
        color_t(0xFFFFFFFF),
        color_t(0xFFFFFFFF),
        color_t(0xFFFFFFFF),
        color_t(0xFFFFFFFF),
    }
{
    pos.x  = pos.x;
    pos.y  = pos.y;
    size.x = size.x;
    size.y = size.y;
}

rect_t::rect_t(gl::f32 x, gl::f32 y, gl::f32 w, gl::f32 h)
    : colors{
        color_t(0xFFFFFFFF),
        color_t(0xFFFFFFFF),
        color_t(0xFFFFFFFF),
        color_t(0xFFFFFFFF),
    }
{
    pos.x  = x;
    pos.y  = y;
    size.x = w;
    size.y = h;
}

rect_t::rect_t()
    : colors{
        color_t(0xFFFFFFFF),
        color_t(0xFFFFFFFF),
        color_t(0xFFFFFFFF),
        color_t(0xFFFFFFFF),
    }
{
    pos.x = pos.y = size.x = size.y = 0;
}

void rect_t::spread_pos(std::vector<gl::f32>& storage)
{
    storage.resize(4*2);
    storage[2*0+0] = pos.x;
    storage[2*0+1] = pos.y;
    storage[2*1+0] = pos.x + size.x;
    storage[2*1+1] = pos.y;
    storage[2*2+0] = pos.x + size.x;
    storage[2*2+1] = pos.y + size.y;
    storage[2*3+0] = pos.x;
    storage[2*3+1] = pos.y + size.y;
}

void rect_t::spread_color(std::vector<gl::f32>& storage, gl::vad_cpv_t cpv)
{
    storage.resize(std::size(colors)*4);
    bool rgb = true;
    bool alpha = false;
    switch (cpv) {
    case gl::vad_cpv_t::comp_3:    break;
    case gl::vad_cpv_t::comp_4:    alpha = true; break;
    case gl::vad_cpv_t::comp_bgra: alpha = true; rgb = false; break;
    case gl::vad_cpv_t::comp_1:
    case gl::vad_cpv_t::comp_2:
    default:                       assert(false && "Invalid value for spreading color");
    }
    size_t comps = alpha ? 4 : 3;
    for (size_t i = 0; i < std::size(colors); ++i) {
        size_t comps_width = comps*i;
        if (rgb) {
            storage[comps_width + 0] = colors[i].r;
            storage[comps_width + 1] = colors[i].g;
            storage[comps_width + 2] = colors[i].b;
        } else {
            storage[comps_width + 0] = colors[i].b;
            storage[comps_width + 1] = colors[i].g;
            storage[comps_width + 2] = colors[i].r;
        }
        if (alpha) storage[comps_width + 3] = colors[i].a;
    }
}

triangle_t::triangle_t(vec2_t<gl::f32> v1, vec2_t<gl::f32> v2, vec2_t<gl::f32> v3)
    : vertices{v1, v2, v3},
      colors{
          color_t(0xFFFFFFFF),
          color_t(0xFFFFFFFF),
          color_t(0xFFFFFFFF),
      }
{
    update_cache();
}

triangle_t::triangle_t(vec2_t<gl::f32> center, vec2_t<gl::f32> up, gl::f32 radius)
    : colors{
          color_t(0xFFFFFFFF),
          color_t(0xFFFFFFFF),
          color_t(0xFFFFFFFF),
      }
{
    // TODO: right now op for vec2_t<T> *= K does not exist
    up = up * radius;
    vertices[0] = center + up;

    constexpr gl::f32 angle = 2*gl::PI / 3.0f;

    vertices[1] = center + up.rot(angle);
    vertices[2] = center + up.rot(angle);

    update_cache();
}

triangle_t::triangle_t()
    : colors{
          color_t(0xFFFFFFFF),
          color_t(0xFFFFFFFF),
          color_t(0xFFFFFFFF),
      }
{}

void triangle_t::update_cache()
{
    auto& [a, b, c] = vertices;
    sides[0] = vec2::dst(a, b);
    sides[1] = vec2::dst(b, c);
    sides[2] = vec2::dst(c, a);

    const float area_twice = std::abs(
        (b.x - a.x) * (c.y - a.y) -
        (b.y - a.y) * (c.x - a.x)
    );

    if (area_twice > 0.0f) {
        R = sides[0] * sides[1] * sides[2] / (2.0f * area_twice);
    } else R = 0.0f;
}

void triangle_t::change_R(gl::f32 R)
{
    assert(this->R != 0);

    const gl::f32 scale = R / this->R;
    const auto center = get_center();

    for (auto& vertex : vertices) {
        vertex = center + (vertex - center) * scale;
    }

    update_cache();
}

void triangle_t::change_side(gl::f32 side)
{
    change_R(side * R_to_side_ratio_eq_tr);
}

vec2_t<gl::f32> triangle_t::get_center()
{
    return (vertices[0] + vertices[1] + vertices[2]) / 3.0f;
}

void triangle_t::set_center(vec2_t<gl::f32> center)
{
    move(center - get_center());
}

void triangle_t::set_up(vec2_t<gl::f32> up)
{
    const auto current_up = vertices[0] - get_center();

    rot(vec2::compute_angle(up, current_up));
}

void triangle_t::move(vec2_t<gl::f32> vec)
{
    for (auto& vertex : vertices) {
        vertex += vec;
    }
}

void triangle_t::rot(gl::f32 angle)
{
    const auto center = get_center();

    for (auto& vertex : vertices) {
        vertex.rot(angle, center);
    }
}

void triangle_t::spread_pos(std::vector<gl::f32>& storage)
{
    storage.resize(3 * 2);

    for (size_t i = 0; i < std::size(vertices); ++i) {
        storage[2*i + 0] = vertices[i].x;
        storage[2*i + 1] = vertices[i].y;
    }
}

void triangle_t::spread_color(
    std::vector<gl::f32>& storage,
    gl::vad_cpv_t cpv
)
{
    bool rgb = true;
    bool alpha = false;

    switch (cpv) {
    case gl::vad_cpv_t::comp_3:    break;
    case gl::vad_cpv_t::comp_4:    alpha = true; break;
    case gl::vad_cpv_t::comp_bgra: alpha = true; rgb = false; break;
    case gl::vad_cpv_t::comp_1:
    case gl::vad_cpv_t::comp_2:
    default:                       assert(false && "Invalid value for spreading color");
    }

    const size_t comps = alpha ? 4 : 3;
    storage.resize(std::size(colors) * comps);

    for (size_t i = 0; i < std::size(colors); ++i) {
        const size_t comps_width = comps * i;

        if (rgb) {
            storage[comps_width + 0] = colors[i].r;
            storage[comps_width + 1] = colors[i].g;
            storage[comps_width + 2] = colors[i].b;
        } else {
            storage[comps_width + 0] = colors[i].b;
            storage[comps_width + 1] = colors[i].g;
            storage[comps_width + 2] = colors[i].r;
        }

        if (alpha) storage[comps_width + 3] = colors[i].a;
    }
}
