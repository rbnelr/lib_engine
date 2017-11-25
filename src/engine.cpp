
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

static Vbo_Data<Vertex>	shapes_mesh;
static Vbo_Data<Vertex>	grid_floor;

static void gen_tetrahedron (f32 r, v3 pos_world, m3 ori) {
	
	pos_world += v3(0,0, r * (1.0f/3));
	
	f32 SIN_0	= +0.0f;
	f32 SIN_120	= +0.86602540378443864676372317075294f;
	f32 SIN_240	= -0.86602540378443864676372317075294f;
	f32 COS_0	= +1.0f;
	f32 COS_120	= -0.5f;
	f32 COS_240	= -0.54f;
	
	auto* out = shapes_mesh.append(4*3);
	
	*out++ = { ori * (r*v3(COS_0,	SIN_0,		-1.0f/3))	+pos_world,	v3(1,0,0) };
	*out++ = { ori * (r*v3(COS_240,	SIN_240,	-1.0f/3))	+pos_world,	v3(0,0,1) };
	*out++ = { ori * (r*v3(COS_120,	SIN_120,	-1.0f/3))	+pos_world,	v3(0,1,0) };
	
	*out++ = { ori * (r*v3(COS_0,	SIN_0,		-1.0f/3))	+pos_world,	v3(1,0,0) };
	*out++ = { ori * (r*v3(COS_120,	SIN_120,	-1.0f/3))	+pos_world,	v3(0,1,0) };
	*out++ = { ori * (r*v3(0,		0,			+1.0f)	)	+pos_world,	v3(1,1,1) };
	
	*out++ = { ori * (r*v3(COS_120,	SIN_120,	-1.0f/3))	+pos_world,	v3(0,1,0) };
	*out++ = { ori * (r*v3(COS_240,	SIN_240,	-1.0f/3))	+pos_world,	v3(0,0,1) };
	*out++ = { ori * (r*v3(0,		0,			+1.0f)	)	+pos_world,	v3(1,1,1) };
	
	*out++ = { ori * (r*v3(COS_240,	SIN_240,	-1.0f/3))	+pos_world,	v3(0,0,1) };
	*out++ = { ori * (r*v3(COS_0,	SIN_0,		-1.0f/3))	+pos_world,	v3(1,0,0) };
	*out++ = { ori * (r*v3(0,		0,			+1.0f)	)	+pos_world,	v3(1,1,1) };
	
	dbg_assert(out == (shapes_mesh.data() +shapes_mesh.length())); // check size calculation above
}
static void gen_cube (f32 r, v3 pos_world, m3 ori) {
	
	pos_world += v3(0,0,r);
	
	auto* out = shapes_mesh.append(6*6);
	
	auto quad = [&] (v3 a, v3 b, v3 c, v3 d) {
		*out++ = { ori * (r*b) +pos_world, b/2+0.5f };
		*out++ = { ori * (r*c) +pos_world, c/2+0.5f };
		*out++ = { ori * (r*a) +pos_world, a/2+0.5f };
		*out++ = { ori * (r*a) +pos_world, a/2+0.5f };
		*out++ = { ori * (r*c) +pos_world, c/2+0.5f };
		*out++ = { ori * (r*d) +pos_world, d/2+0.5f };
	};
	
	v3 LLL = v3(-1,-1,-1);
	v3 HLL = v3(+1,-1,-1);
	v3 LHL = v3(-1,+1,-1);
	v3 HHL = v3(+1,+1,-1);
	v3 LLH = v3(-1,-1,+1);
	v3 HLH = v3(+1,-1,+1);
	v3 LHH = v3(-1,+1,+1);
	v3 HHH = v3(+1,+1,+1);
		
	quad(	LLH,
			HLH,
			HHH,
			LHH );
	
	quad(	HLL,
			LLL,
			LHL,
			HHL );
	
	quad(	LHL,
			LLL,
			LLH,
			LHH );
	
	quad(	HLL,
			HHL,
			HHH,
			HLH );
	
	quad(	LLL,
			HLL,
			HLH,
			LLH );
	
	quad(	HHL,
			LHL,
			LHH,
			HHH );
	
	dbg_assert(out == (shapes_mesh.data() +shapes_mesh.length())); // check size calculation above
}
static void gen_cylinder (f32 r, f32 l, u32 faces, v3 pos_world) {
	
	pos_world += v3(0,0,l/2);
	
	auto* out = shapes_mesh.append(faces*(3 +6 +3));
	
	auto quad = [&] (v3 a, v3 b, v3 c, v3 d) {
		*out++ = { v3(r,r,l/2)*b +pos_world, b/2+0.5f };
		*out++ = { v3(r,r,l/2)*c +pos_world, c/2+0.5f };
		*out++ = { v3(r,r,l/2)*a +pos_world, a/2+0.5f };
		*out++ = { v3(r,r,l/2)*a +pos_world, a/2+0.5f };
		*out++ = { v3(r,r,l/2)*c +pos_world, c/2+0.5f };
		*out++ = { v3(r,r,l/2)*d +pos_world, d/2+0.5f };
	};
	auto tri = [&] (v3 a, v3 b, v3 c) {
		*out++ = { v3(r,r,l/2)*a +pos_world, a/2+0.5f };
		*out++ = { v3(r,r,l/2)*b +pos_world, b/2+0.5f };
		*out++ = { v3(r,r,l/2)*c +pos_world, c/2+0.5f };
	};
	
	for (u32 i=0; i<faces; ++i) {
		v2 a = rotate2( deg(360) * ((f32)(i+0) / (f32)faces) ) * v2(1,0);
		v2 b = rotate2( deg(360) * ((f32)(i+1) / (f32)faces) ) * v2(1,0);
		
		tri(	v3(b,	-1),
				v3(a,	-1),
				v3(0,0,	-1) );
		quad(	v3(a,	-1),
				v3(b,	-1),
				v3(b,	+1),
				v3(a,	+1) );
		tri(	v3(a,	+1),
				v3(b,	+1),
				v3(0,0,	+1) );
	}
	
	dbg_assert(out == (shapes_mesh.data() +shapes_mesh.length())); // check size calculation above
}
static void gen_iso_shphere (f32 r, u32 wfaces, u32 hfaces, v3 pos_world) {
	
	if (wfaces < 2 || hfaces < 2) return;
	
	pos_world += v3(0,0,r);
	
	auto* out = shapes_mesh.append((hfaces-2)*wfaces*6 +2*wfaces*3);
	
	auto quad = [&] (v3 a, v3 b, v3 c, v3 d) {
		*out++ = { r*b +pos_world, b/2+0.5f };
		*out++ = { r*c +pos_world, c/2+0.5f };
		*out++ = { r*a +pos_world, a/2+0.5f };
		*out++ = { r*a +pos_world, a/2+0.5f };
		*out++ = { r*c +pos_world, c/2+0.5f };
		*out++ = { r*d +pos_world, d/2+0.5f };
	};
	auto tri = [&] (v3 a, v3 b, v3 c) {
		*out++ = { r*a +pos_world, a/2+0.5f };
		*out++ = { r*b +pos_world, b/2+0.5f };
		*out++ = { r*c +pos_world, c/2+0.5f };
	};
	
	for (u32 j=0; j<hfaces; ++j) {
		
		m3 rot_ha = rotate3_Y( deg(180) * ((f32)(j+0) / (f32)hfaces) );
		m3 rot_hb = rotate3_Y( deg(180) * ((f32)(j+1) / (f32)hfaces) );
		
		for (u32 i=0; i<wfaces; ++i) {
			m3 rot_wa = rotate3_Z( deg(360) * ((f32)(i+0) / (f32)wfaces) );
			m3 rot_wb = rotate3_Z( deg(360) * ((f32)(i+1) / (f32)wfaces) );
			
			if (j == 0) {
				tri(	v3(0,0,1),
						rot_wa * rot_hb * v3(0,0,1),
						rot_wb * rot_hb * v3(0,0,1) );
			} else if (j == (hfaces -1)) {
				tri(	rot_wb * rot_ha * v3(0,0,1),
						rot_wa * rot_ha * v3(0,0,1),
						v3(0,0,-1) );
			} else {
				quad(	rot_wb * rot_ha * v3(0,0,1),
						rot_wa * rot_ha * v3(0,0,1),
						rot_wa * rot_hb * v3(0,0,1),
						rot_wb * rot_hb * v3(0,0,1) );
			}
		}
	}
	
	dbg_assert(out == (shapes_mesh.data() +shapes_mesh.length())); // check size calculation above
}

