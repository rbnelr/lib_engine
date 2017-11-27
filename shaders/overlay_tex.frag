#version 150 core // version 3.2

in		vec2	vs_uv;

out		vec4	frag_col;

uniform	vec2	mcursor_pos;
uniform	vec2	screen_dim;

uniform sampler2D	tex0;

vec2 mouse () {		return mcursor_pos / screen_dim; }
vec2 screen () {	return gl_FragCoord.xy / screen_dim; }

void main () {
	frag_col = texture(tex0, vs_uv).rgba;
}
