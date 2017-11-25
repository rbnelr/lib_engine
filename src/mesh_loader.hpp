
namespace parse {
	struct str {
		char*	ptr;
		u32		len;
		
		operator bool () { return ptr; }
	};
	
	static bool comp (str cr l, cstr r) {
		char const* lcur = l.ptr;
		char const* lend = lcur +l.len;
		char const* rcur = r;
		while (*rcur != '\0') {
			if (*rcur++ != *lcur++) return false;
		}
		return lcur == lend;
	}
	
	static bool whitespace_c (char c) {	return c == ' ' || c == '\t'; }
	static str whitespace (char** pcur) {
		char* ret = *pcur;
		if (!whitespace_c(*ret)) return {};
		
		while (whitespace_c(**pcur)) ++(*pcur);
		
		return { ret, (u32)(*pcur -ret) };
	}
	
	static bool newline_c (char c) {	return c == '\n' || c == '\r'; }
	static str newline (char** pcur) {
		char* ret = *pcur;
		if (!newline_c(*ret)) return {};
		
		char c = **pcur;
		++(*pcur);
		
		if (newline_c(**pcur) && **pcur != c) ++(*pcur);
		
		return { ret, (u32)(*pcur -ret) };
	}
	
	static str rest_of_line (char** pcur) {
		char* ret = *pcur;
		if (newline_c(*ret) || *ret == '\0') return {};
		
		while (!newline_c(**pcur) && **pcur != '\0') ++(*pcur);
		
		newline(pcur);
		
		return { ret, (u32)(*pcur -ret) };
	}
	
	static bool identifier_c (char c) {	return (c >= 'A' && c <= 'Z')||(c >= 'a' && c <= 'z')|| c == '_'; }
	static str identifier (char** pcur) {
		char* ret = *pcur;
		if (!identifier_c(*ret)) return {};
		
		while (identifier_c(**pcur)) ++(*pcur);
		
		return { ret, (u32)(*pcur -ret) };
	}
	
	static bool token_c (char c) {	return !whitespace_c(c) && !newline_c(c) && c != '\0'; }
	static str token (char** pcur) {
		char* ret = *pcur;
		if (!token_c(*ret)) return {};
		
		while (token_c(**pcur)) ++(*pcur);
		
		return { ret, (u32)(*pcur -ret) };
	}
	
	static bool sign_c (char c) {	return c == '-' || c == '+'; }
	static bool digit_c (char c) {	return c >= '0' && c <= '9'; }
	
	static str int_ (char** pcur, u32* val) {
		char* ret = *pcur;
		if (!sign_c(*ret) && !digit_c(*ret)) return {};
		
		if (sign_c(**pcur)) ++(*pcur);
		
		while (digit_c(**pcur)) ++(*pcur);
		
		if (val) *val = (u32)atoi(ret);
		return { ret, (u32)(*pcur -ret) };
	}
	static str float_ (char** pcur, f32* val) {
		char* ret = *pcur;
		if (!sign_c(*ret) && !digit_c(*ret)) return {};
		
		if (sign_c(**pcur)) ++(*pcur);
		
		while (digit_c(**pcur)) ++(*pcur);
		
		if (**pcur == '.') ++(*pcur);
		
		while (digit_c(**pcur)) ++(*pcur);
		
		if (val) *val = (f32)atof(ret);
		return { ret, (u32)(*pcur -ret) };
	}
	
}

