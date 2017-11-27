
static bool get_shader_compile_log (GLuint shad, std::string* log) {
	GLsizei log_len;
	{
		GLint temp = 0;
		glGetShaderiv(shad, GL_INFO_LOG_LENGTH, &temp);
		log_len = (GLsizei)temp;
	}
	
	if (log_len <= 1) {
		return false; // no log available
	} else {
		// GL_INFO_LOG_LENGTH includes the null terminator, but it is not allowed to write the null terminator in std::string, so we have to allocate one additional char and then resize it away at the end
		
		log->resize(log_len);
		
		GLsizei written_len = 0;
		glGetShaderInfoLog(shad, log_len, &written_len, &(*log)[0]);
		dbg_assert(written_len == (log_len -1));
		
		log->resize(written_len);
		
		return true;
	}
}
static bool get_program_link_log (GLuint prog, std::string* log) {
	GLsizei log_len;
	{
		GLint temp = 0;
		glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &temp);
		log_len = (GLsizei)temp;
	}
	
	if (log_len <= 1) {
		return false; // no log available
	} else {
		// GL_INFO_LOG_LENGTH includes the null terminator, but it is not allowed to write the null terminator in std::string, so we have to allocate one additional char and then resize it away at the end
		
		log->resize(log_len);
		
		GLsizei written_len = 0;
		glGetProgramInfoLog(prog, log_len, &written_len, &(*log)[0]);
		dbg_assert(written_len == (log_len -1));
		
		log->resize(written_len);
		
		return true;
	}
}

static bool load_shader (GLenum type, cstr filepath, GLuint* out) {
	bool success = true;
	
	GLuint shad = glCreateShader(type);
	
	std::string text;
	if (!read_text_file(filepath, &text)) {
		log_warning_asset_load(filepath);
		return false; // fail
	}
	
	{
		cstr ptr = text.c_str();
		glShaderSource(shad, 1, &ptr, NULL);
	}
	
	glCompileShader(shad);
	
	{
		GLint status;
		glGetShaderiv(shad, GL_COMPILE_STATUS, &status);
		
		std::string log_str;
		bool log_avail = get_shader_compile_log(shad, &log_str);
		
		success = status == GL_TRUE;
		if (!success) {
			// compilation failed
			log_warning("OpenGL error in shader compilation \"%s\"!\n>>>\n%s\n<<<\n", filepath, log_avail ? log_str.c_str() : "<no log available>");
		} else {
			// compilation success
			if (log_avail) {
				log_warning("OpenGL shader compilation log \"%s\":\n>>>\n%s\n<<<\n", filepath, log_str.c_str());
			}
		}
	}
	
	*out = shad;
	return success;
}
static GLuint load_program (cstr vert_filepath, cstr frag_filepath, GLuint* out) {
	bool success = true;
	
	GLuint prog = glCreateProgram();
	
	GLuint vert;
	GLuint frag;
	
	success = success && load_shader(GL_VERTEX_SHADER, vert_filepath, &vert);
	success = success && load_shader(GL_FRAGMENT_SHADER, frag_filepath, &frag);
	
	glAttachShader(prog, vert);
	glAttachShader(prog, frag);
	
	glLinkProgram(prog);
	
	{
		GLint status;
		glGetProgramiv(prog, GL_LINK_STATUS, &status);
		
		std::string log_str;
		bool log_avail = get_program_link_log(prog, &log_str);
		
		success = status == GL_TRUE;
		if (!success) {
			// linking failed
			log_warning("OpenGL error in shader linkage \"%s\"|\"%s\"!\n>>>\n%s\n<<<\n", vert_filepath, frag_filepath, log_avail ? log_str.c_str() : "<no log available>");
		} else {
			// linking success
			if (log_avail) {
				log_warning("OpenGL shader linkage log \"%s\"|\"%s\":\n>>>\n%s\n<<<\n", vert_filepath, frag_filepath, log_str.c_str());
			}
		}
	}
	
	glDetachShader(prog, vert);
	glDetachShader(prog, frag);
	
	glDeleteShader(vert);
	glDeleteShader(frag);
	
	*out = prog;
	return success;
}
static void unload_program (GLuint prog) {
	glDeleteProgram(prog);
}

struct Base_Uniform {
	GLint	loc;
	
	void get_loc (GLuint prog, cstr name) {
		loc = glGetUniformLocation(prog, name);
	}
};

