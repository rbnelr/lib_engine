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

#include "platform.hpp"


#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STBI_ONLY_BMP	1
#define STBI_ONLY_PNG	1
#define STBI_ONLY_TGA	1
//#define STBI_ONLY_JPEG	1

#include "stb_image.h"

#define STB_RECT_PACK_IMPLEMENTATION
#define STBRP_STATIC
#include "stb_rect_pack.h"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb_truetype.h"


#if RZ_DEV
	#define RZ_AUTO_FILE_RELOAD 1
#else
	#define RZ_AUTO_FILE_RELOAD 0
#endif

#if RZ_AUTO_FILE_RELOAD
	#define IF_RZ_AUTO_FILE_RELOAD(...) __VA_ARGS__
#else
	#define IF_RZ_AUTO_FILE_RELOAD(...)
#endif

static std::vector< std::basic_string<utf32> >		g_console_log_lines;

static void logf (cstr format, ...) {
	std::string str;
	
	va_list vl;
	va_start(vl, format);
	
	_prints(&str, format, vl);
	
	va_end(vl);
	
	g_console_log_lines.push_back( utf8_to_utf32(str) );
	
	str.push_back('\n');
	printf(str.c_str());
}

struct File_Change_Poller {
	std::string	filepath;
	
	HANDLE		fh;
	FILETIME	last_change_t;
	
