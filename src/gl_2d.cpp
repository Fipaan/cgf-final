#include "gl_2d.hpp"

gl::rect_t::rect_t(vec2_t<GLfloat> pos, vec2_t<GLfloat> size)
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

gl::rect_t::rect_t(GLfloat x, GLfloat y, GLfloat w, GLfloat h)
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

gl::rect_t::rect_t()
    : colors{
        color_t(0xFFFFFFFF),
        color_t(0xFFFFFFFF),
        color_t(0xFFFFFFFF),
        color_t(0xFFFFFFFF),
    }
{
    pos.x = pos.y = size.x = size.y = 0;
}

void gl::rect_t::spread_pos(std::vector<GLfloat>& storage)
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

void gl::rect_t::spread_color(std::vector<GLfloat>& storage, gl::vad_cpv_t cpv)
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

gl::triangle_t::triangle_t(vec2_t<GLfloat> v1, vec2_t<GLfloat> v2, vec2_t<GLfloat> v3)
    : vertices{v1, v2, v3},
      colors{
          color_t(0xFFFFFFFF),
          color_t(0xFFFFFFFF),
          color_t(0xFFFFFFFF),
      }
{
    update_cache();
}

gl::triangle_t::triangle_t(vec2_t<GLfloat> center, vec2_t<GLfloat> up, GLfloat radius)
    : colors{
          color_t(0xFFFFFFFF),
          color_t(0xFFFFFFFF),
          color_t(0xFFFFFFFF),
      }
{
    // TODO: right now op for vec2_t<T> *= K does not exist
    up = up * radius;
    vertices[0] = center + up;

    constexpr GLfloat angle = 2*gl::PI / 3.0f;

    vertices[1] = center + up.rot(angle);
    vertices[2] = center + up.rot(angle);

    update_cache();
}

gl::triangle_t::triangle_t()
    : colors{
          color_t(0xFFFFFFFF),
          color_t(0xFFFFFFFF),
          color_t(0xFFFFFFFF),
      }
{}

void gl::triangle_t::update_cache()
{
    auto& [a, b, c] = vertices;
    sides[0] = std::sqrt(vec2::dst_sqr(a, b));
    sides[1] = std::sqrt(vec2::dst_sqr(b, c));
    sides[2] = std::sqrt(vec2::dst_sqr(c, a));

    const float area_twice = std::abs(
        (b.x - a.x) * (c.y - a.y) -
        (b.y - a.y) * (c.x - a.x)
    );

    if (area_twice > 0.0f) {
        R = sides[0] * sides[1] * sides[2] / (2.0f * area_twice);
    } else R = 0.0f;
}

void gl::triangle_t::change_R(GLfloat R)
{
    assert(this->R != 0);

    const GLfloat scale = R / this->R;
    const auto center = get_center();

    for (auto& vertex : vertices) {
        vertex = center + (vertex - center) * scale;
    }

    update_cache();
}

void gl::triangle_t::change_side(GLfloat side)
{
    change_R(side * R_to_side_ratio_eq_tr);
}

vec2_t<GLfloat> gl::triangle_t::get_center()
{
    return (vertices[0] + vertices[1] + vertices[2]) / 3.0f;
}

void gl::triangle_t::set_center(vec2_t<GLfloat> center)
{
    move(center - get_center());
}

void gl::triangle_t::set_up(vec2_t<GLfloat> up)
{
    const auto current_up = vertices[0] - get_center();

    rot(vec2::compute_angle(up, current_up));
}

void gl::triangle_t::move(vec2_t<GLfloat> vec)
{
    for (auto& vertex : vertices) {
        vertex += vec;
    }
}

void gl::triangle_t::rot(GLfloat angle)
{
    const auto center = get_center();

    for (auto& vertex : vertices) {
        vertex.rot(angle, center);
    }
}

void gl::triangle_t::spread_pos(std::vector<GLfloat>& storage)
{
    storage.resize(3 * 2);

    for (size_t i = 0; i < std::size(vertices); ++i) {
        storage[2*i + 0] = vertices[i].x;
        storage[2*i + 1] = vertices[i].y;
    }
}

void gl::triangle_t::spread_color(
    std::vector<GLfloat>& storage,
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
