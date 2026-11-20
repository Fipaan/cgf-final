#version 330 core

in vec2 uv;

out vec4 frag;

uniform sampler2D glyph;
uniform vec4 color;

void main() {
    // Freetype returns red-channel bitmap, so it results in alpha channel for the texture
    frag = vec4(color.rgb, texture(glyph, uv).r * color.a);
}
