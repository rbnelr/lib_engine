
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
typedef u32v2	uv2;
typedef u32v3	uv3;
typedef u32v4	uv4;
typedef fv2		v2;
typedef fv3		v3;
typedef fv4		v4;
typedef fm2		m2;
typedef fm3		m3;
typedef fm4		m4;
typedef fhm		hm;

#define _USING_V110_SDK71_ 1
#include "glad.c"
#include "GLFW/glfw3.h"

#if RZ_DEV
	#define RZ_AUTO_FILE_RELOADING 1
#else
	#define RZ_AUTO_FILE_RELOADING 0
#endif

struct File_Change_Poller {
	HANDLE		fh;
	FILETIME	last_change_t;
	
	bool init (cstr filepath) {
		fh = CreateFile(filepath, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (fh != INVALID_HANDLE_VALUE) {
			GetFileTime(fh, NULL, NULL, &last_change_t);
		}
		return fh != INVALID_HANDLE_VALUE;
	}
	
	bool poll_did_change (cstr filepath) {
		if (fh == INVALID_HANDLE_VALUE) return init(filepath); // contiously try to open the file so we can 'reload' it if it becomes available
		
		FILETIME cur_last_change_t;
		
		GetFileTime(fh, NULL, NULL, &cur_last_change_t);
		
		auto result = CompareFileTime(&last_change_t, &cur_last_change_t);
		dbg_assert(result == 0 || result == -1);
		
		last_change_t = cur_last_change_t;
		
		bool did_change = result != 0;
		if (did_change) {
			//Sleep(5); // files often are not completely written when the first change get's noticed, so we might want to wait for a bit
		}
		return did_change;
	}
};

static void log_warning_asset_load (cstr asset, cstr reason="") {
	log_warning("\"%s\" could not be loaded!%s", asset, reason);
}

#include "platform.hpp"
#include "gl.hpp"
#include "mesh_loader.hpp"
#include "shapes.hpp"

struct Base_Mesh {
	Mesh_Vbo			vbo; // one vbo per mesh for now
	Base_Shader*		shad;
	
	void init (Base_Shader* shad_) {
		vbo.init();
		shad = shad_;
	}
	
	virtual bool reload_if_needed () = 0;
	
};

struct File_Mesh : public Base_Mesh {
	cstr	filename;
	v3		pos_world;
	m3		ori;
	v3		scale;
	
	#if RZ_AUTO_FILE_RELOADING
	File_Change_Poller	fc;
	#endif
	
	void init_load (Base_Shader* shad_, cstr filename_, v3 pos_world_, m3 ori_=m3::ident(), v3 scale_=1) {
		Base_Mesh::init(shad_);
		
		filename =	filename_;
		pos_world = pos_world_;
		ori =		ori_;
		scale =		scale_;
		
		reload();
		
		#if RZ_AUTO_FILE_RELOADING
		std::string filepath = prints("assets_src/meshes/%s", filename);
		fc.init(filepath.c_str());
		#endif
	}
	
	void reload () {
		vbo.clear();
		
		auto filepath = prints("assets_src/meshes/%s", filename);
		
		load_mesh(&vbo, filepath.c_str(), transl_rot_scale(pos_world, ori, scale));
		vbo.upload();
	}
	virtual bool reload_if_needed () {
		#if RZ_AUTO_FILE_RELOADING
		auto filepath = prints("assets_src/meshes/%s", filename);
		
		bool reloaded = fc.poll_did_change(filepath.c_str());
		if (reloaded) {
			printf("mesh source file changed, reloading mesh \"%s\".\n", filepath.c_str());
			reload();
		}
		
		return reloaded;
		#else
		return false;
		#endif
	}
	
};

struct Generated_Tile_Floor : public Base_Mesh {
	void init_load (Base_Shader* shad_) {
		Base_Mesh::init(shad_);
		gen_tile_floor(&vbo.vertecies);
		vbo.upload();
	}
	virtual bool reload_if_needed () {	return false; }
};

struct Generated_Tetrahedron : public Base_Mesh {
	void init_load (Base_Shader* shad_, v3 pos_world, m3 ori, f32 r) {
		Base_Mesh::init(shad_);
		gen_tetrahedron(&vbo.vertecies, pos_world, ori, r);
		vbo.upload();
	}
	virtual bool reload_if_needed () {	return false; }
};
struct Generated_Cube : public Base_Mesh {
	void init_load (Base_Shader* shad_, v3 pos_world, m3 ori, f32 r) {
		Base_Mesh::init(shad_);
		gen_cube(&vbo.vertecies, pos_world, ori, r);
		vbo.upload();
	}
	virtual bool reload_if_needed () {	return false; }
};
struct Generated_Cylinder : public Base_Mesh {
	void init_load (Base_Shader* shad_, v3 pos_world, m3 ori, f32 r, f32 l, u32 faces) {
		Base_Mesh::init(shad_);
		gen_cylinder(&vbo.vertecies, pos_world, ori, r, l, faces);
		vbo.upload();
	}
	virtual bool reload_if_needed () {	return false; }
};
struct Generated_Iso_Sphere : public Base_Mesh {
	void init_load (Base_Shader* shad_, v3 pos_world, m3 ori, f32 r, u32 wfaces, u32 hfaces) {
		Base_Mesh::init(shad_);
		gen_iso_sphere(&vbo.vertecies, pos_world, ori, r, wfaces, hfaces);
		vbo.upload();
	}
	virtual bool reload_if_needed () {	return false; }
};

//
static f32			dt = 0;

struct Input {
	v2		mouse_look_diff =	0;
	
