#version 330 core

layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec3 a_flat;

uniform mat4 model, view, proj;
uniform vec3 light0_pos, light1_pos;   // eye space
uniform vec4 light_ambient, light_diffuse, light_specular;
uniform vec4 material_ambient, material_diffuse, material_specular;
uniform float material_shininess;

out vec3 v_pos;
out vec3 v_normal;
out vec3 v_flat;
out vec4 v_gouraud;

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
    mat4 mv = view * model;
    vec4 p  = mv * vec4(a_pos, 1.0);

    v_pos     = p.xyz;
    v_normal  = normalize(mat3(transpose(inverse(mv))) * a_normal);
    v_flat    = a_flat;
    v_gouraud = shade(v_pos, v_normal);

    gl_Position = proj * p;
}
