
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
		log_warning("shader file \"%s\" could not be loaded!", filepath);
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
	GLint	gl_loc;
	
	void get_loc (GLuint gl_prog, cstr name) {
		gl_loc = glGetUniformLocation(gl_prog, name);
	}
};

struct Uniform_M4 : public Base_Uniform {
	void set (m4 m) {	glUniformMatrix4fv(gl_loc, 1, GL_FALSE, &m.arr[0][0]); }
};
struct Uniform_V2 : public Base_Uniform {
	void set (v2 v) {	glUniform2fv(gl_loc, 1, &v.x); }
};

struct Base_Shader {
	GLuint		gl_prog;
	
	cstr		vert_name;
	cstr		frag_name;
	
	#if RZ_DBG
	HANDLE		_vert_h;
	HANDLE		_frag_h;
	
	FILETIME	_vert_t;
	FILETIME	_frag_t;
	#endif
	
	Base_Shader (cstr v, cstr f): gl_prog{0}, vert_name{v}, frag_name{f} {}
	
	void init () {
		#if RZ_DBG
		std::string vert_filepath = prints("shaders/%s", vert_name);
		std::string frag_filepath = prints("shaders/%s", frag_name);
		
		{
			_vert_h = CreateFile(vert_filepath.c_str(), GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			dbg_assert(_vert_h != INVALID_HANDLE_VALUE);
			GetFileTime(_vert_h, NULL, NULL, &_vert_t);
		}
		{
			_frag_h = CreateFile(frag_filepath.c_str(), GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			dbg_assert(_frag_h != INVALID_HANDLE_VALUE);
			GetFileTime(_frag_h, NULL, NULL, &_frag_t);
		}
		#endif
	}
	bool _load (GLuint* out) {
		std::string vert_filepath = prints("shaders/%s", vert_name);
		std::string frag_filepath = prints("shaders/%s", frag_name);
		
		return load_program(vert_filepath.c_str(), frag_filepath.c_str(), out);
	}
	void load () {
		_load(&gl_prog);
	}
	bool try_reload () {
		GLuint tmp;
		bool success = _load(&tmp);
		if (success) {
			unload_program(gl_prog);
			gl_prog = tmp;
		}
		return success;
	}
	bool poll_reload_shader () {
		#if RZ_DBG
		std::string vert_filepath = prints("shaders/%s", vert_name);
		std::string frag_filepath = prints("shaders/%s", frag_name);
		
		FILETIME vt, ft;
		
		GetFileTime(_vert_h, NULL, NULL, &vt);
		GetFileTime(_frag_h, NULL, NULL, &ft);
		
		auto vr = CompareFileTime(&_vert_t, &vt);
		auto fr = CompareFileTime(&_frag_t, &ft);
		
		bool reloaded = vr != 0 || fr != 0;
		dbg_assert(vr == 0 || vr == -1);
		dbg_assert(fr == 0 || fr == -1);
		
		_vert_t = vt;
		_frag_t = ft;
		
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
		glUseProgram(gl_prog);
	}
};

struct Shader : public Base_Shader {
	Shader (cstr v, cstr f): Base_Shader{v,f} {}
	
	struct Common_Uniforms {
		Uniform_M4		world_to_clip;
		Uniform_V2		mcursor_pos;
		Uniform_V2		screen_dim;
		
		void set (m4 world_to_clip_, v2 mcursor_pos_px, v2 screen_dim_px) {
			world_to_clip.set(	world_to_clip_ );
			mcursor_pos.set(	mcursor_pos_px );
			screen_dim.set(		screen_dim_px );
		}
	} common;
	
	Uniform_M4			skybox_to_clip;
	
	void load () {
		Base_Shader::load();
		
		common.world_to_clip.get_loc(gl_prog, "world_to_clip");
		common.mcursor_pos.get_loc(gl_prog, "mcursor_pos");
		common.screen_dim.get_loc(gl_prog, "screen_dim");
		
		skybox_to_clip.get_loc(gl_prog, "skybox_to_clip");
	}
};

struct Mesh_Vertex {
	v3	pos;
	v3	norm;
	v2	uv;
	v3	col;
	
	bool operator== (Mesh_Vertex cr r) const {
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
	
	void clear () {
		vertecies.clear();
		indices.clear();
	}
	
	void gen () {
		glGenBuffers(1, &vbo_vert);
		glGenBuffers(1, &vbo_indx);
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
	
	void bind (Base_Shader shad) {
		
		glBindBuffer(GL_ARRAY_BUFFER, vbo_vert);
		
		GLint pos =		glGetAttribLocation(shad.gl_prog, "pos_world");
		glEnableVertexAttribArray(pos);
		glVertexAttribPointer(pos,	3, GL_FLOAT, GL_FALSE, sizeof(Mesh_Vertex), (void*)offsetof(Mesh_Vertex, pos));
		
		GLint norm =	glGetAttribLocation(shad.gl_prog, "norm_world");
		glEnableVertexAttribArray(norm);
		glVertexAttribPointer(norm,	3, GL_FLOAT, GL_FALSE, sizeof(Mesh_Vertex), (void*)offsetof(Mesh_Vertex, norm));
		
		GLint uv =		glGetAttribLocation(shad.gl_prog, "uv");
		glEnableVertexAttribArray(uv);
		glVertexAttribPointer(uv,	2, GL_FLOAT, GL_FALSE, sizeof(Mesh_Vertex), (void*)offsetof(Mesh_Vertex, uv));
		
		GLint col =		glGetAttribLocation(shad.gl_prog, "col");
		glEnableVertexAttribArray(col);
		glVertexAttribPointer(col,	3, GL_FLOAT, GL_FALSE, sizeof(Mesh_Vertex), (void*)offsetof(Mesh_Vertex, col));
		
		if (indices.size()) {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_indx);
		} else {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}
	}
	
	void draw_all (Base_Shader shad) {
		bind(shad);
		
		if (indices.size()) {
			glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, NULL);
		} else {
			glDrawArrays(GL_TRIANGLES, 0, vertecies.size());
		}
	}
};

struct Skybox {
	
	
	void draw () {
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glDrawArrays(GL_TRIANGLES, 0, 6*6);
	}
};
