#version 330 core

in vec2 uv;

out vec4 frag;

uniform sampler2D glyph;
uniform vec3 color;

void main() {
    frag = vec4(color, texture(glyph, uv).r);
}
