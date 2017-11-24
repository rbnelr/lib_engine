#version 330 // version 3.3

varying		vec3	vs_col;

void main () {
	gl_FragColor = vec4(vs_col, 1.0);
}
