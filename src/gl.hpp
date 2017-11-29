
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

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_BMP	1
#define STBI_ONLY_PNG	1
#define STBI_ONLY_TGA	1
//#define STBI_ONLY_JPEG	1

#include "stb_image.h"

static u8* load_texture2d (cstr filepath, iv2* dim, ui* bbp) {
	stbi_set_flip_vertically_on_load(true); // OpenGL has textues bottom-up
	int n;
	u8* data = stbi_load(filepath, &dim->x, &dim->y, &n, 0);
	*bbp = (ui)n * 8;
	return data;
}

enum tex_type {
	TEX_TYPE_SRGB_A		=0,
	TEX_TYPE_LIN_RGB	,
};

struct Texture2D {
	GLuint		tex;
	iv2			dim;
	tex_type	type;
	
	cstr		filename;
	
	GLenum		internalFormat;
	GLenum		format;
	
	#if RZ_AUTO_FILE_RELOADING
	File_Change_Poller	fc;
	#endif
	
	void init_load (cstr fn, tex_type t) {
		filename = fn;
		type = t;
		
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		
		auto filepath = prints("assets_src/textures/%s", filename);
		
		if (!reload(filepath)) {
			log_warning_asset_load(filepath.c_str());
		}
		
		#if RZ_AUTO_FILE_RELOADING
		fc.init(filepath.c_str());
		#endif
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
	
	bool reload (std::string filepath) {
		
		#if 0
		std::string ext;
		std::string type_ext;
		{
			ui i = 0;
			std::string* exts[2] = { &ext, &type_ext };
			
			auto end = filepath.end();
			
			for (auto c = filepath.end(); c != filepath.begin();) { --c;
				if (*c == '.') {
					*exts[i] = std::string(c +1, end);
					
					end = c;
					
					++i;
					if (i == 2) break;
				}
			}
		}
		
		printf(">>> '%s' -> '%s' '%s'\n", filepath.c_str(), type_ext.c_str(), ext.c_str());
		#endif
		
		ui bbp;
		auto* data = load_texture2d(filepath.c_str(), &dim, &bbp);
		
		internalFormat = 0;
		format = 0;
		
		switch (bbp) {
			case 32:	format = GL_RGBA;	break;
			case 24:	format = GL_RGB;	break;
			
			default: dbg_assert(false);
		}
		
		switch (type) {
			case TEX_TYPE_SRGB_A:
				switch (bbp) {
					case 32:	internalFormat = GL_SRGB8_ALPHA8;	break;
					case 24:	internalFormat = GL_SRGB8;			break;
					
					default: dbg_assert(false);
				} break;
			
			case TEX_TYPE_LIN_RGB:
				switch (bbp) {
					case 32:	internalFormat = GL_RGBA8;
						log_warning("Texture2D:: RGBA image loaded as TEX_TYPE_LIN_RGB.");
						break;
					case 24:	internalFormat = GL_RGB;	break;
					
					default: dbg_assert(false);
				} break;
			
			default: dbg_assert(false);
		}
		
		if (data) {
			glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, dim.x,dim.y, 0, format, GL_UNSIGNED_BYTE, data);
			
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		
		return data != nullptr;
	}
	
	bool reload_if_needed () {
		#if RZ_AUTO_FILE_RELOADING
		auto filepath = prints("assets_src/textures/%s", filename);
		
		bool reloaded = fc.poll_did_change(filepath.c_str());
		if (!reloaded) return false;
		
		printf("texture source file changed, reloading \"%s\".\n", filepath.c_str());
		
		return reload(filepath);
		#else
		return false;
		#endif
	}
	
};

enum data_type {
	T_FLT		=0,
	T_V2		,
	T_V3		,
	T_V4		,
	
	T_INT		,
	T_IV2		,
	T_IV3		,
	T_IV4		,
	
	T_M3		,
	T_M4		,
};

struct Uniform {
	GLint		loc;
	data_type	type;
	cstr		name;
	
	Uniform (data_type t, cstr n): type{t}, name{n} {}
	
	void set (f32 v) {
		dbg_assert(type == T_FLT, "%s", name);
		glUniform1fv(loc, 1, &v);
	}
	void set (v2 v) {
		dbg_assert(type == T_V2, "%s", name);
		glUniform2fv(loc, 1, &v.x);
	}
	void set (v3 v) {
		dbg_assert(type == T_V3, "%s", name);
		glUniform3fv(loc, 1, &v.x);
	}
	void set (v4 v) {
		dbg_assert(type == T_V4, "%s", name);
		glUniform4fv(loc, 1, &v.x);
	}
	void set (iv2 v) {
		dbg_assert(type == T_IV2, "%s", name);
		glUniform2iv(loc, 1, &v.x);
	}
	void set (m3 v) {
		dbg_assert(type == T_M3, "%s", name);
		glUniformMatrix3fv(loc, 1, GL_FALSE, &v.arr[0][0]);
	}
	void set (m4 v) {
		dbg_assert(type == T_M4, "%s", name);
		glUniformMatrix4fv(loc, 1, GL_FALSE, &v.arr[0][0]);
	}
};

struct Shader {
	GLuint		prog;
	
