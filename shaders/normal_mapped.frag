#version 150 core // version 3.2

$include "common.glsl"

in		vec3	vs_pos_cam;
in		vec3	vs_norm_cam;
in		vec4	vs_tang_cam;
in		vec2	vs_uv;
in		vec4	vs_col;

uniform sampler2D	albedo;
uniform sampler2D	normal;
uniform sampler2D	metallic;
uniform sampler2D	roughness;

uniform mat4	cam_to_world;

float map (float x, float a, float b) { return (x -a) / (b -a); }

vec3 normal_mapping (vec3 pos_cam, vec3 geom_norm_cam, vec4 tang_cam, vec2 uv, sampler2D normal_tex) {
	
	geom_norm_cam = normalize(geom_norm_cam);
	
	mat3 TBN_tang_to_cam;
	{
		vec3	t =			tang_cam.xyz;
		float	b_sign =	tang_cam.w;
		vec3	b;
		vec3	n =			geom_norm_cam;
		
		t = normalize( t -(dot(t, n) * n) );
		b = cross(n, t);
		b *= b_sign;
		
		TBN_tang_to_cam = mat3(t, b, n);
	}
	
	vec3 normal_tang_sample = texture(normal_tex, uv).rgb; // normal in tangent space
	vec3 norm_tang = normalize(normal_tang_sample * 2.0 -1.0);
	vec3 norm_cam = TBN_tang_to_cam * norm_tang;
	
	vec3 view_cam = normalize(-pos_cam);
	
	//const float ARTIFACT_FIX_THRES = 0.128;
	//if (dot(view_cam, norm_cam) < ARTIFACT_FIX_THRES) {
	//	norm_cam = mix(norm_cam, geom_norm_cam, map(dot(view_cam, norm_cam), ARTIFACT_FIX_THRES, 0));
	//	//DBG_COL(map(dot(view_cam, norm_cam), ARTIFACT_FIX_THRES, 0));
	//}
	
	//DBG_COL(norm_cam * v3(-1,-1,1));
	//DBG_COL(pow(normal_tang_sample, v3(2.2)));
	
	return norm_cam;
}

$include "skybox.glsl"

void main () {
	vec3 alb = vec4(texture(albedo, vs_uv).rgb, 1).rgb;
	vec3 norm_cam = normal_mapping(vs_pos_cam, vs_norm_cam, vs_tang_cam, vs_uv, normal);
	
	vec3 cam_to_p = normalize(vs_pos_cam);
	vec3 refl_cam = reflect(cam_to_p, norm_cam);
	
	vec4 col = vec4( sky(mat3(cam_to_world) * refl_cam), 1 );
	
	col *= vec4(alb,1) * vs_col;
	
	FRAG_COL(col);
}
