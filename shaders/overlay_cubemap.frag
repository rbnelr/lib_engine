#version 150 core // version 3.2

$include "common.glsl"

in		vec2	vs_uv;

uniform samplerCube	tex0;

#define PI 3.1415926535897932384626433832795

const mat3 Z_UP_CONVENTION_TO_OPENGL_CUBEMAP_CONVENTION = mat3(
	vec3(+1, 0, 0),
	vec3( 0, 0,+1),
	vec3( 0,-1, 0) );	// my z-up convention:			+x = right,		+y = forward,	+z = up
						// opengl cubemap convention:	+x = left,		+y = down,		+z = forward

vec3 uv_to_cubemap_dir (vec2 uv) {
	return Z_UP_CONVENTION_TO_OPENGL_CUBEMAP_CONVENTION * vec3(
		sin(uv.x * 2*PI -PI) * cos(uv.y * PI -PI/2),
		cos(uv.x * 2*PI -PI) * cos(uv.y * PI -PI/2),
		sin(uv.y * PI -PI/2) );
}

void main () {
	vec3 dir = uv_to_cubemap_dir(vs_uv);
	
	//if (max(max(abs(dir.x), abs(dir.y)), abs(dir.z)) != +dir.z) DBG_COL(vec3(1,0,0));
	FRAG_COL( texture(tex0, dir).rgba );
	//FRAG_COL( (uv_to_cubemap_dir(vs_uv) / 2 +0.5).zzz );
}
