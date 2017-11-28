#version 150 core // version 3.2

in		vec3	vs_pos_cam;
in		vec3	vs_norm_cam;
in		vec4	vs_tang_cam;
in		vec2	vs_uv;
in		vec3	vs_col;

out		vec3	frag_col;

uniform	vec2	mcursor_pos;
uniform	vec2	screen_dim;

uniform sampler2D	tex0;

void main () {
	frag_col = texture(tex0, vs_uv).rgb;
}
