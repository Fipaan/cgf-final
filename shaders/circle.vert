#version 300 es

precision highp float;
precision highp int;

layout (location = 0) in vec2 aPos;
layout (location = 1) in float aAlpha;

out float alpha;

void main() {
   gl_Position = vec4(aPos, 0.0, 1.0);
   alpha = aAlpha;
}