static void gen_shapes () {
	
	shapes_mesh.clear();
	
	gen_cube(			1,						v3( 0, 0,0), rotate3_Z(deg(37)));
	gen_tetrahedron(	2.0f / (1 +1.0f/3),		v3(+4, 0,0), rotate3_Z(deg(13)));
	gen_cylinder(		1, 2, 24, 				v3(-4, 0,0));
	gen_iso_shphere(	1, 64, 32,				v3(-4,-4,0));
	
}

static void gen_grid_floor () {
	
	f32	Z = 0;
	f32	tile_dim = 1;
	iv2	floor_r = 16;
	
	grid_floor.clear();
	auto* out = grid_floor.append(floor_r.y*2 * floor_r.x*2 * 6);
	
	auto emit_quad = [&] (v3 pos, v3 col) {
		*out++ = { pos +v3(+0.5f,-0.5f,0), col };
		*out++ = { pos +v3(+0.5f,+0.5f,0), col };
		*out++ = { pos +v3(-0.5f,-0.5f,0), col };
		
		*out++ = { pos +v3(-0.5f,-0.5f,0), col };
		*out++ = { pos +v3(+0.5f,+0.5f,0), col };
		*out++ = { pos +v3(-0.5f,+0.5f,0), col };
	};
	
	for (s32 y=0; y<(floor_r.y*2); ++y) {
		for (s32 x=0; x<(floor_r.x*2); ++x) {
			v3 col = BOOL_XOR(EVEN(x), EVEN(y)) ? srgb(224,226,228) : srgb(41,49,52);
			emit_quad( v3(+0.5f,+0.5f,Z) +v3((f32)(x -floor_r.x)*tile_dim, (f32)(y -floor_r.y)*tile_dim, 0), col );
		}
	}
	
	dbg_assert(out == (grid_floor.data() +grid_floor.length())); // check size calculation above
}

