#version 330 core

layout(location = 0) in vec3 a_pos;

uniform mat4 model, view, proj;

out vec3 v_world;

void main()
{
    vec4 w  = model * vec4(a_pos, 1.0);
    v_world = w.xyz;
    gl_Position = proj * view * w;
}