static void load_mesh (std::vector<Mesh_Vertex>* data, cstr name, v3 pos_offs=0) {
	
	std::string filepath = prints("assets_src/meshes/%s", name);
	
	data->clear();
	
	struct Vert_Indecies {
		u32		pos;
		u32		norm;
		u32		uv;
	};
	struct Triangle {
		Vert_Indecies arr[3];
	};
	
	std::vector<v3> poss;
	std::vector<v2> uvs;
	std::vector<v3> nrms;
	std::vector<Triangle> tris;
	
	poss.reserve(4*1024);
	uvs.reserve(4*1024);
	nrms.reserve(4*1024);
	tris.reserve(4*1024);
	
	{
		std::string file;
		if (!read_text_file(filepath.c_str(), &file)) {
			dbg_assert(false, "Mesh file \"%s\" not found!", filepath.c_str());
			return;
		}
		
		char* cur = &file[0];
		char* res;
		
		using namespace parse;
		auto ignore_line = [&] () {
			rest_of_line(&cur);
			newline(&cur);
		};
		
		auto parse_vec3 = [&] () -> v3 {
			v3 v;
			
			whitespace(&cur);
			if (!float_(&cur, &v.x)) goto error;
			
			whitespace(&cur);
			if (!float_(&cur, &v.y)) goto error;
			
			whitespace(&cur);
			if (!float_(&cur, &v.z)) goto error;
			
			if (!newline(&cur)) {
				log_warning("load_mesh: \"%s\" Too many components in vec3 parsing, ignoring rest!");
				ignore_line();
			}
			
			return v;
			
			error: {
				log_warning("load_mesh: \"%s\" Error in vec3 parsing, setting to NAN!");
				return QNAN;
			}
		};
		auto parse_vec2 = [&] () -> v2 {
			v2 v;
			
			whitespace(&cur);
			if (!float_(&cur, &v.x)) goto error;
			
			whitespace(&cur);
			if (!float_(&cur, &v.y)) goto error;
			
			if (!newline(&cur)) {
				log_warning("load_mesh: \"%s\" Too many components in vec2 parsing, ignoring rest!");
				ignore_line();
			}
			
			return v;
			
			error: {
				log_warning("load_mesh: \"%s\" Error in vec2 parsing, setting to NAN!");
				return QNAN;
			}
		};
		
		str obj_name = {};
		
		auto face = [&] () {
			Triangle tri;
			
			for (ui i=0; i<3; ++i) {
				whitespace(&cur);
				if (!int_(&cur, &tri.arr[i].pos) || tri.arr[i].pos == 0) goto error;
				
				if (*cur++ != '/') goto error;
				if (!int_(&cur, &tri.arr[i].uv) || tri.arr[i].uv == 0) goto error;
				
				if (*cur++ != '/') goto error;
				if (!int_(&cur, &tri.arr[i].norm) || tri.arr[i].norm == 0) goto error;
			}
			
			if (!newline(&cur)) goto error;
			
			tris.push_back(tri);
			
			return;
			
			error: {
				log_warning("load_mesh: \"%s\" Error in face parsing, setting to 0!", filepath.c_str());
				tris.push_back({});
			}
		};
		
		for (ui line_i=0;; ++line_i) {
			
			auto tok = token(&cur);
			if (!tok) {
				log_warning("load_mesh: \"%s\" Missing line token, ignoring line!", filepath.c_str());
				rest_of_line(&cur); // skip line
				
			} else {
				if (		comp(tok, "v") ) {
					poss.push_back( parse_vec3() );
				}
				else if (	comp(tok, "vt") ) {
					uvs.push_back( parse_vec2() );
				}
				else if (	comp(tok, "vn") ) {
					nrms.push_back( parse_vec3() );
				}
				else if (	comp(tok, "f") ) {
					face();
				}
				else if (	comp(tok, "o") ) {
					whitespace(&cur);
					
					if (obj_name) {
						dbg_assert(false, "load_mesh: \"%s\" More than 1 object in file, not supported!");
						return;
					}
					
					obj_name = rest_of_line(&cur);
					newline(&cur);
				}
				else if (	comp(tok, "s") ||
							comp(tok, "mtllib") ||
							comp(tok, "usemtl") ||
							comp(tok, "#") ) {
					ignore_line();
				}
				else {
					log_warning("load_mesh: \"%s\" Unknown line token \"%.*s\", ignoring line!", filepath.c_str(), tok.len,tok.ptr);
					ignore_line();
				}
			}
			
			if (*cur == '\0') break;
		}
	}
	
	{
		data->reserve( tris.size() * 3 ); // max possible size
		
		//std::unordered_map<Mesh_Vertex, u32> 
		
		for (auto& t : tris) {
			for (ui i=0; i<3; ++i) {
				Mesh_Vertex exp;
				
				exp.pos =	poss[t.arr[i].pos -1] +pos_offs;
				exp.norm =	nrms[t.arr[i].norm -1];
				exp.uv =	uvs[t.arr[i].uv -1];
				
				data->push_back(exp);
			}
		}
		
	}
}
