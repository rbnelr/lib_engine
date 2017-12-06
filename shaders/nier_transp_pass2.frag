#version 150 core // version 3.2

#define ALPHA_TEST 0
#define SHOW_ALPHA_TEST_HACK if (alb.a != 0) DBG_COL(vec3(1,0,0));
$include "nier.glsl"
