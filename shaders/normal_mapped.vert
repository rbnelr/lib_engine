#version 150 core // version 3.2

in		vec3	pos_model;
in		vec3	norm_model;
in		vec4	tang_model;
in		vec2	uv;
in		vec4	col;

out		vec3	vs_pos_cam;
out		vec3	vs_norm_cam;
out		vec4	vs_tang_cam;
out		vec2	vs_uv;
out		vec4	vs_col;

uniform	mat4	model_to_world;
uniform	mat4	world_to_cam;
uniform	mat4	cam_to_clip;

void main () {
	vec3 pos_world =	(model_to_world * vec4(pos_model,1)).xyz;
	vec3 pos_cam =		(world_to_cam * vec4(pos_world,1)).xyz;
	
	mat3 model_to_world_norm =	transpose(inverse(mat3(model_to_world))); // only rotation
	mat3 world_to_cam_norm =	transpose(inverse(mat3(world_to_cam))); // only rotation
	
	vec3 norm_world =	model_to_world_norm * norm_model;
	vec3 norm_cam =		world_to_cam_norm * norm_world;
	
	vec4 tang_world =	vec4(model_to_world_norm * tang_model.xyz, tang_model.w);
	vec4 tang_cam =		vec4(world_to_cam_norm * tang_world.xyz, tang_world.w);
	
	gl_Position =		cam_to_clip * vec4(pos_cam, 1);
	
	vs_pos_cam =		pos_cam.xyz;
	vs_norm_cam =		norm_cam;
	vs_tang_cam =		tang_cam;
	vs_uv =				uv;
	vs_col =			col;
}
