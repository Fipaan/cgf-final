#version 330 core

in vec3 v_world;

uniform vec3  light_pos;
uniform float far_plane;

out float frag_dist;

// distance to the light, 0..1: what the shadow cube map stores
void main()
{
    frag_dist = length(v_world - light_pos) / far_plane;
}
