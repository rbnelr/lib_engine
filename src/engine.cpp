
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
#include "mesh_loader.hpp"
#include "shapes.hpp"

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
	
	//
	Mesh_Vbo	vbo_shapes;
	Mesh_Vbo	vbo_floor;
	Mesh_Vbo	vbo_cerberus;
	
	Shader		shad;
	
	vbo_shapes.gen();
	vbo_floor.gen();
	vbo_cerberus.gen();
	
	shad.load();
	
	load_mesh(&vbo_cerberus.data, "cerberus/cerberus.obj", v3(0,0,2));
	
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
				v3 cam_vel = 4*v3(1,2.0f/3,1);
				
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
		
		{ // Draw all
			gen_shapes(&vbo_shapes.data);
			gen_grid_floor(&vbo_floor.data);
			
			vbo_shapes.upload();
			vbo_floor.upload();
			vbo_cerberus.upload();
			
			
			shad.bind();
			shad.world_to_clip.set(world_to_clip);
			
			vbo_shapes.bind(shad);
			glDrawArrays(GL_TRIANGLES, 0, vbo_shapes.data.size());
			
			vbo_floor.bind(shad);
			glDrawArrays(GL_TRIANGLES, 0, vbo_floor.data.size());
			
			vbo_cerberus.bind(shad);
			glDrawArrays(GL_TRIANGLES, 0, vbo_cerberus.data.size());
		}
		
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