static Vbo			vbo_shapes;
static Vbo			vbo_floor;

static Shader		shad;

//
static f32			dt = 0;

struct Camera {
	v3 pos_world;
	v2 ori_ae; // azimuth elevation
};

static Camera default_camera = {	v3(0, -5, 1),
									v2(deg(0), deg(+80)) };

static Camera		cam;

#define SAVE_FILE	"saves/camera_view.bin"

static void load_game () {
	bool loaded = read_entire_file(SAVE_FILE, &cam, sizeof(cam));
	if (loaded) {
		printf("camera_view loaded from \"" SAVE_FILE "\".\n");
	} else {
		cam = default_camera;
		printf("camera_view could not be loaded from \"" SAVE_FILE "\", using defaults.\n");
	}
}
static void save_game () {
	bool saved = overwrite_file(SAVE_FILE, &cam, sizeof(cam));
	if (saved) {
		printf("camera_view saved to \"" SAVE_FILE "\".\n");
	} else {
		log_warning("could not write \"" SAVE_FILE "\", camera_view wont be loaded on next launch.");
	}
}

static void glfw_key_event (GLFWwindow* window, int key, int scancode, int action, int mods) {
	dbg_assert(action == GLFW_PRESS || action == GLFW_RELEASE || action == GLFW_REPEAT);
	
	bool went_down =	action == GLFW_PRESS;
	bool went_up =		action == GLFW_RELEASE;
	
	bool repeated =		!went_down && !went_up; // GLFW_REPEAT
	
	bool alt =			(mods & GLFW_MOD_ALT) != 0;
	
	if (repeated) {
		
	} else {
		if (!alt) {
			switch (key) {
				case GLFW_KEY_F11:			if (went_down) {		toggle_fullscreen(); }	break;
				
				//
				case GLFW_KEY_A:			inp.cam_dir.x -= went_down ? +1 : -1;		break;
				case GLFW_KEY_D:			inp.cam_dir.x += went_down ? +1 : -1;		break;
				
				case GLFW_KEY_S:			inp.cam_dir.z += went_down ? +1 : -1;		break;
				case GLFW_KEY_W:			inp.cam_dir.z -= went_down ? +1 : -1;		break;
				
				case GLFW_KEY_LEFT_CONTROL:	inp.cam_dir.y -= went_down ? +1 : -1;		break;
				case GLFW_KEY_SPACE:		inp.cam_dir.y += went_down ? +1 : -1;		break;
			}
		} else {
			switch (key) {
				
				case GLFW_KEY_ENTER:		if (alt && went_down) {	toggle_fullscreen(); }	break;
				
				//
				case GLFW_KEY_S:			if (alt && went_down) {	save_game(); }			break;
				case GLFW_KEY_L:			if (alt && went_down) {	load_game(); }			break;
			}
		}
	}
}
static void glfw_mouse_button_event (GLFWwindow* window, int button, int action, int mods) {
    switch (button) {
		case GLFW_MOUSE_BUTTON_RIGHT:
			if (action == GLFW_PRESS) {
				start_mouse_look();
			} else {
				stop_mouse_look();
			}
			break;
	}
}

