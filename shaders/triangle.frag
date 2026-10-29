#version 300 es

precision highp float;
precision highp int;

in vec3 color;

out vec4 FragColor;

void main() {
   FragColor = vec4(color.xyz, 1.0);
}
