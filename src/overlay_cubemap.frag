#version 150 core // version 3.2

$include "common.glsl"

in		vec3	vs_pos_cubemap_space;

uniform samplerCube	tex0;

void main () {
	FRAG_COL( texture(tex0, normalize(vs_pos_cubemap_space)).rgba );
}
