#version 150 core // version 3.2

in		vec3	vs_pos_cam;
in		vec3	vs_norm_cam;
in		vec4	vs_tang_cam;
in		vec2	vs_uv;
in		vec4	vs_col;

out		vec4	frag_col;

uniform	vec2	mcursor_pos;
uniform	vec2	screen_dim;

bool dbg_out_written = false;
vec4 dbg_out = vec4(1,0,1,1); // complains about maybe used uninitialized

vec2 mouse () {		return mcursor_pos / screen_dim; }
vec2 screen () {	return gl_FragCoord.xy / screen_dim; }

bool split_horizon = false;
bool split_vertial = false;

bool _split_left () {
	split_vertial = true;
	return gl_FragCoord.x < mcursor_pos.x;
}
bool _split_top () {
	split_horizon = true;
	return gl_FragCoord.y > mcursor_pos.y;
}

#define SPLIT_LEFT		_split_left()
#define SPLIT_TOP		_split_top()
#define SPLIT_RIGHT		!_split_left()
#define SPLIT_BOTTOM	!_split_top()

void DBG_COL (vec4 col) {
	dbg_out_written = true;
	dbg_out = col;
}
void DBG_COL (vec3 col) {
	DBG_COL(vec4(col, 1));
}
void FRAG_COL (vec4 col) {
	if (dbg_out_written) col = dbg_out;
	
	if (split_vertial && distance(gl_FragCoord.x, mcursor_pos.x) < 1) {
		col.rgb = mix(col.rgb, vec3(0.9,0.9,0.1), 0.7);
	}
	if (split_horizon && distance(gl_FragCoord.y, mcursor_pos.y) < 1) {
		col.rgb = mix(col.rgb, vec3(0.9,0.9,0.1), 0.7);
	}
	
	frag_col = col;
}
void FRAG_COL (vec3 col) {
	FRAG_COL(vec4(col, 1));
}

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

#define PI			3.1415926535897932384626433832795
#define DEG_TO_RAD	PI/180.0

float deg (float deg) {
	return deg * DEG_TO_RAD;
}

mat3 rotate3_X (float ang) {
	float s = sin(ang);
	float c = cos(ang);
	return mat3(	1,	0,	0,
					0,	+c,	+s,
					0,	-s,	+c );
}
mat3 rotate3_Y (float ang) {
	float s = sin(ang);
	float c = cos(ang);
	return mat3(	-c,	0,	-s,
					0,	1,	0,
					+s,	0,	+c );
}
mat3 rotate3_Z (float ang) {
	float s = sin(ang);
	float c = cos(ang);
	return mat3(	+c,	+s,	0,
					-s,	+c,	0,
					0,	0,	1 );
}

/*
vec3 hash3( vec2 p ) {
    vec3 q = vec3( dot(p,vec2(127.1,311.7)), 
                   dot(p,vec2(269.5,183.3)), 
                   dot(p,vec2(419.2,371.9)) );
    return fract(sin(q)*43758.5453);
}*/

vec3 sky (vec3 dir_world) {
	if (false) {
		return dir_world * 0.5 +0.5;
	} else {
		
		vec3 fog_color =	vec3(0.85);
		float fog = 0.4; // [0,1]
		//fog = mouse().x;
		
		vec3 ground_color =		vec3(0.2,0.5,0.25)*0.4; // grass
		//vec3 ground_color =		vec3(0.1,0.2,0.5); // water
		
		vec3 sky_color =		vec3(0.4,0.4,0.6);
		//vec3 sky_color =		vec3(0.4,0.4,0.6)*0.025;
		//fog += 0.2;
		//fog_color *= 0.015;
		//ground_color *= 0.025;
		
		//if (hash3(dir_world.xy).x > 0.995) sky_color += vec3(1);
		
		vec2 sun_ae = vec2(deg(-25), deg(40));
		//vec2 sun_ae = vec2(deg(-25), deg(-40));
		
		vec3 sun_dir = rotate3_Z(sun_ae.x) * rotate3_X(sun_ae.y) * vec3(0,1,0);
		vec3 sun_color = vec3(1,0.9,0.4) * 1;
		
		float sun_ang = acos( dot(dir_world, sun_dir) );
		
		if (sun_ang <= deg(3)) {
			sky_color += sun_color;
		} else {
			sky_color += sun_color * vec3( pow(max( (deg(8) -(sun_ang -deg(3)))/deg(8), 0), 2) );
		}
		
		//
		fog = pow(clamp(fog, 0, 1), 1.0/8);
		
		sky_color = mix(sky_color, fog_color, pow(fog, 32));
		ground_color = mix(ground_color, fog_color, pow(fog, 32));
		
		if (dir_world.z < 0) {
			return ground_color +mix(fog_color, vec3(0), pow(-dir_world.z, 1.0/mix(32, 1, fog)));
		} else {
			return sky_color+ mix(fog_color, vec3(0), pow(+dir_world.z, 1.0/mix(16, 0.5, fog)));
		}
	}
}

void main () {
	vec3 alb = vec4(texture(albedo, vs_uv).rgb, 1).rgb;
	vec3 norm_cam = normal_mapping(vs_pos_cam, vs_norm_cam, vs_tang_cam, vs_uv, normal);
	
	vec3 cam_to_p = normalize(vs_pos_cam);
	vec3 refl_cam = reflect(cam_to_p,
			SPLIT_LEFT ? normalize(vs_norm_cam) :
			norm_cam );
	
	vec4 col = vec4( sky(mat3(cam_to_world) * refl_cam), 1 );
	
	col *= vec4(alb,1) * vs_col;
	
	//if (SPLIT_RIGHT)				DBG_COL( norm_cam );
	//else							DBG_COL( vs_norm_cam );
	
	//if (SPLIT_LEFT)					DBG_COL( vec3( normalize( texture(normal, vs_uv).rgb * 2 - 1).rg, 0) );
	
	//if (SPLIT_TOP)					DBG_COL( texture(normal, vs_uv).rgba );
	
	//if (SPLIT_TOP && SPLIT_RIGHT)	DBG_COL( vs_tang_cam.xyz );
	//if (SPLIT_LEFT)		DBG_COL( vs_tang_cam.www );
	
	FRAG_COL(col);
}
