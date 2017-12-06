
static bool load_shader_source (strcr filepath, std::string* src_text IF_RZ_AUTO_FILE_RELOAD( , std::vector<File_Change_Poller>* fcs ) ) {
	vector_append(fcs)->init(filepath); // put file shader depends on into the file change poll list before we even know that it exists, so that if it does not exist we automaticly (re)load the shader if it gets created
	
	if (!read_text_file(filepath.c_str(), src_text)) return false;
	
	for (auto c=src_text->begin(); c!=src_text->end();) {
		
		if (*c == '$') {
			auto line_begin = c;
			++c;
			
			auto syntax_error = [&] () {
				
				while (*c != '\n' && *c != '\r') ++c;
				std::string line (line_begin, c);
				
				log_warning("load_shader_source:: expected '$include \"filename\"' syntax but got: '%s'!", line.c_str());
			};
			
			while (*c == ' ' || *c == '\t') ++c;
			
			auto cmd = c;
			
			while ((*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') || *c == '_') ++c;
			
			if (std::string(cmd, c).compare("include") == 0) {
				
				while (*c == ' ' || *c == '\t') ++c;
				
				if (*c != '"') {
					syntax_error();
					return false;
				}
				++c;
				
				auto filename_str_begin = c;
				
				while (*c != '"') ++c;
				
				std::string filename_str (filename_str_begin, c);
				
				if (*c != '"') {
					syntax_error();
					return false;
				}
				++c;
				
				while (*c == ' ' || *c == '\t') ++c;
				
				if (*c != '\r' && *c != '\n') {
					syntax_error();
					return false;
				}
				
				auto line_end = c;
				
				{
					auto last_slash = filepath.begin();
					for (auto ch=filepath.begin(); ch!=filepath.end(); ++ch) if (*ch == '/') last_slash = ch +1;
					
					std::string inc_path (filepath.begin(), last_slash);
					
					std::string inc_filepath = prints("%s%s", inc_path.c_str(), filename_str.c_str());
					
					std::string inc_text;
					if (!load_shader_source(inc_filepath, &inc_text IF_RZ_AUTO_FILE_RELOAD(,fcs) )) {
						log_warning("load_shader_source:: &include: '%s' could not be loaded!", inc_filepath.c_str());
						return false;
					}
					
					auto line_begin_i = line_begin -src_text->begin();
					
					src_text->erase(line_begin, line_end);
					src_text->insert(src_text->begin() +line_begin_i, inc_text.begin(), inc_text.end());
					
					c = src_text->begin() +line_begin_i +inc_text.length();
				}
				
			}
		} else {
			++c;
		}
		
	}
	
	return true;
}

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

static bool load_shader (GLenum type, strcr filepath, GLuint* out IF_RZ_AUTO_FILE_RELOAD( , std::vector<File_Change_Poller>* fcs ) ) {
	*out = 0;
	
	GLuint shad = glCreateShader(type);
	
	std::string src_text;
	if (!load_shader_source(filepath, &src_text IF_RZ_AUTO_FILE_RELOAD(,fcs))) {
		log_warning_asset_load(filepath.c_str());
		return false;
	}
	
	{
		cstr ptr = src_text.c_str();
		glShaderSource(shad, 1, &ptr, NULL);
	}
	
	glCompileShader(shad);
	
	bool success;
	{
		GLint status;
		glGetShaderiv(shad, GL_COMPILE_STATUS, &status);
		
		std::string log_str;
		bool log_avail = get_shader_compile_log(shad, &log_str);
		
		success = status == GL_TRUE;
		if (!success) {
			// compilation failed
			log_warning("OpenGL error in shader compilation \"%s\"!\n>>>\n%s\n<<<\n", filepath.c_str(), log_avail ? log_str.c_str() : "<no log available>");
		} else {
			// compilation success
			if (log_avail) {
				log_warning("OpenGL shader compilation log \"%s\":\n>>>\n%s\n<<<\n", filepath.c_str(), log_str.c_str());
			}
		}
	}
	
	*out = shad;
	return success;
}
static GLuint load_program (std::string vert_filepath, strcr frag_filepath, GLuint* out  IF_RZ_AUTO_FILE_RELOAD( , std::vector<File_Change_Poller>* fcs ) ) {
	
	GLuint prog = glCreateProgram();
	
	GLuint vert;
	GLuint frag;
	
	*out = 0;
	if (!load_shader(GL_VERTEX_SHADER, vert_filepath, &vert IF_RZ_AUTO_FILE_RELOAD(,fcs) )) return false;
	if (!load_shader(GL_FRAGMENT_SHADER, frag_filepath, &frag IF_RZ_AUTO_FILE_RELOAD(,fcs) )) return false;
	
	glAttachShader(prog, vert);
	glAttachShader(prog, frag);
	
	glLinkProgram(prog);
	
	bool success;
	{
		GLint status;
		glGetProgramiv(prog, GL_LINK_STATUS, &status);
		
		std::string log_str;
		bool log_avail = get_program_link_log(prog, &log_str);
		
		success = status == GL_TRUE;
		if (!success) {
			// linking failed
			log_warning("OpenGL error in shader linkage \"%s\"|\"%s\"!\n>>>\n%s\n<<<\n", vert_filepath.c_str(), frag_filepath, log_avail ? log_str.c_str() : "<no log available>");
		} else {
			// linking success
			if (log_avail) {
				log_warning("OpenGL shader linkage log \"%s\"|\"%s\":\n>>>\n%s\n<<<\n", vert_filepath.c_str(), frag_filepath, log_str.c_str());
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

namespace dds_n {
	typedef u32 DWORD;
	
	struct DDS_PIXELFORMAT {
		DWORD			dwSize;
		DWORD			dwFlags;
		DWORD			dwFourCC;
		DWORD			dwRGBBitCount;
		DWORD			dwRBitMask;
		DWORD			dwGBitMask;
		DWORD			dwBBitMask;
		DWORD			dwABitMask;
	};
	
	struct DDS_HEADER {
		DWORD			dwSize;
		DWORD			dwFlags;
		DWORD			dwHeight;
		DWORD			dwWidth;
		DWORD			dwPitchOrLinearSize;
		DWORD			dwDepth;
		DWORD			dwMipMapCount;
		DWORD			dwReserved1[11];
		DDS_PIXELFORMAT	ddspf;
		DWORD			dwCaps;
		DWORD			dwCaps2;
		DWORD			dwCaps3;
		DWORD			dwCaps4;
		DWORD			dwReserved2;
	};
	
	static constexpr DWORD DDSD_CAPS			=0x1;
	static constexpr DWORD DDSD_HEIGHT			=0x2;
	static constexpr DWORD DDSD_WIDTH			=0x4;
	static constexpr DWORD DDSD_PITCH			=0x8;
	static constexpr DWORD DDSD_PIXELFORMAT		=0x1000;
	static constexpr DWORD DDSD_MIPMAPCOUNT		=0x20000;
	static constexpr DWORD DDSD_LINEARSIZE		=0x80000;
	static constexpr DWORD DDSD_DEPTH			=0x800000;
	
	static constexpr DWORD DDSCAPS_COMPLEX		=0x8;
	static constexpr DWORD DDSCAPS_MIPMAP		=0x400000;
	static constexpr DWORD DDSCAPS_TEXTURE		=0x1000;
	
	static constexpr DWORD DDPF_ALPHAPIXELS		=0x1;
	static constexpr DWORD DDPF_ALPHA			=0x2;
	static constexpr DWORD DDPF_FOURCC			=0x4;
	static constexpr DWORD DDPF_RGB				=0x40;
	static constexpr DWORD DDPF_YUV				=0x200;
	static constexpr DWORD DDPF_LUMINANCE		=0x20000;

	struct DXT_Block_128 {
		u64	alpha_table; // 4-bit [4][4] table of alpha values
		u16	c0; // RGB 565
		u16	c1; // RGB 565
		u32 col_LUT; // 2-bit [4][4] LUT into c0 - c4
	};
	
	static DXT_Block_128 flip_vertical (DXT_Block_128 b) {
		
		b.alpha_table =
			(((b.alpha_table >>  0) & 0xffff) << 48) |
			(((b.alpha_table >> 16) & 0xffff) << 32) |
			(((b.alpha_table >> 32) & 0xffff) << 16) |
			(((b.alpha_table >> 48) & 0xffff) <<  0);
		
		b.col_LUT =
			(((b.col_LUT >>  0) & 0xff) << 24) |
			(((b.col_LUT >>  8) & 0xff) << 16) |
			(((b.col_LUT >> 16) & 0xff) <<  8) |
			(((b.col_LUT >> 24) & 0xff) <<  0);
		
		return b;
	}

	static void inplace_flip_DXT64_vertical (void* data, u32 w, u32 h) {
		
		DXT_Block_128* line_a =		(DXT_Block_128*)data;
		DXT_Block_128* line_b =		line_a +((h -1) * w);
		DXT_Block_128* line_a_end =	line_a +((h / 2) * w);
		
		for (u32 j=0; line_a != line_a_end; ++j) {
			
			for (u32 i=0; i<w; ++i) {
				DXT_Block_128 tmp = line_a[i];
				line_a[i] = flip_vertical(line_b[i]);
				line_b[i] = flip_vertical(tmp);
			}
			
			line_a += w;
			line_b -= w;
		}
	}
}

static void inplace_flip_vertical (void* data, u64 h, u64 stride) {
	dbg_assert((stride % 4) == 0);
	stride /= 4;
	
	u32* line_a =		(u32*)data;
	u32* line_b =		line_a +((h -1) * stride);
	u32* line_a_end =	line_a +((h / 2) * stride);
	
	for (u32 j=0; line_a != line_a_end; ++j) {
		
		for (u32 i=0; i<stride; ++i) {
			u32 tmp = line_a[i];
			line_a[i] = line_b[i];
			line_b[i] = tmp;
		}
		
		line_a += stride;
		line_b -= stride;
	}
}

struct Mip {
	byte*	data;
	u64		size;
	
	iv2		dim;
};

enum tex_type {
	TEX_TYPE_SRGB_A	=0, // srgb rgb and linear alpha
	TEX_TYPE_SRGB	,
	TEX_TYPE_LRGBA	,
	TEX_TYPE_LRGB	,
	
	TEX_TYPE_DXT1	,
	TEX_TYPE_DXT3	,
	TEX_TYPE_DXT5	,
};

static bool load_dds (cstr filepath, bool linear, tex_type* type, Data_Block* file_data, iv2* dim, std::vector<Mip>* mips) {
	using namespace dds_n;
	
	if (!read_entire_file(filepath, file_data)) return false; // fail
	byte* cur = file_data->data;
	byte* end = file_data->data +file_data->size;
	
	if (	(u64)(end -cur) < 4 ||
			memcmp(cur, "DDS ", 4) != 0 ) return false; // fail
	cur += 4;
	
	if (	(u64)(end -cur) < sizeof(DDS_HEADER) ) return false; // fail
	
	auto* header = (DDS_HEADER*)cur;
	cur += sizeof(DDS_HEADER);
	
	dbg_assert(header->dwSize == sizeof(DDS_HEADER));
	dbg_assert((header->dwFlags & (DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT)) == (DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT), "0x%x", header->dwFlags);
	dbg_assert(header->ddspf.dwSize == sizeof(DDS_PIXELFORMAT));
	
	if (!(header->dwFlags & DDSD_MIPMAPCOUNT) || !(header->dwCaps & DDSCAPS_MIPMAP)) {
		header->dwMipMapCount = 1;
	} else {
		dbg_assert(header->dwFlags & DDSD_MIPMAPCOUNT);
	}
	
	*dim = iv2((s32)header->dwWidth, (s32)header->dwHeight);
	
	mips->resize(header->dwMipMapCount);
	
	if (header->ddspf.dwFlags & DDPF_FOURCC) {
		if (		memcmp(&header->ddspf.dwFourCC, "DXT1", 4) == 0 )	*type = TEX_TYPE_DXT1;
		else if (	memcmp(&header->ddspf.dwFourCC, "DXT3", 4) == 0 )	*type = TEX_TYPE_DXT3;
		else if (	memcmp(&header->ddspf.dwFourCC, "DXT5", 4) == 0 )	*type = TEX_TYPE_DXT5;
		else															return false; // fail
		
		u64 block_size = *type == TEX_TYPE_DXT1 ? 8 : 16;
		
		s32 w=dim->x, h=dim->y;
		
		for (u32 i=0; i<header->dwMipMapCount; ++i) {
			u32 blocks_w = max( (u32)1, ((u32)w +3)/4 );
			u32 blocks_h = max( (u32)1, ((u32)h +3)/4 );
			
			u64 size =	blocks_w * blocks_h * block_size;
			if ((u64)(end -cur) < size) return false; // fail
			
			inplace_flip_DXT64_vertical(cur, blocks_w, blocks_h);
			
			(*mips)[i] = {cur, size, iv2(w,h)};
			
			if (w > 1) w /= 2;
			if (h > 1) h /= 2;
			
			cur += size;
		}
		
	} else {
		
		dbg_assert(header->ddspf.dwFlags & DDPF_RGB);
		
		switch (header->ddspf.dwRGBBitCount) {
			case 32: {
				dbg_assert(header->ddspf.dwFlags & DDPF_ALPHAPIXELS);
				
				*type = linear ? TEX_TYPE_LRGBA :	TEX_TYPE_SRGB_A;
			} break;
			case 24: {
				*type = linear ? TEX_TYPE_LRGB :	TEX_TYPE_SRGB;
			} break;
			
			default: dbg_assert(false);
		}
		
		dbg_assert(header->dwFlags & DDSD_PITCH);
		
		s32 w=dim->x, h=dim->y;
		
		for (u32 i=0; i<header->dwMipMapCount; ++i) {
			
			u64 size =	h * header->dwPitchOrLinearSize;
			if ((u64)(end -cur) < size) return false; // fail
			
			inplace_flip_vertical(cur, h, header->dwPitchOrLinearSize);
			
			(*mips)[i] = {cur, size, iv2(w,h)};
			
			if (w > 1) w /= 2;
			if (h > 1) h /= 2;
			
			cur += size;
		}
	}
	return true;
}
static bool load_img_stb (cstr filepath, bool linear, tex_type* type, Data_Block* file_data, iv2* dim, std::vector<Mip>* mips) {
	stbi_set_flip_vertically_on_load(true); // OpenGL has textues bottom-up
	
	int n;
	file_data->data = stbi_load(filepath, &dim->x, &dim->y, &n, 0);
	
	switch (n) {
		case 4:	*type = linear ? TEX_TYPE_LRGBA :	TEX_TYPE_SRGB_A;	break;
		case 3:	*type = linear ? TEX_TYPE_LRGB :	TEX_TYPE_SRGB;		break;
		default: dbg_assert(false);
	}
	
	file_data->size = dim->x * dim->y * n;
	
	mips->resize(1);
	(*mips)[0] = { file_data->data, file_data->size, *dim };
	
	return file_data->data != nullptr;
}

static constexpr bool	TEX_LINEAR = true;
static constexpr bool	TEX_SRGB = true;

static f32				max_aniso;

static bool load_texture2d (strcr filepath, bool linear, tex_type* type, Data_Block* file_data, iv2* dim, std::vector<Mip>* mips) {
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
	
	//printf(">>> '%s' -> '%s' '%s'\n", filepath.c_str(), type_ext.c_str(), ext.c_str());
	
	if (ext.compare("dds") == 0)	return load_dds(filepath.c_str(), linear, type, file_data, dim, mips);
	else							return load_img_stb(filepath.c_str(), linear, type, file_data, dim, mips);
}

struct Texture2D {
	GLuint				tex;
	
	cstr				filename;
	bool				linear; // linear colorspace (for normal maps, etc.)
	
	tex_type			type;
	Data_Block			data;
	iv2					dim;
	std::vector<Mip>	mips;
	
	#if RZ_AUTO_FILE_RELOAD
	File_Change_Poller	fc;
	#endif
	
	void init_storage () {
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
	}
	
	void init_load (cstr fn, bool l) {
		filename = fn;
		linear = l;
		
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, max_aniso);
		
		auto filepath = prints("%s/%s", textures_base_path, filename);
		
		data.data = nullptr;
		
		#if RZ_AUTO_FILE_RELOAD
		fc.init(filepath);
		#endif
		
		if (!reload(filepath)) {
			log_warning_asset_load(filepath.c_str());
		}
	}
	
	void upload_compressed (GLenum internalFormat) {
		//GL_TEXTURE_2D == tex needs tex to be bound
		
		dbg_assert((u32)mips.size() >= 1);
		
		s32 w=dim.x, h=dim.y;
		
		u32 mip_i;
		for (mip_i=0; mip_i<(u32)mips.size();) {
			auto& m = mips[mip_i];
			
			glCompressedTexImage2D(GL_TEXTURE_2D, mip_i, internalFormat, m.dim.x,m.dim.y, 0, m.size, m.data);
			
			if (++mip_i == (u32)mips.size()) break;
			
			if (w > 0) w /= 2;
			if (h > 0) h /= 2;
			if (w == 1 && h == 1) break;
		}
		
		if (mip_i != (u32)mips.size() || w != 1 || h != 1) {
			dbg_assert(false);
			
			glGenerateMipmap(GL_TEXTURE_2D);
		}
	}
	void upload (GLenum format, GLenum internalFormat) {
		
		dbg_assert((u32)mips.size() >= 1);
		
		s32 w=dim.x, h=dim.y;
		
		u32 mip_i;
		for (mip_i=0; mip_i<(u32)mips.size();) {
			auto& m = mips[mip_i];
			
			glTexImage2D(GL_TEXTURE_2D, mip_i, internalFormat, m.dim.x,m.dim.y, 0, format, GL_UNSIGNED_BYTE, m.data);
			
			if (++mip_i == (u32)mips.size()) break;
			
			if (w > 0) w /= 2;
			if (h > 0) h /= 2;
			if (w == 1 && h == 1) break;
		}
		
		if (mip_i != (u32)mips.size() || w != 1 || h != 1) {
			dbg_assert(mip_i == 1, "%u %u %u %u", mip_i, (u32)mips.size(), w, h);
			
			glGenerateMipmap(GL_TEXTURE_2D);
		}
	}
	bool reload (strcr filepath) {
		
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		
		glBindTexture(GL_TEXTURE_2D, tex);
		
		data.free();
		
		if (!load_texture2d(filepath, linear, &type, &data, &dim, &mips)) return false;
		
		switch (type) {
			case TEX_TYPE_SRGB_A	:	upload(GL_RGBA,	GL_SRGB8_ALPHA8);	break;
			case TEX_TYPE_SRGB		:	upload(GL_RGB,	GL_SRGB8);			break;
			case TEX_TYPE_LRGBA		:	upload(GL_RGBA,	GL_RGBA8);			break;
			case TEX_TYPE_LRGB		:	upload(GL_RGB,	GL_RGB);			break;
			
			case TEX_TYPE_DXT1:			upload_compressed(GL_COMPRESSED_RGB_S3TC_DXT1_EXT);		break;
			case TEX_TYPE_DXT3:			upload_compressed(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT);	break;
			case TEX_TYPE_DXT5:			upload_compressed(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT);	break;
			
			default: dbg_assert(false);
		}
		
		return true;
	}
	
	bool reload_if_needed () {
		#if RZ_AUTO_FILE_RELOAD
		bool reloaded = fc.poll_did_change();
		if (!reloaded) return false;
		
		printf("texture source file changed, reloading \"%s\".\n", filename);
		
		auto filepath = prints("%s/%s", textures_base_path, filename);
		
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
	
	#if RZ_AUTO_FILE_RELOAD
	std::vector<File_Change_Poller>	fcs;
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
		
		_load(&prog IF_RZ_AUTO_FILE_RELOAD( , &fcs ) );
	}
	bool _load (GLuint* out, IF_RZ_AUTO_FILE_RELOAD( std::vector<File_Change_Poller>* fcs ) ) {
		auto vert_filepath = prints("%s/%s", shaders_base_path, vert_filename);
		auto frag_filepath = prints("%s/%s", shaders_base_path, frag_filename);
		
		bool res = load_program(vert_filepath, frag_filepath, out IF_RZ_AUTO_FILE_RELOAD(,fcs) );
		if (res) {
			get_uniform_locations(*out);
			setup_uniform_textures(*out);
		}
		return res;
	}
	
	bool valid () {
		return prog != 0;
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
	
	bool reload_if_needed () {
		#if RZ_AUTO_FILE_RELOAD
		bool reloaded = false;
		for (auto& f : fcs) {
			if (f.poll_did_change()) {
				reloaded = true;
				break;
			}
		}
		
		if (reloaded) {
			printf("shader source changed, reloading shader \"%s\"|\"%s\".\n", vert_filename, frag_filename);
			
			// keep old data if the reloading of the shader fails
			GLuint tmp;
			#if RZ_AUTO_FILE_RELOAD
			std::vector<File_Change_Poller>	tmp_fcs;
			#endif
			
			reloaded = _load(&tmp, IF_RZ_AUTO_FILE_RELOAD(&tmp_fcs) );
			if (reloaded) {
				
				for (auto& f : fcs) f.close();
				
				fcs = tmp_fcs;
				
				unload_program(prog);
				
				prog = tmp;
			} else {
				for (auto& f : tmp_fcs) f.close();
			}
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
