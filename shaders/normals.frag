#version 150 core // version 3.2

in		vec3	vs_pos_world;
in		vec3	vs_norm_world;
in		vec2	vs_uv;
in		vec3	vs_col;

out		vec3	frag_col;

void main () {
	frag_col = vs_norm_world / 2 +0.5;
}
