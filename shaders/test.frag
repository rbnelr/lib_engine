#version 150 core // version 3.2

in		vec3	vs_col;
out		vec3	frag_col;

void main () {
	frag_col = vs_col;
}
