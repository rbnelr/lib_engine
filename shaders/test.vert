#version 330 // version 3.3

attribute	vec3	col;
attribute	vec3	pos_world;
varying		vec3	vs_col;

uniform		mat4	world_to_clip;

void main () {
	gl_Position = world_to_clip * vec4(pos_world,1);
	vs_col = col;
}