	bool init (strcr f) {
		filepath = f;
		last_change_t = {}; // zero for debuggability
		return open();
	}
	bool open () {
		fh = CreateFile(filepath.c_str(), GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (fh != INVALID_HANDLE_VALUE) {
			GetFileTime(fh, NULL, NULL, &last_change_t);
		}
		return fh != INVALID_HANDLE_VALUE;
	}
	
	void close () {
		if (fh != INVALID_HANDLE_VALUE) {
			auto ret = CloseHandle(fh);
			dbg_assert(ret != 0);
		}
	}
	
	bool poll_did_change () {
		if (fh == INVALID_HANDLE_VALUE) return open();
		
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

typedef u32 vert_indx_t;

static cstr shaders_base_path =		"shaders";
static cstr meshes_base_path =		"assets_src";
static cstr textures_base_path =	"assets_src";

#include "gl.hpp"
#include "font.hpp"

static font::Font g_console_font;

static void draw_loading_screen_frame ();

#define UV2(name)	Uniform(T_V2, name)
#define UV3(name)	Uniform(T_V3, name)
#define UM4(name)	Uniform(T_M4, name)

static std::vector<Shader*>		g_shaders;
static std::vector<Texture2D*>	g_textures;

static Shader* new_shader (cstr v, cstr f, std::initializer_list<Uniform> u, std::initializer_list<Shader::Uniform_Texture> t={}) {
	Shader* s = new Shader;
	g_shaders.push_back(s);
	s->init_load(v, f, u, t);
	return s;
}

static Texture2D* new_texture2d (cstr filename, bool linear=false) {
	File_Texture2D* t = new File_Texture2D;
	g_textures.push_back(t);
	
	logf("Loading texture '%s'...", filename);
	draw_loading_screen_frame();
	
	t->init_load(filename, linear);
	
	return t;
}

//
struct Mesh_Vertex {
	v3	pos_model;
	v3	norm_model;
	v4	tang_model;
	v2	uv;
	v4	col;
	
	bool operator== (Mesh_Vertex const& r) const; // for hash map
};
static constexpr v3 DEFAULT_POS =	0;
static constexpr v3 DEFAULT_NORM =	0;
static constexpr v4 DEFAULT_TANG =	0;
static constexpr v2 DEFAULT_UV =	0.5f;
static constexpr v4 DEFAULT_COL =	1;

static Vertex_Layout g_mesh_vert_layout = {
	{ "pos_model",	T_V3, sizeof(Mesh_Vertex), offsetof(Mesh_Vertex, pos_model) },
	{ "norm_model",	T_V3, sizeof(Mesh_Vertex), offsetof(Mesh_Vertex, norm_model) },
	{ "tang_model",	T_V4, sizeof(Mesh_Vertex), offsetof(Mesh_Vertex, tang_model) },
	{ "uv",			T_V2, sizeof(Mesh_Vertex), offsetof(Mesh_Vertex, uv) },
	{ "col",		T_V4, sizeof(Mesh_Vertex), offsetof(Mesh_Vertex, col) }
};

#include "mesh_loader.hpp"
#include "shapes.hpp"

static constexpr GLint MAX_TEXTURE_UNIT = 8; // for debugging only, to unbind textures from unused texture units

static void bind_texture_unit (GLint tex_unit, Texture2D* tex) {
	dbg_assert(tex_unit >= 0 && tex_unit < MAX_TEXTURE_UNIT, "increase MAX_TEXTURE_UNIT (%d, tex_unit: %d)", MAX_TEXTURE_UNIT, tex_unit);
	
	glActiveTexture(GL_TEXTURE0 +tex_unit);
	glBindTexture(GL_TEXTURE_2D, tex->tex);
}
static void unbind_texture_unit (GLint tex_unit) {
	dbg_assert(tex_unit >= 0 && tex_unit < MAX_TEXTURE_UNIT);
	
	glActiveTexture(GL_TEXTURE0 +tex_unit);
	glBindTexture(GL_TEXTURE_2D, 0);
}

struct Allotted_Texture {
	GLint		tex_unit;
	Texture2D*	tex;
	
	void bind () {
		bind_texture_unit(tex_unit, tex);
	}
};

//
struct Base_Mesh;

static std::vector<Base_Mesh*>		g_meshes;
static std::vector<Base_Mesh*>		g_meshes_opaque;
static std::vector<Base_Mesh*>		g_meshes_translucent;

struct Base_Mesh {
	cstr		name;
	
	v3			pos_world;
	m3			ori;
	v3			scale;
	
	Shader*		shad;
	Shader*		shad_transp_pass2; // only for meshes that use transparency
	
	Vbo			vbo; // one vbo per mesh for now
	
	std::vector<Allotted_Texture>	textures;
	
	Base_Mesh (cstr n, Shader* s, Shader* s2, v3 p, m3 o, std::initializer_list<Allotted_Texture> t={}) {
		name = n;
		
		pos_world = p;
		ori = o;
		scale = 1;
		
		shad =				s;
		shad_transp_pass2 =	s2;
		
		vbo.init(&g_mesh_vert_layout);
		
		textures = t;
		
		g_meshes.push_back(this);
	}
	
	hm get_transform () {
		return transl_rot_scale(pos_world, ori, scale);
	}
	
	void bind_textures () {
		for (GLint tex_unit=0; tex_unit<MAX_TEXTURE_UNIT; ++tex_unit) {
			bool tex_unit_used = false;
			for (auto& t : textures) {
				if (t.tex_unit == tex_unit) {
					tex_unit_used = true;
					break;
				}
			}
			if (!tex_unit_used) unbind_texture_unit(tex_unit);
		}
		for (auto& t : textures) t.bind();
	}
	
	virtual void reload () = 0;
	virtual bool reload_if_needed () = 0;
};

struct File_Mesh : public Base_Mesh {
	
	cstr					filename;
	
	#if RZ_AUTO_FILE_RELOAD
	File_Change_Poller	fc;
	#endif
	
	File_Mesh (cstr n, cstr f, Shader* s, Shader* s2, v3 p, m3 o, std::initializer_list<Allotted_Texture> t={}):
			Base_Mesh{n, s, s2, p, o, t} {
		
		filename = f;
		reload();
		
		#if RZ_AUTO_FILE_RELOAD
		auto filepath = prints("%s/%s", meshes_base_path, filename);
		fc.init(filepath.c_str());
		#endif
	}
	
	virtual void reload () {
		vbo.clear();
		
		auto filepath = prints("%s/%s", meshes_base_path, filename);
		
		logf("Loading file mesh '%s'...", filename);
		draw_loading_screen_frame();
		
		load_mesh(&vbo, filepath.c_str(), hm::ident());
		
		vbo.upload();
	}
	virtual bool reload_if_needed () {
		#if RZ_AUTO_FILE_RELOAD
		bool reloaded = fc.poll_did_change();
		if (reloaded) {
			printf("mesh source file changed, reloading mesh \"%s\".\n", name);
			reload();
		}
		
		return reloaded;
		#else
		return false;
		#endif
	}
	
};
File_Mesh* new_mesh (cstr n, cstr f, Shader* s, v3 p, m3 o, std::initializer_list<Allotted_Texture> t={}) {
	auto* m = new File_Mesh(n,f,s,nullptr,p,o,t);
	g_meshes_opaque.push_back(m);
	return m;
}
File_Mesh* new_transp_mesh (cstr n, cstr f, Shader* s, Shader* s2, v3 p, m3 o, std::initializer_list<Allotted_Texture> t={}) {
	auto* m = new File_Mesh(n,f,s,s2,p,o,t);
	g_meshes_translucent.push_back(m);
	return m;
}

struct Gen_Mesh_Floor : public Base_Mesh {
	
	Gen_Mesh_Floor (cstr n, Shader* s, std::initializer_list<Allotted_Texture> t={}):
			Base_Mesh{n, s, nullptr, 0, m3::ident(), t} {
		reload();
	}
	void gen () { gen_tile_floor(&vbo.vertecies); };
	
	virtual void reload () {
		vbo.clear();
		printf("Generating mesh for %s...\n", name);
		gen();
		vbo.upload();
	}
	virtual bool reload_if_needed () { return false; }
};
Gen_Mesh_Floor* new_gen_tile_floor (cstr n, Shader* s, std::initializer_list<Allotted_Texture> t={}) {
	auto* m = new Gen_Mesh_Floor(n,s,t);
	g_meshes_opaque.push_back(m);
	return m;
}

struct Gen_Mesh_Tetrahedron : public Base_Mesh {
	f32		radius;
	
	Gen_Mesh_Tetrahedron (cstr n, Shader* s, v3 p, m3 o, f32 r, std::initializer_list<Allotted_Texture> t={}):
			Base_Mesh{n, s, nullptr, p,o, t}, radius{r} {
		reload();
	}
	void gen () { gen_tetrahedron(&vbo.vertecies, radius); };
	
	virtual void reload () {
		vbo.clear();
		gen();
		vbo.upload();
	}
	virtual bool reload_if_needed () { return false; }
};
Gen_Mesh_Tetrahedron* new_gen_tetrahedron (cstr n, Shader* s, v3 p, m3 o, f32 r, std::initializer_list<Allotted_Texture> t={}) {
	auto* m = new Gen_Mesh_Tetrahedron(n,s,p,o,r,t);
	g_meshes_opaque.push_back(m);
	return m;
}

struct Gen_Mesh_Cube : public Base_Mesh {
	f32		radius;
	
	Gen_Mesh_Cube (cstr n, Shader* s, v3 p, m3 o, f32 r, std::initializer_list<Allotted_Texture> t={}):
			Base_Mesh{n, s, nullptr, p,o, t}, radius{r} {
		reload();
	}
	void gen () { gen_cube(&vbo.vertecies, radius); };
	
	virtual void reload () {
		vbo.clear();
		gen();
		vbo.upload();
	}
	virtual bool reload_if_needed () { return false; }
};
Gen_Mesh_Cube* new_gen_cube (cstr n, Shader* s, v3 p, m3 o, f32 r, std::initializer_list<Allotted_Texture> t={}) {
	auto* m = new Gen_Mesh_Cube(n,s,p,o,r,t);
	g_meshes_opaque.push_back(m);
	return m;
}

struct Gen_Mesh_Cylinder : public Base_Mesh {
	f32		radius;
	f32		length;
	u32		faces;
	
	Gen_Mesh_Cylinder (cstr n, Shader* s, v3 p, m3 o, f32 r, f32 l, u32 faces, std::initializer_list<Allotted_Texture> t={}):
			Base_Mesh{n, s, nullptr, p,o, t}, radius{r}, length{l}, faces{faces} {
		reload();
	}
	void gen () { gen_cylinder(&vbo.vertecies, radius, length, faces); };
	
	virtual void reload () {
		vbo.clear();
		gen();
		vbo.upload();
	}
	virtual bool reload_if_needed () { return false; }
};
Gen_Mesh_Cylinder* new_gen_cylinder (cstr n, Shader* s, v3 p, m3 o, f32 r, f32 l, u32 f, std::initializer_list<Allotted_Texture> t={}) {
	auto* m = new Gen_Mesh_Cylinder(n,s,p,o,r,l,f,t);
	g_meshes_opaque.push_back(m);
	return m;
}

struct Gen_Mesh_Iso_Sphere : public Base_Mesh {
	f32		radius;
	u32		wfaces;
	u32		hfaces;
	
	Gen_Mesh_Iso_Sphere (cstr n, Shader* s, v3 p, m3 o, f32 r, u32 fw, u32 fh, std::initializer_list<Allotted_Texture> t={}):
			Base_Mesh{n, s, nullptr, p,o, t}, radius{r}, wfaces{fw}, hfaces{fh} {
		reload();
	}
	void gen () { gen_iso_sphere(&vbo.vertecies, radius, wfaces, hfaces); };
	
	virtual void reload () {
		vbo.clear();
		gen();
		vbo.upload();
	}
	virtual bool reload_if_needed () { return false; }
};
Gen_Mesh_Iso_Sphere* new_gen_iso_sphere (cstr n, Shader* s, v3 p, m3 o, f32 r, u32 fw, u32 fh, std::initializer_list<Allotted_Texture> t={}) {
	auto* m = new Gen_Mesh_Iso_Sphere(n,s,p,o,r,fw,fh,t);
	g_meshes_opaque.push_back(m);
	return m;
}

//
static f32			dt = 0;

struct Input {
	iv2		wnd_dim;
	v2		wnd_dim_aspect;
	
	iv2		mcursor_pos_px;
	
	//
	v2		mouse_look_diff;
	
	iv3		move_dir =			0;
	bool	move_fast =			false;
	
	void get_non_callback_input () {
		{
			glfwGetFramebufferSize(wnd, &wnd_dim.x, &wnd_dim.y);
			
			v2 tmp = (v2)wnd_dim;
			wnd_dim_aspect = tmp / v2(tmp.y, tmp.x);
		}
		{
			f64 x, y;
			glfwGetCursorPos(wnd, &x, &y);
			mcursor_pos_px = iv2((int)x, (int)y);
		}
	}
	
	v2 bottom_up_mcursor_pos () {
		return v2(mcursor_pos_px.x, wnd_dim.y -mcursor_pos_px.y);
	}
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
	if (!inp.move_fast) {
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

static Vbo		vbo_console_font;
static Shader*	shad_font;

static void draw_console_log_text () {
	vbo_console_font.clear();
	
	u32 max_fully_visible_lines = max( (u32)1, (u32)floor((f32)inp.wnd_dim.y / g_console_font.line_height) );
	
	u32 max_buffered_lines = 1000;
	if (g_console_log_lines.size() > max_buffered_lines) { // only keep at most max_buffered_lines lines
		g_console_log_lines.erase( g_console_log_lines.begin(), g_console_log_lines.begin() +(g_console_log_lines.size() -max_buffered_lines));
	}
	
	f32 pos_y_px = g_console_font.ascent_plus_gap;
	
	for (auto l = g_console_log_lines.begin() +max((s32)0, (s32)g_console_log_lines.size() -(s32)max_fully_visible_lines);
			l!=g_console_log_lines.end(); ++l) {
		g_console_font.draw_line(&vbo_console_font.vertecies, pos_y_px, shad_font, *l, v4(1,0,0,1));
		pos_y_px += g_console_font.line_height;
	}
	
	if (shad_font->valid()) {
		glEnable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		
		shad_font->bind();
		shad_font->set_unif("screen_dim", (v2)inp.wnd_dim);
		bind_texture_unit(0, &g_console_font.tex);
		
		vbo_console_font.upload();
		vbo_console_font.draw_entire(shad_font);
		
		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
	}
}

static void draw_loading_screen_frame () {
	
	glfwSetWindowTitle(wnd, prints("loading...").c_str());
	
	inp.mouse_look_diff = 0;
	
	glfwPollEvents();
	
	inp.get_non_callback_input();
	
	v4 clear_color = v4(srgb(41,49,52)*3, 1);
	glClearColor(clear_color.x,clear_color.y,clear_color.z,clear_color.w);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	
	draw_console_log_text();
	
	glfwSwapBuffers(wnd);
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
		
		glDisable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max_aniso);
	}
	
	#define UCOM UV2("screen_dim"), UV2("mcursor_pos") // common uniforms
	#define UMAT UM4("model_to_world"), UM4("world_to_cam"), UM4("cam_to_clip"), UM4("cam_to_world") // transformation uniforms
	
	{ // init game console overlay
		f32 sz =	16; // 14 16 24
		f32 jpsz =	floor(sz * 1.75f);
		
		std::initializer_list<font::Glyph_Range> ranges = {
			{ "consola.ttf",	sz,		  U'\xfffd' }, // missing glyph placeholder, must be the zeroeth glyph
			{ "consola.ttf",	sz,		  U' ', U'~' },
			{ "consola.ttf",	sz,		{ U'ß',U'Ä',U'Ö',U'Ü',U'ä',U'ö',U'ü' } }, // german umlaute
			{ "meiryo.ttc",		jpsz,	  U'\x3040', U'\x30ff' }, // hiragana +katakana
			{ "meiryo.ttc",		jpsz,	{ U'　',U'、',U'。',U'”',U'「',U'」' } }, // some jp puncuation
		};
		
		g_console_font.init(sz, ranges);
		
		vbo_console_font.init(&font::g_mesh_vert_layout);
		shad_font = new_shader("font.vert", "font.frag", {UCOM}, {{0,"glyphs"}});
	}
	
	//
	
	auto* shad_skybox =				new_shader("skybox.vert",		"skybox.frag",			{UCOM, UM4("skybox_to_clip")});
	auto* shad_overlay_tex =		new_shader("overlay_tex.vert",	"overlay_tex.frag",		{UCOM, UV2("tex_dim")}, {{0,"tex0"}});
	
	auto* tex_test =				new_texture2d("test/fast.png");
	//auto* tex_test =				new_texture2d("test/haha.dds");
	//auto* tex_test =				new_texture2d("test/test.dds");
	
	{ // Generated meshes
		auto* shad_vc =			new_shader("mesh_vertex.vert",	"vertex_color.frag",	{UCOM, UMAT});
		auto* shad_diff =		new_shader("mesh_vertex.vert",	"diffuse.frag",			{UCOM, UMAT}, {{0,"tex0"}});
		
		new_gen_tile_floor("tile_floor",	shad_diff, {{0, new_texture2d("rz/tile2.png")}});
		new_gen_tetrahedron("tetrahedron",	shad_vc,	v3(+2,+2,0),	rotate3_Z(deg(13)),		1.0f / (1 +1.0f/3));
		new_gen_cube("cube",				shad_vc,	v3( 0,+2,0),	rotate3_Z(deg(37)),		0.5f);
		new_gen_cylinder("cylinder",		shad_vc,	v3(-2,+2,0),	m3::ident(),			0.5f, 2, 24);
		new_gen_iso_sphere("iso_sphere",	shad_vc,	v3(-3, 0,0),	m3::ident(),			0.5f, 64, 32);
	}
	{ // Meshes modeled by me
		auto* shad2 =				new_shader("mesh_vertex.vert",	"normals.frag",			{UCOM, UMAT});
		
		new_mesh("pedestal",		"rz/pedestal.obj",			shad2,		v3(3,0,0),		rotate3_Z(deg(-70)));
		new_mesh("multi_obj_test",	"rz/multi_obj_test.obj",	shad2,		v3(10,0,0),		rotate3_Z(deg(-78)));
		
	}
	{ // Cerberus PBR gun
		auto* shad_cerb =			new_shader("mesh_vertex.vert",	"cerberus.frag",		{UCOM, UMAT}, {{0,"albedo"}, {1,"normal"}, {2,"metallic"}, {3,"roughness"}});
		
		auto* tex_cerb_albedo =		new_texture2d("cerberus/Cerberus_A.tga");
		auto* tex_cerb_normal =		new_texture2d("cerberus/Cerberus_N.tga", TEX_LINEAR);
		auto* tex_cerb_metallic =	new_texture2d("cerberus/Cerberus_M.tga");
		auto* tex_cerb_roughness =	new_texture2d("cerberus/Cerberus_R.tga");
		
		new_mesh("cerberus",		"cerberus/cerberus.obj",	shad_cerb,	v3(0,0,1),		rotate3_Z(deg(45)), {{0,tex_cerb_albedo}, {1,tex_cerb_normal}, {2,tex_cerb_metallic}, {3,tex_cerb_roughness}});
	}
	{ // Nier models
		auto* shad =			new_shader("mesh_vertex.vert",	"nier_alpha_test.frag",		{UCOM, UMAT}, {{0,"albedo"}, {1,"normal"}, {2,"a"}, {3,"b"}});
		auto* shad2 =			new_shader("mesh_vertex.vert",	"nier_transp_pass2.frag",	{UCOM, UMAT}, {{0,"albedo"}, {1,"normal"}, {2,"a"}, {3,"b"}});
		
		typedef std::initializer_list<Allotted_Texture>	TL;
		{
			auto* tex_albedo =		new_texture2d("nier/2b_skin_0.dds");
			auto* tex_a =			new_texture2d("nier/2b_skin_1.dds");
			auto* tex_normal =		new_texture2d("nier/2b_skin_2.dds", TEX_LINEAR);
			auto* tex_b =			new_texture2d("nier/2b_skin_4.dds");
			TL texs =				{{0,tex_albedo}, {1,tex_normal}, {2,tex_a}, {3,tex_b}};
			
			new_mesh("2b skin",		"nier/2b_skin.obj",			shad,	v3(-4,-2,0),	rotate3_Z(deg(90)),	texs);
			new_mesh("2b eyes",		"nier/2b_eyes.obj",			shad,	v3(-4,-2,0),	rotate3_Z(deg(90)),	texs);
		}
		{
			auto* tex_albedo =		new_texture2d("nier/2b_clothing_0.dds");
			auto* tex_normal =		new_texture2d("nier/2b_clothing_3.dds", TEX_LINEAR);
			TL texs =				{{0,tex_albedo}, {1,tex_normal}};
			
			new_mesh("2b",			"nier/2b_clothes.obj",		shad,	v3(-4,-2,0),	rotate3_Z(deg(90)),	texs);
			new_mesh("2b skirt",	"nier/2b_dress_skirt.obj",	shad,	v3(-4,-2,0),	rotate3_Z(deg(90)),	texs);
			new_mesh("2b sliceb",	"nier/2b_alice_band.obj",	shad,	v3(-4,-2,0),	rotate3_Z(deg(90)),	texs);
			new_mesh("2b blindf",	"nier/2b_blindfold.obj",	shad,	v3(-4,-2,0),	rotate3_Z(deg(90)),	texs);
		}
		{
			auto* tex_albedo =		new_texture2d("nier/2b_frills_hair_0.dds");
			auto* tex_normal =		new_texture2d("nier/2b_frills_hair_3.dds", TEX_LINEAR);
			TL texs =				{{0,tex_albedo}, {1,tex_normal}};
			
			new_transp_mesh("2b frills","nier/2b_dress_frills.obj",	shad, shad2,	v3(-4,-2,0),	rotate3_Z(deg(90)),	texs);
			
			new_transp_mesh("2b hair",	"nier/2b_hair.obj",			shad, shad2,	v3(-4,-2,0),	rotate3_Z(deg(90)),	texs);
		}
		{
			auto* tex_albedo =		new_texture2d("nier/2b_hair_thin_0.dds");
			auto* tex_normal =		new_texture2d("nier/2b_hair_thin_3.dds", TEX_LINEAR);
			TL texs =				{{0,tex_albedo}, {1,tex_normal}};
			
			new_transp_mesh("2b hair thin","nier/2b_hair_thin.obj",	shad, shad2,	v3(-4,-2,0),	rotate3_Z(deg(90)),	texs);
		}
		
		{
			auto* tex_albedo =		new_texture2d("nier/9s_skin_0.dds");
			auto* tex_a =			new_texture2d("nier/9s_skin_1.dds");
			auto* tex_normal =		new_texture2d("nier/9s_skin_2.dds", TEX_LINEAR);
			auto* tex_b =			new_texture2d("nier/9s_skin_4.dds");
			TL texs =				{{0,tex_albedo}, {1,tex_normal}, {2,tex_a}, {3,tex_b}};
			
			new_mesh("9s skin",		"nier/9s_skin.obj",			shad,	v3(-4,-2,0),	rotate3_Z(deg(90)),	texs);
			new_mesh("9s eyes",		"nier/9s_eyes.obj",			shad,	v3(-4,-2,0),	rotate3_Z(deg(90)),	texs);
		}
		{
			auto* tex_albedo =		new_texture2d("nier/9s_clothing_0.dds");
			auto* tex_a =			new_texture2d("nier/9s_clothing_1.dds");
			auto* tex_normal =		new_texture2d("nier/9s_clothing_3.dds", TEX_LINEAR);
			TL texs =				{{0,tex_albedo}, {1,tex_normal}, {2,tex_a}};
			
			new_mesh("9s clothes",	"nier/9s_clothes.obj",		shad,	v3(-4,-2,0),	rotate3_Z(deg(90)),	texs);
			new_mesh("9s shorts",	"nier/9s_shorts.obj",		shad,	v3(-4,-2,0),	rotate3_Z(deg(90)),	texs);
			new_mesh("9s blindf",	"nier/9s_blindfold.obj",	shad,	v3(-4,-2,0),	rotate3_Z(deg(90)),	texs);
		}
		{
			auto* tex_albedo =		new_texture2d("nier/9s_hair_0.dds");
			auto* tex_normal =		new_texture2d("nier/9s_hair_3.dds", TEX_LINEAR);
			TL texs =				{{0,tex_albedo}, {1,tex_normal}};
			
			new_transp_mesh("9s hair","nier/9s_hair.obj",		shad, shad2,	v3(-4,-2,0),	rotate3_Z(deg(90)),	texs);
		}
		
		{
			auto* tex_albedo =		new_texture2d("nier/sword_small_2b_0.dds");
			auto* tex_a =			new_texture2d("nier/sword_small_2b_2.dds");
			auto* tex_normal =		new_texture2d("nier/sword_small_2b_3.dds", TEX_LINEAR);
			TL texs =				{{0,tex_albedo}, {1,tex_normal}, {2,tex_a}};
			
			new_mesh("sword S 2b",	"nier/sword_small_2b.obj",	shad,	v3(-4,-2,0),	rotate3_Z(deg(90)),	texs);
		}
		{
			auto* tex_albedo =		new_texture2d("nier/sword_small_9s_0.dds");
			auto* tex_a =			new_texture2d("nier/sword_small_9s_2.dds");
			auto* tex_normal =		new_texture2d("nier/sword_small_9s_3.dds", TEX_LINEAR);
			TL texs =				{{0,tex_albedo}, {1,tex_normal}, {2,tex_a}};
			
			new_mesh("sword S 9s",	"nier/sword_small_9s.obj",	shad,	v3(-4,-2,0),	rotate3_Z(deg(90)),	texs);
		}
		{
			auto* tex_albedo =		new_texture2d("nier/sword_large_0.dds");
			auto* tex_a =			new_texture2d("nier/sword_large_2.dds");
			auto* tex_normal =		new_texture2d("nier/sword_large_3.dds", TEX_LINEAR);
			TL texs =				{{0,tex_albedo}, {1,tex_normal}, {2,tex_a}};
			
			new_mesh("sword L",		"nier/sword_large.obj",		shad,	v3(-4,-2,0),	rotate3_Z(deg(90)),	texs);
		}
	}
	
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
			
			//logf("%s %6d  %6.1f fps avg %6.2f ms avg", app_name, frame_i, avg_fps, avg_dt_ms);
		}
		
		inp.mouse_look_diff = 0;
		
		glfwPollEvents();
		
		inp.get_non_callback_input();
		
		if (glfwWindowShouldClose(wnd)) break;
		
		for (auto* s : g_shaders)		s->reload_if_needed();
		for (auto* t : g_textures)		t->reload_if_needed();
		for (auto* m : g_meshes)		m->reload_if_needed();
		
		hm world_to_cam;
		hm cam_to_world;
		m4 cam_to_clip;
		m4 skybox_to_clip;
		{
			{
				v2 mouse_look_sens = v2(deg(1.0f / 8)) * (cam.vfov / deg(70));
				cam.ori_ae -= inp.mouse_look_diff * mouse_look_sens;
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
			world_to_cam = world_to_cam_rot * translateH(-cam.pos_world);
			cam_to_world =translateH(cam.pos_world) * cam_to_world_rot;
			
			{
				f32 vfov =			cam.vfov;
				f32 clip_near =		1.0f/256;
				f32 clip_far =		512;
				
				v2 frust_scale;
				frust_scale.y = tan(vfov / 2);
				frust_scale.x = frust_scale.y * inp.wnd_dim_aspect.x;
				
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
			
			skybox_to_clip = cam_to_clip * world_to_cam_rot;
		}
		
		for (auto* s : g_shaders) {
			if (s->valid()) {
				s->bind();
				s->set_unif("screen_dim", (v2)inp.wnd_dim);
				s->set_unif("mcursor_pos", inp.bottom_up_mcursor_pos());
			}
		}
		
		glViewport(0,0, inp.wnd_dim.x,inp.wnd_dim.y);
		
		if (1) { // draw skybox
			
			if (shad_skybox->valid()) {
				glDisable(GL_DEPTH_TEST);
				
				shad_skybox->bind();
				shad_skybox->set_unif("skybox_to_clip", skybox_to_clip);
				
				// Coordinates generated in vertex shader
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
				glDrawArrays(GL_TRIANGLES, 0, 6*6);
				
				glEnable(GL_DEPTH_TEST);
			}
		} else { // draw clear color
			v4 clear_color = v4(srgb(41,49,52)*3, 1);
			glClearColor(clear_color.x,clear_color.y,clear_color.z,clear_color.w);
			glClear(GL_COLOR_BUFFER_BIT);
		}
		glClear(GL_DEPTH_BUFFER_BIT);
		
		for (auto* m : g_meshes_opaque) {
			if (m->shad->valid()) {
				m->bind_textures();
				
				hm model_to_world = m->get_transform();
				
				m->shad->bind();
				m->shad->set_unif("model_to_world",	model_to_world.m4());
				m->shad->set_unif("world_to_cam",	world_to_cam.m4());
				m->shad->set_unif("cam_to_clip",	cam_to_clip);
				m->shad->set_unif("cam_to_world",	cam_to_world.m4());
				
				m->vbo.draw_entire(m->shad);
			}
		}
		
		for (auto* m : g_meshes_translucent) {
			
			m->bind_textures();
			
			hm model_to_world = m->get_transform();
			
			if (m->shad->valid()) {
				m->shad->bind();
				m->shad->set_unif("model_to_world",	model_to_world.m4());
				m->shad->set_unif("world_to_cam",	world_to_cam.m4());
				m->shad->set_unif("cam_to_clip",	cam_to_clip);
				m->shad->set_unif("cam_to_world",	cam_to_world.m4());
				
				m->vbo.draw_entire(m->shad);
			}
			if (m->shad_transp_pass2->valid()) { 
				glDepthMask(GL_FALSE);
				glEnable(GL_BLEND);
				glDepthFunc(GL_LESS);
			
				m->shad_transp_pass2->bind();
				m->shad_transp_pass2->set_unif("model_to_world",	model_to_world.m4());
				m->shad_transp_pass2->set_unif("world_to_cam",	world_to_cam.m4());
				m->shad_transp_pass2->set_unif("cam_to_clip",	cam_to_clip);
				m->shad_transp_pass2->set_unif("cam_to_world",	cam_to_world.m4());
				
				m->vbo.draw_entire(m->shad_transp_pass2);
				
				glDepthFunc(GL_LEQUAL);
				glDisable(GL_BLEND);
				glDepthMask(GL_TRUE);
			}
		}
		
		if (shad_overlay_tex->valid()) {
			glEnable(GL_BLEND);
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_CULL_FACE);
			
			shad_overlay_tex->bind();
			shad_overlay_tex->set_unif("tex_dim", (v2)tex_test->dim);
			bind_texture_unit(0, tex_test);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			
			glEnable(GL_CULL_FACE);
			glEnable(GL_DEPTH_TEST);
			glDisable(GL_BLEND);
		}
		
		//draw_console_log_text();
		
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