int main (int argc, char** argv) {
	
	cstr app_name = "lib_engine";
	
	platform_setup_context_and_open_window(app_name, iv2(1280, 720));
	
	//
	load_game();
	
	//
	set_vsync(1);
	
	{ // GL state
		glEnable(GL_FRAMEBUFFER_SRGB);
		
		glEnable(GL_DEPTH_TEST);
		glClearDepth(1.0f);
		glDepthFunc(GL_LEQUAL);
		glDepthRange(0.0f, 1.0f);
		glDepthMask(GL_TRUE);
		
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glFrontFace(GL_CCW);
	}
	
	vbo_shapes.gen();
	vbo_floor.gen();
	
	shad.load();
	
	//load_mesh("");
	
	// 
	f64 prev_t = glfwGetTime();
	f32 avg_dt = 1.0f / 60;
	f32 abg_dt_alpha = 0.025f;
	
	for (u32 frame_i=0;; ++frame_i) {
		
		{ //
			f32 fps = 1.0f / dt;
			f32 dt_ms = dt * 1000;
			
			f32 avg_fps = 1.0f / avg_dt;
			f32 avg_dt_ms = avg_dt * 1000;
			
			//printf("frame #%5d %6.1f fps %6.2f ms  avg: %6.1f fps %6.2f ms\n", frame_i, fps, dt_ms, avg_fps, avg_dt_ms);
			glfwSetWindowTitle(wnd, prints("%s %6d  %6.1f fps avg %6.2f ms avg", app_name, frame_i, avg_fps, avg_dt_ms).c_str());
		}
		
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
		
		v4 clear_color = v4(srgb(41,49,52)*3, 1);
		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		
		m4 world_to_clip;
		{
			{
				v2 mouse_look_sens = v2(deg(1.0f / 8));
				cam.ori_ae -= mouse_look_diff * mouse_look_sens;
				cam.ori_ae.x = mymod(cam.ori_ae.x, deg(360));
				cam.ori_ae.y = clamp(cam.ori_ae.y, deg(2), deg(180.0f -2));
				
				//printf(">>> %f %f\n", to_deg(camera_ae.x), to_deg(camera_ae.y));
			}
			m3 world_to_cam_rot = rotate3_X(-cam.ori_ae.y) * rotate3_Z(-cam.ori_ae.x);
			m3 cam_to_world_rot = rotate3_Z(cam.ori_ae.x) * rotate3_X(cam.ori_ae.y);
			
			{
				f32 cam_vel = 6;
				
				v3 cam_vel_cam = normalize_or_zero( (v3)inp.cam_dir ) * cam_vel;
				cam.pos_world += (cam_to_world_rot * cam_vel_cam) * dt;
				
				//printf(">>> %f %f %f\n", cam_vel_cam.x, cam_vel_cam.y, cam_vel_cam.z);
			}
			m4 world_to_cam = world_to_cam_rot * translate4(-cam.pos_world);
			
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
			
			world_to_clip = cam_to_clip * world_to_cam;
		}
		
		gen_shapes();
		gen_grid_floor();
		
		vbo_shapes.upload(shapes_mesh);
		vbo_floor.upload(grid_floor);
		
		
		shad.bind();
		shad.world_to_clip.set(world_to_clip);
		
		vbo_shapes.bind(shad);
		glDrawArrays(GL_TRIANGLES, 0, shapes_mesh.length());
		
		vbo_floor.bind(shad);
		glDrawArrays(GL_TRIANGLES, 0, grid_floor.length());
		
		glfwSwapBuffers(wnd);
		
		{
			f64 now = glfwGetTime();
			dt = now -prev_t;
			prev_t = now;
			
			avg_dt = lerp(avg_dt, dt, abg_dt_alpha);
		}
	}
	
	platform_terminate();
	
	return 0;
}
