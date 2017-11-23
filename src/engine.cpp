
#include <cstdio>
#include <array>
#include <vector>
#include <string>

#include "types.hpp"
#include "lang_helpers.hpp"
#include "math.hpp"
#include "vector/vector.hpp"

typedef s32v2	iv2;
typedef s32v3	iv3;
typedef s32v4	iv4;
typedef fv2		v2;
typedef fv3		v3;
typedef fv4		v4;
typedef fm2		m2;
typedef fm3		m3;
typedef fm4		m4;

#define _USING_V110_SDK71_ 1
#include "glad.c"
#include "GLFW/glfw3.h"

#include "platform_graphics.hpp"

struct Vertex {
	v3	pos;
	v3	col;
};

static Vertex tetrahedron[1*3] = {
	{ rotate3_Z(deg(   0)) * v3(1,0,0), v3(1,0,0) },
	{ rotate3_Z(deg(-120)) * v3(1,0,0), v3(0,1,0) },
	{ rotate3_Z(deg(-240)) * v3(1,0,0), v3(0,0,1) },
};

static const char* shad_vert = R"_SHAD(
	attribute	vec3	col;
	attribute	vec3	pos;
	varying		vec3	vs_col;
	
	uniform		mat4	MVP;
	
	void main() {
		gl_Position = MVP * vec4(pos,1);
		vs_col = col;
	}
)_SHAD";
static const char* shad_frag = R"_SHAD(
	varying		vec3	vs_col;
	
	void main() {
		gl_FragColor = vec4(vs_col, 1.0);
	}
)_SHAD";

static GLuint		vertex_buffer;
static GLuint		vertex_shader;
static GLuint		fragment_shader;
static GLuint		program;

static GLint		mvp_location;
static GLint		vpos_location;
static GLint		vcol_location;

static void setup_gl () {
	glGenBuffers(1, &vertex_buffer);
	
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(tetrahedron), tetrahedron, GL_STATIC_DRAW);
	
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &shad_vert, NULL);
	glCompileShader(vertex_shader);
	
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &shad_frag, NULL);
	glCompileShader(fragment_shader);
	
	program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);
	
	mvp_location = glGetUniformLocation(program, "MVP");
	vpos_location = glGetAttribLocation(program, "pos");
	vcol_location = glGetAttribLocation(program, "col");
	
	glEnableVertexAttribArray(vpos_location);
	glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
	
	glEnableVertexAttribArray(vcol_location);
	glVertexAttribPointer(vcol_location, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, col));
	
}

int main (int argc, char** argv) {
	
	setup_graphics_context();
	
	set_vsync(1);
	
	setup_gl();
	
	for (;;) {
		
		glfwPollEvents();
		
		if (glfwWindowShouldClose(wnd)) break;
		
		iv2 wnd_dim;
		v2	wnd_dim_aspect;
		{
			glfwGetFramebufferSize(wnd, &wnd_dim.x, &wnd_dim.y);
			
			v2 tmp = (v2)wnd_dim;
			wnd_dim_aspect = tmp / v2(tmp.y, tmp.x);
		}
		
		glViewport(0, 0, wnd_dim.x, wnd_dim.y);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		
		glUseProgram(program);
		{
			v3 cam_pos_world = v3(0,0, 5);
			m4 world_to_cam = translate4(-cam_pos_world);
			
			m4 cam_to_clip;
			{
				f32 vfov =			deg(70);
				f32 clip_near =		deg(1.0f/16);
				f32 clip_far =		deg(512);
				
				v2 frust_scale;
				frust_scale.y = tan(vfov / 2);
				frust_scale.x = frust_scale.y * wnd_dim_aspect.x;
				
				v2 frust_scale_inv = 1.0f / frust_scale;
				
				f32 x = frust_scale_inv.x;
				f32 y = frust_scale_inv.y;
				f32 a = (clip_far +clip_near) / (clip_near -clip_far);
				f32 b = (2.0f * clip_far * clip_near) / (clip_near -clip_far);
				
				cam_to_clip = m4::row(
								x, 0, 0, 0,
								0, y, 0, 0,
								0, 0, a, b,
								0, 0, -1, 0 );
			}
			
			m4 world_to_clip = cam_to_clip * world_to_cam;
			
			glUniformMatrix4fv(mvp_location, 1, GL_FALSE, &world_to_clip.arr[0][0]);
		}
		glDrawArrays(GL_TRIANGLES, 0, ARRLEN(tetrahedron));
		
		glfwSwapBuffers(wnd);
		
	}
	
	glfwDestroyWindow(wnd);
	glfwTerminate();
	
	return 0;
}
