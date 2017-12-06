#version 150 core // version 3.2

$include "common.glsl"

in		vec3	vs_pos_cam;
in		vec3	vs_norm_cam;
in		vec4	vs_tang_cam;
in		vec2	vs_uv;
in		vec4	vs_col;

uniform sampler2D	tex0;

void main () {
	FRAG_COL( texture(tex0, vs_uv).rgba );
}
