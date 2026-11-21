#include "color.hpp"

using namespace math;

void color_t::spread(gl::flat_t<gl::f32>& container, gl::vad_cpv_t cpv) const
{
    spread(container, gl::vad_cpv::to_rgb(cpv), gl::vad_cpv::to_alpha(cpv));
}

void color_t::spread(gl::flat_t<gl::f32>& container, bool rgb, bool alpha) const
{
    if (rgb) {
        container.add(r);
        container.add(g);
        container.add(b);
    } else {
        container.add(b);
        container.add(g);
        container.add(r);
    }
    if (alpha) container.add(a);
}