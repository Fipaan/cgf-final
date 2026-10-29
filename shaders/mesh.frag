#version 330 core

in vec3 v_pos;
in vec3 v_normal;
in vec3 v_flat;
in vec4 v_gouraud;

uniform int shading_mode; // 0 = normal as color, 1 = Gouraud, 2 = Phong
uniform vec3 light0_pos, light1_pos;
uniform vec4 light_ambient, light_diffuse, light_specular;
uniform vec4 material_ambient, material_diffuse, material_specular;
uniform float material_shininess;

out vec4 frag_color;

vec4 shade(vec3 P, vec3 N)
{
    vec3 V = normalize(-P);
    vec3 lp[2] = vec3[2](light0_pos, light1_pos);

    vec4 c = material_ambient * light_ambient;
    for (int i = 0; i < 2; ++i) {
        vec3 L = normalize(lp[i] - P);
        float d = max(dot(N, L), 0.0);
        c += material_diffuse * light_diffuse * d;
        if (d > 0.0) {
            vec3 R = reflect(-L, N);
            c += material_specular * light_specular * pow(max(dot(R, V), 0.0), material_shininess);
        }
    }
    c = clamp(c, 0.0, 1.0);
    c.a = 1.0;
    return c;
}

void main()
{
    if      (shading_mode == 0) frag_color = vec4(v_flat, 1.0);
    else if (shading_mode == 1) frag_color = v_gouraud;
    else                        frag_color = shade(v_pos, normalize(v_normal));
}