	iv3		move_dir =			0;
	bool	move_fast =			false;
};
static Input		inp;

struct Camera {
	v3	pos_world;
	v2	ori_ae; // azimuth elevation
	
	f32	fly_vel;
	f32	fly_vel_fast_mul;
	f32	vfov;
};

static Camera default_camera = {	v3(0, -5, 1),
									v2(deg(0), deg(+80)),
									4,
									4,
									deg(70) };

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
				case GLFW_KEY_A:			inp.move_dir.x -= went_down ? +1 : -1;		break;
				case GLFW_KEY_D:			inp.move_dir.x += went_down ? +1 : -1;		break;
				
				case GLFW_KEY_S:			inp.move_dir.z += went_down ? +1 : -1;		break;
				case GLFW_KEY_W:			inp.move_dir.z -= went_down ? +1 : -1;		break;
				
				case GLFW_KEY_LEFT_CONTROL:	inp.move_dir.y -= went_down ? +1 : -1;		break;
				case GLFW_KEY_SPACE:		inp.move_dir.y += went_down ? +1 : -1;		break;
				
				case GLFW_KEY_LEFT_SHIFT:	inp.move_fast = went_down;					break;
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
static void glfw_mouse_scroll (GLFWwindow* window, double xoffset, double yoffset) {
	if (inp.move_fast) {
		f32 delta_log = 0.1f * (f32)yoffset;
		cam.fly_vel = pow( 2, log2(cam.fly_vel) +delta_log );
		printf(">>> fly_vel: %f\n", cam.fly_vel);
	} else {
		f32 delta_log = -0.1f * (f32)yoffset;
		f32 vfov = pow( 2, log2(cam.vfov) +delta_log );
		if (vfov >= deg(1.0f/10) && vfov <= deg(170)) cam.vfov = vfov;
	}
}
static void glfw_cursor_move_relative (GLFWwindow* window, double dx, double dy) {
	v2 diff = v2((f32)dx,(f32)dy);
	inp.mouse_look_diff += diff;
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
	
	Shader		shad, shad2, shad_skybox;
	Shader_Tex	shad_diffuse, shad_overlay_tex;
	shad				.init_load("test.vert",			"test.frag");
	shad2				.init_load("normals.vert",		"normals.frag");
	shad_skybox			.init_load("skybox.vert",		"skybox.frag");
	shad_diffuse		.init_load("diffuse.vert",		"diffuse.frag");
	shad_overlay_tex	.init_load("overlay_tex.vert",	"overlay_tex.frag");
	
	std::vector<Base_Shader*> shaders = { &shad, &shad2, &shad_skybox, &shad_diffuse, &shad_overlay_tex };
	
	Texture2D	tex_test, tex_cerberus_diffuse;
	tex_test				.init_load("test.png");
	tex_cerberus_diffuse	.init_load("cerberus/Cerberus_A.tga");
	
	std::vector<Texture2D*>	textures = { &tex_test, &tex_cerberus_diffuse };
	
	Generated_Tile_Floor	mesh_tile_floor;
	Generated_Tetrahedron	mesh_tetrahedron;
	Generated_Cube			mesh_cube;
	Generated_Cylinder		mesh_cylinder;
	Generated_Iso_Sphere	mesh_iso_sphere;
	mesh_tile_floor		.init_load(&shad);
	mesh_tetrahedron	.init_load(&shad, v3(+2,+2,0), rotate3_Z(deg(13)), 1.0f / (1 +1.0f/3));
	mesh_cube			.init_load(&shad, v3( 0,+2,0), rotate3_Z(deg(37)), 0.5f);
	mesh_cylinder		.init_load(&shad, v3(-2,+2,0), m3::ident(), 0.5f, 2, 24);
	mesh_iso_sphere		.init_load(&shad, v3(-3, 0,0), m3::ident(), 0.5f, 64, 32);
	
	
	File_Mesh		mesh_pedestal, mesh_multi_obj_test, mesh_cerberus, mesh_nier;
	mesh_pedestal		.init_load(&shad2, "pedestal.obj",				v3(3,0,0),		rotate3_Z(deg(-70)));
	mesh_cerberus		.init_load(&shad_diffuse, "cerberus/cerberus.obj",		v3(0,0,1),		rotate3_Z(deg(45)));
	mesh_multi_obj_test	.init_load(&shad2, "multi_obj_test.obj",		v3(10,0,0),		rotate3_Z(deg(-78)));
	mesh_nier			.init_load(&shad2, "nier_models_test.obj",		v3(-4,-2,0),	rotate3_Z(deg(90)));
	
	
	std::vector<Base_Mesh*> meshes = {
		&mesh_tile_floor,
		&mesh_tetrahedron, &mesh_cube, &mesh_cylinder, &mesh_iso_sphere,
		&mesh_pedestal, &mesh_multi_obj_test, &mesh_cerberus, &mesh_nier
		//&mesh_pedestal, &mesh_multi_obj_test, &mesh_cerberus
	};
	
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
		
		for (auto* s : shaders)		s->reload_if_needed();
		for (auto* m : meshes)		m->reload_if_needed();
		for (auto* t : textures)	t->reload_if_needed();
		
		iv2 wnd_dim;
		v2	wnd_dim_aspect;
		{
			glfwGetFramebufferSize(wnd, &wnd_dim.x, &wnd_dim.y);
			
			v2 tmp = (v2)wnd_dim;
			wnd_dim_aspect = tmp / v2(tmp.y, tmp.x);
		}
		v2	mouse_look_diff;
		iv2	mcursor_pos_px;
		{
			mouse_look_diff = inp.mouse_look_diff;
			inp.mouse_look_diff = 0;
			
			{
				f64 x, y;
				glfwGetCursorPos(wnd, &x, &y);
				mcursor_pos_px = iv2((int)x, (int)y);
			}
		}
		auto bottom_up_mcursor_pos = [&] () -> v2 {
			return v2(mcursor_pos_px.x, wnd_dim.y -mcursor_pos_px.y);
		};
		
		m4 world_to_clip;
		m4 skybox_to_clip;
		{
			{
				v2 mouse_look_sens = v2(deg(1.0f / 8)) * (cam.vfov / deg(70));
				cam.ori_ae -= mouse_look_diff * mouse_look_sens;
				cam.ori_ae.x = mymod(cam.ori_ae.x, deg(360));
				cam.ori_ae.y = clamp(cam.ori_ae.y, deg(2), deg(180.0f -2));
				
				//printf(">>> %f %f\n", to_deg(camera_ae.x), to_deg(camera_ae.y));
			}
			m3 world_to_cam_rot = rotate3_X(-cam.ori_ae.y) * rotate3_Z(-cam.ori_ae.x);
			m3 cam_to_world_rot = rotate3_Z(cam.ori_ae.x) * rotate3_X(cam.ori_ae.y);
			
			{
				f32 cam_vel_forw = cam.fly_vel;
				if (inp.move_fast) cam_vel_forw *= cam.fly_vel_fast_mul;
				
				v3 cam_vel = cam_vel_forw * v3(1,2.0f/3,1);
				
				v3 cam_vel_cam = normalize_or_zero( (v3)inp.move_dir ) * cam_vel;
				cam.pos_world += (cam_to_world_rot * cam_vel_cam) * dt;
				
				//printf(">>> %f %f %f\n", cam_vel_cam.x, cam_vel_cam.y, cam_vel_cam.z);
			}
			m4 world_to_cam = world_to_cam_rot * translate4(-cam.pos_world);
			
			m4 cam_to_clip;
			{
				f32 vfov =			cam.vfov;
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
			skybox_to_clip = cam_to_clip * world_to_cam_rot;
		}
		
		glViewport(0, 0, wnd_dim.x, wnd_dim.y);
		
		shad.bind();
		shad				.common.set(world_to_clip, bottom_up_mcursor_pos(), (v2)wnd_dim);
		shad2.bind();
		shad2				.common.set(world_to_clip, bottom_up_mcursor_pos(), (v2)wnd_dim);
		shad_skybox.bind();
		shad_skybox			.common.set(world_to_clip, bottom_up_mcursor_pos(), (v2)wnd_dim);
		shad_diffuse.bind();
		shad_diffuse		.common.set(world_to_clip, bottom_up_mcursor_pos(), (v2)wnd_dim);
		shad_overlay_tex.bind();
		shad_overlay_tex	.common.set(world_to_clip, bottom_up_mcursor_pos(), (v2)wnd_dim);
		
		shad_diffuse.tex0.bind(tex_cerberus_diffuse);
		
		if (1) { // draw skybox
			glDisable(GL_DEPTH_TEST);
			
			shad_skybox.bind();
			shad_skybox.skybox_to_clip.set(skybox_to_clip);
			//shad_skybox.tex_skybox.bind();
			
			// Coordinates generated in vertex shader
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			glDrawArrays(GL_TRIANGLES, 0, 6*6);
			
			glEnable(GL_DEPTH_TEST);
		} else { // draw clear color
			v4 clear_color = v4(srgb(41,49,52)*3, 1);
			glClearColor(clear_color.x,clear_color.y,clear_color.z,clear_color.w);
			glClear(GL_COLOR_BUFFER_BIT);
		}
		glClear(GL_DEPTH_BUFFER_BIT);
		
		for (auto* m : meshes) {
			m->shad->bind();
			m->vbo.draw_entire(m->shad);
		}
		
		shad_overlay_tex.bind();
		shad_overlay_tex.tex_dim.set( (v2)tex_test.dim );
		shad_overlay_tex.tex0.bind( tex_test );
		glDrawArrays(GL_TRIANGLES, 0, 6);
		
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
