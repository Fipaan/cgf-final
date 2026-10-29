#version 330 core

layout (location = 3) in vec4 v; // xy = pos (pixels), zw = uv

out vec2 uv;

uniform mat4 proj;

void main() {
    gl_Position = proj * vec4(v.xy, 0.0, 1.0);
    uv = v.zw;
}
