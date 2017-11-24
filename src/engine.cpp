
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

#include "platform.hpp"
#include "gl.hpp"

#define SIN_0	+0.0f
#define SIN_120	+0.86602540378443864676372317075294f
#define SIN_240	-0.86602540378443864676372317075294f
#define COS_0	+1.0f
#define COS_120	-0.5f
#define COS_240	-0.54f

static constexpr f32	tetrahedron_r = 1;
static constexpr v3		tetrahedron_pos = v3(0,0,+1.0f/3);

static Vertex			tetrahedron[4*3] = {
	{ tetrahedron_r*v3(COS_0,	SIN_0,		-1.0f/3)	+tetrahedron_pos,	v3(1,0,0) },
	{ tetrahedron_r*v3(COS_240,	SIN_240,	-1.0f/3)	+tetrahedron_pos,	v3(0,0,1) },
	{ tetrahedron_r*v3(COS_120,	SIN_120,	-1.0f/3)	+tetrahedron_pos,	v3(0,1,0) },
	
	{ tetrahedron_r*v3(COS_0,	SIN_0,		-1.0f/3)	+tetrahedron_pos,	v3(1,0,0) },
	{ tetrahedron_r*v3(COS_120,	SIN_120,	-1.0f/3)	+tetrahedron_pos,	v3(0,1,0) },
	{ tetrahedron_r*v3(0,		0,			+1.0f)		+tetrahedron_pos,	v3(1,1,1) },
	
	{ tetrahedron_r*v3(COS_120,	SIN_120,	-1.0f/3)	+tetrahedron_pos,	v3(0,1,0) },
	{ tetrahedron_r*v3(COS_240,	SIN_240,	-1.0f/3)	+tetrahedron_pos,	v3(0,0,1) },
	{ tetrahedron_r*v3(0,		0,			+1.0f)		+tetrahedron_pos,	v3(1,1,1) },
	
	{ tetrahedron_r*v3(COS_240,	SIN_240,	-1.0f/3)	+tetrahedron_pos,	v3(0,0,1) },
	{ tetrahedron_r*v3(COS_0,	SIN_0,		-1.0f/3)	+tetrahedron_pos,	v3(1,0,0) },
	{ tetrahedron_r*v3(0,		0,			+1.0f)		+tetrahedron_pos,	v3(1,1,1) },
};

static constexpr f32	floor_Z = 0;
static constexpr f32	tile_dim = 1;
static constexpr iv2	floor_r = iv2(16,16);

static Vertex grid_floor[floor_r.y*2 * floor_r.x*2 * 6];

static void gen_grid_floor () {
	auto emit_quad = [] (Vertex* out, v3 pos, v3 col) {
		out[0] = { pos +v3(+0.5f,-0.5f,0), col };
		out[1] = { pos +v3(+0.5f,+0.5f,0), col };
		out[2] = { pos +v3(-0.5f,-0.5f,0), col };
		
		out[3] = { pos +v3(-0.5f,-0.5f,0), col };
		out[4] = { pos +v3(+0.5f,+0.5f,0), col };
		out[5] = { pos +v3(-0.5f,+0.5f,0), col };
	};
	auto index = [] (s32 x, s32 y) -> Vertex* {
		return grid_floor +(floor_r.x*2)*6*y +6*x;
	};
	
	for (s32 y=0; y<(floor_r.y*2); ++y) {
		for (s32 x=0; x<(floor_r.x*2); ++x) {
			v3 col = BOOL_XOR(EVEN(x), EVEN(y)) ? srgb(224,226,228) : srgb(41,49,52);
			emit_quad( index(x,y), v3(+0.5f,+0.5f,floor_Z) +v3((f32)(x -floor_r.x)*tile_dim, (f32)(y -floor_r.y)*tile_dim, 0), col );
		}
	}
}

static Vbo			vbo_shapes;
static Vbo			vbo_floor;

static Shader		shad;

static void setup_gl () {
	glEnable(GL_FRAMEBUFFER_SRGB);
	
	glEnable(GL_DEPTH_TEST);
	glClearDepth(1.0f);
	glDepthFunc(GL_LEQUAL);
	glDepthRange(0.0f, 1.0f);
	glDepthMask(GL_TRUE);
	
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	
	vbo_shapes.gen();
	vbo_floor.gen();
	
	gen_grid_floor();
	
	vbo_shapes.upload(tetrahedron);
	vbo_floor.upload(grid_floor);
	
	shad.load();
	
}

int main (int argc, char** argv) {
	
	platform_setup_context();
	
	set_vsync(1);
	
	setup_gl();
	
	f32 dt = 0;
	f64 prev_t = glfwGetTime();
	
	v3 cam_pos_world =	v3(0, -5, 1);
	v2 camera_ae =		v2(deg(0), deg(+80)); // azimuth elevation
	
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
		v2 mouse_look_diff;
		{
			mouse_look_diff = inp.mouse_look_diff;
			inp.mouse_look_diff = 0;
		}
		
		//
		glViewport(0, 0, wnd_dim.x, wnd_dim.y);
		
		v4 clear_color = v4(srgb(41,49,52), 1);
		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		
		shad.bind();
		{
			{
				v2 mouse_look_sens = v2(deg(1.0f / 10));
				camera_ae -= mouse_look_diff * mouse_look_sens;
				camera_ae.x = mymod(camera_ae.x, deg(360));
				camera_ae.y = clamp(camera_ae.y, deg(2), deg(180.0f -2));
				
				//printf(">>> %f %f\n", to_deg(camera_ae.x), to_deg(camera_ae.y));
			}
			m3 world_to_cam_rot = rotate3_X(-camera_ae.y) * rotate3_Z(-camera_ae.x);
			m3 cam_to_world_rot = rotate3_Z(camera_ae.x) * rotate3_X(camera_ae.y);
			
			{
				f32 cam_vel = 6;
				
				v3 cam_vel_cam = normalize_or_zero( (v3)inp.cam_dir ) * cam_vel;
				cam_pos_world += (cam_to_world_rot * cam_vel_cam) * dt;
				
				//printf(">>> %f %f %f\n", cam_vel_cam.x, cam_vel_cam.y, cam_vel_cam.z);
			}
			m4 world_to_cam = world_to_cam_rot * translate4(-cam_pos_world);
			
			m4 cam_to_clip;
			{
				f32 vfov =			deg(70);
				f32 clip_near =		1.0f/16;
				f32 clip_far =		512;
				
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
			
			shad.world_to_clip.set(world_to_clip);
		}
		vbo_shapes.bind(shad);
		glDrawArrays(GL_TRIANGLES, 0, ARRLEN(tetrahedron));
		
		vbo_floor.bind(shad);
		glDrawArrays(GL_TRIANGLES, 0, ARRLEN(grid_floor));
		
		glfwSwapBuffers(wnd);
		
		{
			f64 now = glfwGetTime();
			dt = now -prev_t;
			prev_t = now;
		}
	}
	
	glfwDestroyWindow(wnd);
	glfwTerminate();
	
	return 0;
}
