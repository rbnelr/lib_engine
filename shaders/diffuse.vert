#version 150 core // version 3.2

in		vec3	pos_world;
in		vec3	norm_world;
in		vec2	uv;
in		vec3	col;

out		vec3	vs_pos_world;
out		vec3	vs_norm_world;
out		vec2	vs_uv;
out		vec3	vs_col;

uniform	mat4	world_to_clip;

void main () {
	gl_Position = world_to_clip * vec4(pos_world,1);
	vs_pos_world =		pos_world;
	vs_norm_world =		norm_world;
	vs_uv =				uv;
	vs_col =			col;
}
