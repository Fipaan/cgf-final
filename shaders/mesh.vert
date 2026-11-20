#version 330 core

layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec3 a_flat;

uniform mat4 model, view, proj;
uniform int shading_mode; // 0 = normal as color, 1 = Gouraud, 2 = Phong
uniform vec3 light0_pos, light1_pos;   // eye space
uniform vec4 light_ambient, light_diffuse, light_specular;
uniform vec4 material_ambient, material_diffuse, material_specular;
uniform float material_shininess;

uniform vec3  up_eye;         // world up in eye space, for the hemisphere ambient
uniform float time;           // seconds, the lamp flickers with it
uniform float exposure;
uniform float fog_density;
uniform vec3  fog_color;
uniform int   point_count;    // candles, the cauldron glow
uniform vec3  point_pos[8];   // eye space
uniform vec3  point_col[8];   // colour * intensity
uniform float point_radius[8];

out vec3 v_pos;
out vec3 v_normal;
out vec3 v_flat;
out vec4 v_gouraud;

// HDR result, `finish` brings it to the screen
vec3 shade(vec3 P, vec3 N)
{
    vec3 V = normalize(-P);

    // sky-ish from above, a bit dimmer from below
    float hemi = 0.65 + 0.35 * (dot(N, up_eye) * 0.5 + 0.5);
    vec3 c = material_ambient.rgb * light_ambient.rgb * hemi;

    // light 0: the lamp, flickers like a flame; light 1: the headlight, falls off quickly
    float flick = 1.0 + 0.04 * sin(time * 13.0) + 0.03 * sin(time * 7.3 + 1.0) + 0.02 * sin(time * 31.0);
    vec3 lp[2] = vec3[2](light0_pos, light1_pos);
    for (int i = 0; i < 2; ++i) {
        vec3 L = lp[i] - P;
        float d = length(L);
        L /= d;
        float att = i == 0 ? flick / (1.0 + 0.02 * d + 0.012 * d * d) : 1.0 / (1.0 + 0.03 * d * d);
        float nl = max(dot(N, L), 0.0);
        c += material_diffuse.rgb * light_diffuse.rgb * nl * att;
        if (nl > 0.0) {
            vec3 H = normalize(L + V);
            c += material_specular.rgb * light_specular.rgb * pow(max(dot(N, H), 0.0), material_shininess * 3.0) * att;
        }
    }

    for (int i = 0; i < point_count; ++i) {
        vec3 L = point_pos[i] - P;
        float d = length(L);
        L /= d;
        float r = point_radius[i];
        float att = clamp(1.0 - (d * d) / (r * r), 0.0, 1.0);
        att *= att;
        float nl = max(dot(N, L), 0.0);
        c += material_diffuse.rgb * point_col[i] * nl * att;
        if (nl > 0.0) {
            vec3 H = normalize(L + V);
            c += material_specular.rgb * point_col[i] * pow(max(dot(N, H), 0.0), material_shininess * 3.0) * att;
        }
    }

    // cool rim, the room feels like it has a night outside
    float rim = pow(1.0 - max(dot(N, V), 0.0), 3.0);
    c += material_diffuse.rgb * vec3(0.30, 0.40, 0.65) * rim * 0.25;
    return c;
}

void main()
{
    mat4 mv = view * model;
    vec4 p  = mv * vec4(a_pos, 1.0);

    v_pos     = p.xyz;
    v_normal  = normalize(mat3(transpose(inverse(mv))) * a_normal);
    v_flat    = a_flat;
    v_gouraud = vec4(shade(v_pos, v_normal), 1.0);

    gl_Position = proj * p;
}
