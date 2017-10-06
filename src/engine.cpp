
#define _USING_V110_SDK71_ 1
#include "windows.h"

#include "types.hpp"
#include "lang_helpers.hpp"
#include "assert.hpp"
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

#include <cstdio>

#include "glad/glad.c"
#include "GLFW/glfw3.h"

struct Vertex {
	v2	pos;
	v3	col;
};

static constexpr Vertex vertices[3] = {
	{ v2(-0.6f, -0.4f), v3(1,0,0) },
	{ v2(+0.6f, -0.4f), v3(0,1,0) },
	{ v2(+0.0f, +0.6f), v3(0,0,1) },
};

#if 0
struct Uniform {
	cstr	name;
	GLint	location;
};
struct Shader {
	Uniform*	uniforms;
	cstr		vert_src;
	cstr		frag_src;
	GLuint		gl_h;
};
#endif

static const char* shad_vert = R"_SHAD(
	uniform		mat4	MVP;
	attribute	vec3	vCol;
	attribute	vec2	vPos;
	varying		vec3	color;
	
	void main() {
		gl_Position = MVP * vec4(vPos, 0.0, 1.0);
		color = vCol;
	}
)_SHAD";
static const char* shad_frag = R"_SHAD(
	varying		vec3	color;
	
	void main() {
		gl_FragColor = vec4(color, 1.0);
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
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	
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
	vpos_location = glGetAttribLocation(program, "vPos");
	vcol_location = glGetAttribLocation(program, "vCol");
	
	glEnableVertexAttribArray(vpos_location);
	glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*) 0);
	
	glEnableVertexAttribArray(vcol_location);
	glVertexAttribPointer(vcol_location, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*) (sizeof(float) * 2));
	
}

static GLFWwindow*	wnd;

static void draw_frame () {
	
	iv2 wnd_dim;
	v2	wnd_dim_aspect;
	{
		glfwGetFramebufferSize(wnd, &wnd_dim.x, &wnd_dim.y);
		
		v2 tmp = cast(v2, wnd_dim);
		wnd_dim_aspect = tmp / v2(tmp.y, tmp.x);
	}
	
	glViewport(0, 0, wnd_dim.x, wnd_dim.y);
	glClear(GL_COLOR_BUFFER_BIT);
	
	f32 t = (f32)glfwGetTime();
	m4	MVP = m4::row(	wnd_dim_aspect.y,0,0,0,
						0,1,0,0,
						0,0,1,0,
						0,0,0,1	);
	MVP *= rotate4_Z(t * deg(30.0f));
	
	glUseProgram(program);
	glUniformMatrix4fv(mvp_location, 1, GL_FALSE, &MVP.arr[0][0]);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	
	glfwSwapBuffers(wnd);
	
}

static void glfw_error_proc(int err, const char* msg) {
	fprintf(stderr, ANSI_COLOUR_CODE_RED "GLFW Error! 0x%x '%s'\n" ANSI_COLOUR_CODE_NC, err, msg);
}
static void wnd_refresh_proc (GLFWwindow* wnd) { // to keep drawing while resizing window, does not work for moving, though
	draw_frame();
}

int main (int argc, char** argv) {
	
	glfwSetErrorCallback(glfw_error_proc);
	
	dbg_assert( glfwInit(), "Hello %s", "world" );
	
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,	3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,	1);
	//glfwWindowHint(GLFW_OPENGL_PROFILE,			GLFW_OPENGL_CORE_PROFILE);
	
	wnd = glfwCreateWindow(1280, 720, "GLFW test", NULL, NULL);
	dbg_assert(wnd);
	
	glfwSetWindowRefreshCallback(wnd, wnd_refresh_proc);
	
	glfwMakeContextCurrent(wnd);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	
	glfwSwapInterval(1);
	
	setup_gl();
	
	for (;;) {
		
		glfwPollEvents();
		
		if (glfwWindowShouldClose(wnd)) break;
		
		draw_frame();
		
	}
	
	glfwDestroyWindow(wnd);
	glfwTerminate();
	
	return 0;
}
