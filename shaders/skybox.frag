#version 150 core // version 3.2

in		vec3	vs_pos_world_rot;

out		vec3	frag_col;

uniform	vec2	mcursor_pos;
uniform	vec2	screen_dim;

vec2 mouse () {		return mcursor_pos / screen_dim; }
vec2 screen () {	return gl_FragCoord.xy / screen_dim; }

//mat3 rotate3_X (float ang) {
//	
//}
//auto sc = sin_cos(ang);
//	return M3::row(	1,		0,		0,
//					0,		+sc.c,	-sc.s,
//					0,		+sc.s,	+sc.c);
//}
//static M3 rotate3_Y (T ang) {
//	auto sc = sin_cos(ang);
//	return M3::row(	+sc.c,	0,		+sc.s,
//					0,		1,		0,
//					-sc.s,	0,		+sc.c);
//}
//static M3 rotate3_Z (T ang) {
//	auto sc = sin_cos(ang);
//	return M3::row(	+sc.c,	-sc.s,	0,
//					+sc.s,	+sc.c,	0,
//					0,		0,		1);

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
		
		//vec3 sun_dir = rotate3_Z(deg()) * rotate3_X(deg()) * vec3(0,1,0));
		//float sun_ang = acos( dot(dir_world, sun_dir) );
		//
		//return vec3(sun_ang);
		
		//
		fog = pow(clamp(fog, 0, 1), 1.0/8);
		
		sky_color = mix(sky_color, fog_color, pow(fog, 32));
		ground_color = mix(ground_color, fog_color, pow(fog, 32));
		
		if (dir_world.z < 0) {
			return mix(fog_color, ground_color, pow(-dir_world.z, 1.0/mix(32, 1, fog)));
		} else {
			return mix(fog_color, sky_color, pow(+dir_world.z, 1.0/mix(16, 0.5, fog)));
		}
	}
}

void main () {
	frag_col = sky(normalize(vs_pos_world_rot));
}