	cstr		vert_filename;
	cstr		frag_filename;
	
	#if RZ_AUTO_FILE_RELOADING
	File_Change_Poller	fc_vert;
	File_Change_Poller	fc_frag;
	#endif
	
	struct Uniform_Texture {
		GLint			tex_unit;
		GLint			loc;
		cstr			name;
		
		Uniform_Texture (GLint unit, cstr n): tex_unit{unit}, name{n} {}
	};

	std::vector<Uniform>			uniforms;
	std::vector<Uniform_Texture>	textures;
	
	void init_load (cstr v, cstr f, std::initializer_list<Uniform> u, std::initializer_list<Uniform_Texture> t) {
		vert_filename = v;
		frag_filename = f;
		
		uniforms = u;
		textures = t;
		
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
		
		bool res = load_program(vert_filepath.c_str(), frag_filepath.c_str(), out);
		get_uniform_locations(*out);
		setup_uniform_textures(*out);
		return res;
	}
	
	void get_uniform_locations (GLuint prog) {
		for (auto& u : uniforms) {
			u.loc = glGetUniformLocation(prog, u.name);
			//if (u.loc <= -1) log_warning("Uniform not valid %s!", u.name);
		}
	}
	
	void setup_uniform_textures (GLuint prog) {
		glUseProgram(prog);
		for (auto& t : textures) {
			t.loc = glGetUniformLocation(prog, t.name);
			//if (t.loc <= -1) log_warning("Uniform Texture not valid '%s'!", t.name);
			glUniform1i(t.loc, t.tex_unit);
		}
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
	
	Uniform* get_uniform (cstr name) {
		for (auto& u : uniforms) {
			if (strcmp(u.name, name) == 0) {
				return &u;
			}
		}
		return nullptr;
	}
	
	template <typename T>
	void set_unif (cstr name, T v) {
		auto* u = get_uniform(name);
		if (u) u->set(v);
		else {
			log_warning("Uniform %s is not a uniform in the shader!", name);
		}
	}
};

struct Vertex_Layout {
	struct Attribute {
		cstr		name;
		data_type	type;
		u64			stride;
		u64			offs;
	};
	
	std::vector<Attribute>	attribs;
	
	Vertex_Layout (std::initializer_list<Attribute> a): attribs{a} {}
	
	void bind_attrib_arrays (Shader const* shad) {
		for (auto& a : attribs) {
			
			GLint loc = glGetAttribLocation(shad->prog, a.name);
			//if (loc <= -1) log_warning("Attribute %s is not used in the shader!", a.name);
			
			if (loc != -1) {
				dbg_assert(loc > -1);
				
				glEnableVertexAttribArray(loc);
				
				GLint comps = 1;
				GLenum type = GL_FLOAT;
				switch (a.type) {
					case T_FLT:	comps = 1;	type = GL_FLOAT;	break;
					case T_V2:	comps = 2;	type = GL_FLOAT;	break;
					case T_V3:	comps = 3;	type = GL_FLOAT;	break;
					case T_V4:	comps = 4;	type = GL_FLOAT;	break;
					
					case T_INT:	comps = 1;	type = GL_INT;		break;
					case T_IV2:	comps = 2;	type = GL_INT;		break;
					case T_IV3:	comps = 3;	type = GL_INT;		break;
					case T_IV4:	comps = 4;	type = GL_INT;		break;
					
					default: dbg_assert(false);
				}
				
				glVertexAttribPointer(loc, comps, type, GL_FALSE, a.stride, (void*)a.offs);
				
			}
		}
	}
};

struct Vbo {
	GLuint						vbo_vert;
	GLuint						vbo_indx;
	std::vector<byte>			vertecies;
	std::vector<vert_indx_t>	indices;
	
	Vertex_Layout*		layout;
	
	bool format_is_indexed () {
		return indices.size() > 0;
	}
	
	void init (Vertex_Layout* l) {
		layout = l;
		
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
		
		if (format_is_indexed()) {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_indx);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, vector_size_bytes(indices), NULL, GL_STATIC_DRAW);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, vector_size_bytes(indices), indices.data(), GL_STATIC_DRAW);
		}
	}
	
	void bind (Shader const* shad) {
		
		glBindBuffer(GL_ARRAY_BUFFER, vbo_vert);
		
		layout->bind_attrib_arrays(shad);
		
		if (format_is_indexed()) {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_indx);
		} else {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}
	}
	
	void draw_entire (Shader const* shad) {
		bind(shad);
		
		if (format_is_indexed()) {
			if (indices.size() > 0)
				glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, NULL);
		} else {
			if (vertecies.size() > 0)
				glDrawArrays(GL_TRIANGLES, 0, vertecies.size());
		}
	}
};
