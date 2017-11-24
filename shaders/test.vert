#version 150 core // version 3.2

in		vec3	col;
in		vec3	pos_world;
out		vec3	vs_col;

uniform	mat4	world_to_clip;

void main () {
	gl_Position = world_to_clip * vec4(pos_world,1);
	vs_col = col;
}
