#version 150 core // version 3.2

$include "common.glsl"

in		vec3	vs_pos_cam;
in		vec3	vs_norm_cam;
in		vec4	vs_tang_cam;
in		vec2	vs_uv;
in		vec4	vs_col;

$include "normal_mapping.glsl"
$include "skybox.glsl"

uniform mat4	cam_to_world;

uniform sampler2D	albedo;
uniform sampler2D	normal;
uniform sampler2D	metallic;
uniform sampler2D	roughness;

void main () {
	vec4 alb = texture(albedo, vs_uv).rgba;
	vec3 norm_cam = normal_mapping(vs_pos_cam, vs_norm_cam, vs_tang_cam, vs_uv, normal);
	
	vec3 cam_to_p = normalize(vs_pos_cam);
	vec3 refl_cam = reflect(cam_to_p,
		//SPLIT_LEFT ? normalize(vs_norm_cam) :
		norm_cam);
	
	vec4 col = vec4( sky(mat3(cam_to_world) * refl_cam), 1 );
	
	col *= alb * vs_col;
	
	//DBG_COL(pow(texture(normal, vs_uv).rgb, vec3(2.2)));
	
	FRAG_COL(col);
}
