#version 150 core // version 3.2

in		vec3	vs_pos_world_rot;

out		vec3	frag_col;

uniform	vec2	mcursor_pos;
uniform	vec2	screen_dim;

vec3 sky (vec3 dir_world) {
	return dir_world * 0.5 +0.5;
}

void main () {
	frag_col = sky(normalize(vs_pos_world_rot));
}
