#version 330 core

in vec2 uv;

out vec4 frag;

uniform sampler2D glyph;
uniform vec3 color;

void main() {
    // Freetype returns red-channel bitmap, so it results in alpha channel for the texture
    frag = vec4(color, texture(glyph, uv).r);
}
