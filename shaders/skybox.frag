#version 150 core // version 3.2

$include "common.glsl"

in		vec3	vs_pos_world_dir;

$include "skybox.glsl"

void main () {
	FRAG_COL( vec4( sky(normalize(vs_pos_world_dir)), 1 ) );
}
