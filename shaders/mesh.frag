#version 330 core

in vec3 v_pos;
in vec3 v_world;
in vec3 v_wnormal;
in vec3 v_normal;
in vec3 v_flat;
in vec2 v_uv;
in vec4 v_gouraud;

uniform int shading_mode; // 0 = normal as color, 1 = Gouraud, 2 = Phong
uniform vec3 light0_pos, light1_pos;   // eye space
uniform vec4 light_ambient, light_diffuse, light_specular;
uniform vec4 material_ambient, material_diffuse, material_specular;
uniform float material_shininess;

uniform vec3  up_eye;         // world up in eye space, for the hemisphere ambient
uniform float time;           // seconds, the lamp flickers with it
uniform sampler2D diffuse_map; // unit 1, mipmapped
uniform int   has_texture;
uniform float uv_tile;        // 0 = v_uv, else the image repeats every uv_tile units of the world
uniform samplerCube shadow_map; // distance to the lamp / shadow_far, see shadow.frag
uniform int   shadows_on;
uniform vec3  light0_world;
uniform float shadow_far;
uniform float exposure;
uniform float fog_density;
uniform vec3  fog_color;
uniform int   point_count;    // candles, the cauldron glow
uniform vec3  point_pos[8];   // eye space
uniform vec3  point_col[8];   // colour * intensity
uniform float point_radius[8];

out vec4 frag_color;

// HDR result, `finish` brings it to the screen
// `sh` is how much of the lamp reaches the point (1 = all of it)
// `albedo` is the colour of the image at the point (1 = no texture), it goes on ambient and diffuse
vec3 shade(vec3 P, vec3 N, float sh, vec3 albedo)
{
    vec3 V = normalize(-P);
    vec3 kd = material_diffuse.rgb * albedo;
    vec3 ka = material_ambient.rgb * albedo;

    // sky-ish from above, a bit dimmer from below
    float hemi = 0.65 + 0.35 * (dot(N, up_eye) * 0.5 + 0.5);
    vec3 c = ka * light_ambient.rgb * hemi;

    // what the material glows with on its own (flames, liquids): ambient above diffuse, not dimmed by the room
    c += max(material_ambient.rgb - material_diffuse.rgb, 0.0);

    // light 0: the lamp, flickers like a flame; light 1: the headlight, falls off quickly
    float flick = 1.0 + 0.04 * sin(time * 13.0) + 0.03 * sin(time * 7.3 + 1.0) + 0.02 * sin(time * 31.0);
    vec3 lp[2] = vec3[2](light0_pos, light1_pos);
    for (int i = 0; i < 2; ++i) {
        vec3 L = lp[i] - P;
        float d = length(L);
        L /= d;
        float att = i == 0 ? flick * sh / (1.0 + 0.02 * d + 0.012 * d * d) : 1.0 / (1.0 + 0.03 * d * d);
        float nl = max(dot(N, L), 0.0);
        c += kd * light_diffuse.rgb * nl * att;
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
        c += kd * point_col[i] * nl * att;
        if (nl > 0.0) {
            vec3 H = normalize(L + V);
            c += material_specular.rgb * point_col[i] * pow(max(dot(N, H), 0.0), material_shininess * 3.0) * att;
        }
    }

    // cool rim, the room feels like it has a night outside
    float rim = pow(1.0 - max(dot(N, V), 0.0), 3.0);
    c += kd * vec3(0.30, 0.40, 0.65) * rim * 0.25;
    return c;
}

// 1 = the lamp sees the point, 0 = something is in the way. 8 taps around the direction soften the edge
float shadow_factor(vec3 world)
{
    vec3 dir = world - light0_world;
    float d = length(dir);
    float bias = 0.06 + 0.01 * d;
    float r = 0.02 + 0.01 * d;
    float lit = 0.0;
    for (int i = 0; i < 8; ++i) {
        vec3 o = vec3((i & 1) == 0 ? -1.0 : 1.0, (i & 2) == 0 ? -1.0 : 1.0, (i & 4) == 0 ? -1.0 : 1.0) * r;
        float stored = texture(shadow_map, dir + o).r * shadow_far;
        lit += d - bias <= stored ? 1.0 : 0.0;
    }
    return lit / 8.0;
}

// tone mapping (soft shoulder instead of a hard clamp), fog, a little noise against banding
vec4 finish(vec3 c, vec3 P)
{
    c *= exposure;
    c = c * (1.0 + c / 4.0) / (1.0 + c);

    float f = 1.0 - exp(-pow(length(P) * fog_density, 2.0));
    c = mix(c, fog_color, f);

    float n = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453);
    c += (n - 0.5) / 255.0;
    return vec4(c, 1.0);
}

// colour of the image at this fragment, white without one
vec3 albedo_color()
{
    if (has_texture == 0) return vec3(1.0);

    vec2 uv = v_uv;
    if (uv_tile > 0.0) {
        // box mapping: drop the axis the surface faces along
        vec3 a = abs(v_wnormal);
        vec3 w = v_world / uv_tile;
        uv = (a.x >= a.y && a.x >= a.z) ? w.zy : (a.y >= a.z ? w.xz : w.xy);
    }
    return texture(diffuse_map, uv).rgb;
}

void main()
{
    if (shading_mode == 0) { frag_color = vec4(v_flat, 1.0); return; }

    vec3 albedo = albedo_color();
    if (shading_mode == 1) frag_color = finish(v_gouraud.rgb * albedo, v_pos); // lit per vertex, the image on top
    else {
        float sh = shadows_on != 0 ? shadow_factor(v_world) : 1.0;
        frag_color = finish(shade(v_pos, normalize(v_normal), sh, albedo), v_pos);
    }
}