struct Uniform_M4 : public Base_Uniform {
	void set (m4 m) {	glUniformMatrix4fv(loc, 1, GL_FALSE, &m.arr[0][0]); }
};
struct Uniform_V2 : public Base_Uniform {
	void set (v2 v) {	glUniform2fv(loc, 1, &v.x); }
};
struct Uniform_IV2 : public Base_Uniform {
	void set (iv2 v) {	glUniform2iv(loc, 1, &v.x); }
};

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static u8* load_texture2d_rgba8 (cstr filepath, iv2* dim) {
	stbi_set_flip_vertically_on_load(true); // OpenGL has textues bottom-up
	int n;
	return stbi_load(filepath, &dim->x, &dim->y, &n, 4);
}

struct Texture2D {
	GLuint	tex;
	iv2		dim;
	
	cstr	filename;
	
	#if RZ_AUTO_FILE_RELOADING
	File_Change_Poller	fc;
	#endif
	
	void init_load (cstr filename_) {
		filename = filename_;
		
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		
		auto filepath = prints("assets_src/textures/%s", filename);
		
		auto* data = load_texture2d_rgba8(filepath.c_str(), &dim);
		if (!data) {
			log_warning_asset_load(filepath.c_str());
		} else {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, dim.x,dim.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		
		#if RZ_AUTO_FILE_RELOADING
		fc.init(filepath.c_str());
		#endif
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
	
	bool reload_if_needed () {
		auto filepath = prints("assets_src/textures/%s", filename);
		
		bool reload = fc.poll_did_change(filepath.c_str());
		if (!reload) return false;
		
		printf("texture source file changed, reloading \"%s\".\n", filepath.c_str());
		
		auto* data = load_texture2d_rgba8(filepath.c_str(), &dim);
		if (!data) return false;
		
		glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, dim.x,dim.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		
		glGenerateMipmap(GL_TEXTURE_2D);
		
		return true;
	}
	
};

struct Uniform_Texture {
	GLint	unit;
	
	void bind (Texture2D const& t) {
		glActiveTexture(GL_TEXTURE0 +unit);
		glBindTexture(GL_TEXTURE_2D, t.tex);
	}
};

struct Base_Shader {
	GLuint		prog;
	
	cstr		vert_filename;
	cstr		frag_filename;
	
	#if RZ_AUTO_FILE_RELOADING
	File_Change_Poller	fc_vert;
	File_Change_Poller	fc_frag;
	#endif
	
	void init_load (cstr vert_filename_, cstr frag_filename_) {
		vert_filename = vert_filename_;
		frag_filename = frag_filename_;
		
		_load(&prog);
		
		#if RZ_AUTO_FILE_RELOADING
		auto vert_filepath = prints("shaders/%s", vert_filename);
		auto frag_filepath = prints("shaders/%s", frag_filename);
		
		fc_vert.init(vert_filepath.c_str());
		fc_frag.init(frag_filepath.c_str());
		#endif
	}
	bool _load (GLuint* out) {
		auto vert_filepath = prints("shaders/%s", vert_filename);
		auto frag_filepath = prints("shaders/%s", frag_filename);
		
		return load_program(vert_filepath.c_str(), frag_filepath.c_str(), out);
	}
	bool try_reload () { // we keep the old shader as long as the new one has compilation errors
		GLuint tmp;
		bool success = _load(&tmp);
		if (success) {
			unload_program(prog);
			prog = tmp;
		}
		return success;
	}
	bool reload_if_needed () {
		#if RZ_AUTO_FILE_RELOADING
		
		auto vert_filepath = prints("shaders/%s", vert_filename);
		auto frag_filepath = prints("shaders/%s", frag_filename);
		
		bool reloaded = fc_vert.poll_did_change(vert_filepath.c_str()) || fc_frag.poll_did_change(frag_filepath.c_str());
		if (reloaded) {
			printf("shader source changed, reloading shader \"%s\"|\"%s\".\n", vert_filepath.c_str(), frag_filepath.c_str());
			reloaded = try_reload();
		}
		return reloaded;
		#else
		return false;
		#endif
	}
	
	void bind () {
		glUseProgram(prog);
	}
};

struct Common_Uniforms {
	Uniform_M4		world_to_clip;
	Uniform_V2		mcursor_pos;
	Uniform_V2		screen_dim;
	
	void set (m4 world_to_clip_, v2 mcursor_pos_px, v2 screen_dim_px) {
		world_to_clip.set(	world_to_clip_ );
		mcursor_pos.set(	mcursor_pos_px );
		screen_dim.set(		screen_dim_px );
	}
};

struct Shader : public Base_Shader {
	Common_Uniforms		common;
	Uniform_M4			skybox_to_clip;
	
	void init_load (cstr v, cstr f) {
		Base_Shader::init_load(v, f);
		
		common.world_to_clip.get_loc(prog, "world_to_clip");
		common.mcursor_pos.get_loc(prog, "mcursor_pos");
		common.screen_dim.get_loc(prog, "screen_dim");
		
		skybox_to_clip.get_loc(prog, "skybox_to_clip");
	}
};
struct Shader_Tex : public Base_Shader {
	Common_Uniforms		common;
	Uniform_V2			tex_dim;
	
	Uniform_Texture		tex0;
	
	void init_load (cstr v, cstr f) {
		Base_Shader::init_load(v, f);
		
		common.world_to_clip.get_loc(prog, "world_to_clip");
		common.mcursor_pos.get_loc(prog, "mcursor_pos");
		common.screen_dim.get_loc(prog, "screen_dim");
		
		tex_dim.get_loc(prog, "tex_dim");
		
		tex0.unit = 0;
		glUniform1i( glGetUniformLocation(prog, "tex0"), 0 );
	}
};

struct Mesh_Vertex {
	v3	pos;
	v3	norm;
	v2	uv;
	v3	col;
	
	bool operator== (Mesh_Vertex const& r) const {
		//return all(pos == r.pos) && all(norm == r.norm) && all(uv == r.uv) && all(col == r.col);
		return memcmp(this, &r, sizeof(Mesh_Vertex)) == 0;
	}
};
static constexpr v3 MESH_DEFAULT_NORM =		0;
static constexpr v2 MESH_DEFAULT_UV =		0.5f;
static constexpr v3 MESH_DEFAULT_COL =		1;

typedef u32 vert_indx_t;

struct Mesh_Vbo {
	GLuint						vbo_vert;
	GLuint						vbo_indx;
	std::vector<Mesh_Vertex>	vertecies;
	std::vector<vert_indx_t>	indices;
	
	void init () {
		glGenBuffers(1, &vbo_vert);
		glGenBuffers(1, &vbo_indx);
	}
	
	void clear () {
		vertecies.clear();
		indices.clear();
	}
	
	void upload () {
		glBindBuffer(GL_ARRAY_BUFFER, vbo_vert);
		glBufferData(GL_ARRAY_BUFFER, vector_size_bytes(vertecies), NULL, GL_STATIC_DRAW);
		glBufferData(GL_ARRAY_BUFFER, vector_size_bytes(vertecies), vertecies.data(), GL_STATIC_DRAW);
		
		if (indices.size()) {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_indx);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, vector_size_bytes(indices), NULL, GL_STATIC_DRAW);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, vector_size_bytes(indices), indices.data(), GL_STATIC_DRAW);
		}
	}
	
	void bind (Base_Shader const* shad) {
		
		glBindBuffer(GL_ARRAY_BUFFER, vbo_vert);
		
		GLint pos =		glGetAttribLocation(shad->prog, "pos_world");
		glEnableVertexAttribArray(pos);
		glVertexAttribPointer(pos,	3, GL_FLOAT, GL_FALSE, sizeof(Mesh_Vertex), (void*)offsetof(Mesh_Vertex, pos));
		
		GLint norm =	glGetAttribLocation(shad->prog, "norm_world");
		glEnableVertexAttribArray(norm);
		glVertexAttribPointer(norm,	3, GL_FLOAT, GL_FALSE, sizeof(Mesh_Vertex), (void*)offsetof(Mesh_Vertex, norm));
		
		GLint uv =		glGetAttribLocation(shad->prog, "uv");
		glEnableVertexAttribArray(uv);
		glVertexAttribPointer(uv,	2, GL_FLOAT, GL_FALSE, sizeof(Mesh_Vertex), (void*)offsetof(Mesh_Vertex, uv));
		
		GLint col =		glGetAttribLocation(shad->prog, "col");
		glEnableVertexAttribArray(col);
		glVertexAttribPointer(col,	3, GL_FLOAT, GL_FALSE, sizeof(Mesh_Vertex), (void*)offsetof(Mesh_Vertex, col));
		
		if (indices.size()) {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_indx);
		} else {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}
	}
	
	void draw_entire (Base_Shader const* shad) {
		bind(shad);
		
		if (indices.size()) {
			glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, NULL);
		} else {
			glDrawArrays(GL_TRIANGLES, 0, vertecies.size());
		}
	}
};
