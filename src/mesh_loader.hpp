
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

#include <unordered_map>

namespace std {
	template<> struct hash<v2> {
		size_t operator() (v2 const& v) const {
			return	hash<f32>()(v.x) ^ hash<f32>()(v.y);
		}
	};
	template<> struct hash<v3> {
		size_t operator() (v3 const& v) const {
			return	hash<f32>()(v.x) ^ hash<f32>()(v.y) ^ hash<f32>()(v.z);
		}
	};
	template<> struct hash<Mesh_Vertex> {
		size_t operator() (Mesh_Vertex const& v) const {
			return hash<v3>()(v.pos) ^ hash<v3>()(v.norm) ^ hash<v2>()(v.uv) ^ hash<v3>()(v.col);
		}
	};
}

static void load_mesh (Mesh_Vbo* vbo, cstr filepath, hm transform) {
	
	struct Vert_Indecies {
		u32		pos;
		u32		uv;
		u32		norm;
	};
	struct Triangle {
		Vert_Indecies arr[3];
	};
	
	std::vector<v3> poss;
	std::vector<v2> uvs;
	std::vector<v3> norms;
	std::vector<Triangle> tris;
	
	poss.reserve(4*1024);
	uvs.reserve(4*1024);
	norms.reserve(4*1024);
	tris.reserve(4*1024);
	
	{ // load data from 
		std::string file;
		if (!read_text_file(filepath, &file)) {
			dbg_assert(false, "Mesh file \"%s\" not found!", filepath);
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
			Vert_Indecies vert[4];
			
			ui i = 0;
			for (;;) {
				
				whitespace(&cur);
				
				bool pos, uv, norm;
				
				pos = int_(&cur, &vert[i].pos);
				if (!pos || vert[i].pos == 0) goto error; // position missing
				
				if (*cur == '/') { ++cur;
					uv = int_(&cur, &vert[i].uv);
					if (uv && vert[i].uv == 0) goto error; // out of range index
					
					if (*cur++ != '/') goto error;
					
					norm = int_(&cur, &vert[i].norm);
					if (norm && vert[i].norm == 0) goto error; // out of range index
				}
				if (!uv)	vert[i].uv = 0;
				if (!norm)	vert[i].norm = 0;
				
				++i;
				if (newline(&cur) || *cur == '\0') break;
				if (i == 4) goto error; // only triangles and quads supported
			}
			
			if (i == 3) {
				tris.push_back({{	vert[0],
									vert[1],
									vert[2] }});
			} else /* i == 4 */ {
				tris.push_back({{	vert[1],
									vert[2],
									vert[0] }});
				tris.push_back({{	vert[0],
									vert[2],
									vert[3] }});
			}
			
			return;
			
			error: {
				log_warning("load_mesh: \"%s\" Error in face parsing, setting to 0!", filepath);
				tris.push_back({});
			}
		};
		
		for (ui line_i=0;; ++line_i) {
			
			auto tok = token(&cur);
			if (!tok) {
				log_warning("load_mesh: \"%s\" Missing line token, ignoring line!", filepath);
				rest_of_line(&cur); // skip line
				
			} else {
				if (		comp(tok, "v") ) {
					poss.push_back( parse_vec3() );
				}
				else if (	comp(tok, "vt") ) {
					uvs.push_back( parse_vec2() );
				}
				else if (	comp(tok, "vn") ) {
					norms.push_back( parse_vec3() );
				}
				else if (	comp(tok, "f") ) {
					face();
				}
				else if (	comp(tok, "o") ) {
					whitespace(&cur);
					
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
					log_warning("load_mesh: \"%s\" Unknown line token \"%.*s\", ignoring line!", filepath, tok.len,tok.ptr);
					ignore_line();
				}
			}
			
			if (*cur == '\0') break;
		}
	}
	
	vbo->vertecies.clear();
	vbo->indices.clear();
	
	{ // expand triangles from individually indexed poss/uvs/norms to non-indexed
		vbo->vertecies.reserve( tris.size() * 3 ); // max possible size
		vbo->indices.reserve( tris.size() * 3 );
		
		std::unordered_map<Mesh_Vertex, vert_indx_t> unique;
		
		for (auto& t : tris) {
			for (ui i=0; i<3; ++i) {
				Mesh_Vertex v;
				
				v.pos =		transform * poss[t.arr[i].pos -1];
				v.norm =	t.arr[i].norm ?	normalize(norms[t.arr[i].norm -1])	: MESH_DEFAULT_NORM;
				v.uv =		t.arr[i].uv ?	uvs[t.arr[i].uv -1]					: MESH_DEFAULT_UV;
				v.col =		MESH_DEFAULT_COL;
				
				auto entry = unique.find(v);
				bool is_unique = entry == unique.end();
				
				
				if (is_unique) {
					
					auto indx = (vert_indx_t)unique.size();
					unique.insert({v, indx});
					
					vbo->vertecies.push_back(v);
					
					vbo->indices.push_back(indx);
					
				} else {
					
					vbo->indices.push_back(entry->second);
					
				}
			}
		}
		
	}
	
}
