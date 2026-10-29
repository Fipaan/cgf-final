#version 300 es

precision highp float;
precision highp int;

in float alpha;

out vec4 FragColor;

void main() {
   FragColor = vec4(alpha, 0.0, 0.0, 1.0);
}
