#version 330 core

in vec4 v_color;

out vec4 frag_color;

// soft round sprite, blended additively
void main()
{
    vec2 c = gl_PointCoord * 2.0 - 1.0;
    float r2 = dot(c, c);
    if (r2 > 1.0) discard;
    float a = 1.0 - r2;
    frag_color = vec4(v_color.rgb, v_color.a * a * a);
}
