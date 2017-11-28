#version 150 core // version 3.2

in		vec3	vs_pos_cam;
in		vec3	vs_norm_cam;
in		vec4	vs_tang_cam;
in		vec2	vs_uv;
in		vec3	vs_col;

out		vec4	frag_col;

void main () {
	frag_col = vec4(vs_norm_cam / 2 +0.5, 1);
}
