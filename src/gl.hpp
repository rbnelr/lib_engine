
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

static GLuint load_shader (GLenum type, cstr filepath) {
	GLuint shad = glCreateShader(type);
	
	std::string text;
	if (!read_text_file(filepath, &text)) {
		log_warning("shader file \"%s\" could not be loaded!", filepath);
		return 0;
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
		
		if (status == GL_FALSE) {
			// compilation failed
			log_warning("OpenGL error in shader compilation \"%s\"!\n>>>\n%s\n<<<\n", filepath, log_avail ? log_str.c_str() : "<no log available>");
		} else {
			// compilation success
			if (log_avail) {
				log_warning("OpenGL shader compilation log \"%s\":\n>>>\n%s\n<<<\n", filepath, log_str.c_str());
			}
		}
	}
	
	return shad;
}
static GLuint link_program (GLuint vert, GLuint frag, cstr vert_filepath, cstr frag_filepath) {
	GLuint prog = glCreateProgram();
	
	glAttachShader(prog, vert);
	glAttachShader(prog, frag);
	
	glLinkProgram(prog);
	
	{
		GLint status;
		glGetProgramiv(prog, GL_LINK_STATUS, &status);
		
		std::string log_str;
		bool log_avail = get_program_link_log(prog, &log_str);
		
		if (status == GL_FALSE) {
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
	
	return prog;
}

struct Base_Shader {
	GLuint			gl_prog;
	
	void load (cstr vert_name, cstr frag_name) {
		
		std::string vert_filepath = prints("shaders/%s", vert_name);
		std::string frag_filepath = prints("shaders/%s", frag_name);
		
		GLuint vert = load_shader(GL_VERTEX_SHADER, vert_filepath.c_str());
		GLuint frag = load_shader(GL_FRAGMENT_SHADER, frag_filepath.c_str());
		
		gl_prog = link_program(vert, frag, vert_filepath.c_str(), frag_filepath.c_str());
		
		glDeleteShader(vert);
		glDeleteShader(frag);
	}
};

struct Base_Uniform {
	GLint	gl_loc;
	
	void load (GLuint gl_prog, cstr name) {
		gl_loc = glGetUniformLocation(gl_prog, name);
	}
};

struct Uniform_M4 : public Base_Uniform {
	void set (m4 m) {	glUniformMatrix4fv(gl_loc, 1, GL_FALSE, &m.arr[0][0]); }
};

struct Shader : public Base_Shader {
	
	Uniform_M4		world_to_clip;
	
	void load () {
		Base_Shader::load("test.vert", "test.frag");
		
		world_to_clip.load(gl_prog, "world_to_clip");
	}
	void bind () {
		glUseProgram(gl_prog);
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

typedef u16 vert_indx_t;

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
		
		GLint pos =	glGetAttribLocation(shad.gl_prog, "pos_world");
		GLint col =	glGetAttribLocation(shad.gl_prog, "col");
		
		glEnableVertexAttribArray(pos);
		glVertexAttribPointer(pos,	3, GL_FLOAT, GL_FALSE, sizeof(Mesh_Vertex), (void*)offsetof(Mesh_Vertex, pos));
		
		glEnableVertexAttribArray(col);
		glVertexAttribPointer(col,	3, GL_FLOAT, GL_FALSE, sizeof(Mesh_Vertex), (void*)offsetof(Mesh_Vertex, col));
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_indx);
	}
	
	void draw_all (Base_Shader shad) {
		bind(shad);
		
		if (indices.size()) {
			glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, NULL);
		} else {
			glDrawArrays(GL_TRIANGLES, 0, vertecies.size());
		}
	}
};
