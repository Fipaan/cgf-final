#version 330 core

layout(location = 0) in vec3  a_pos;
layout(location = 1) in vec4  a_color;
layout(location = 2) in float a_size; // world units

uniform mat4  view, proj;
uniform float px_scale; // viewport height / (2 * tan(fovy / 2))

out vec4 v_color;

void main()
{
    vec4 p = view * vec4(a_pos, 1.0);
    gl_Position  = proj * p;
    gl_PointSize = clamp(a_size * px_scale / max(-p.z, 0.1), 1.0, 128.0);
    v_color = a_color;
}
