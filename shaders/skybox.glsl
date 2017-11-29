
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
