
static void gen_tetrahedron (std::vector<byte>* data, hm transform, f32 r) {
	
	transform = transform * translateH(v3(0,0, r * (1.0f/3)));
	
	f32 SIN_0	= +0.0f;
	f32 SIN_120	= +0.86602540378443864676372317075294f;
	f32 SIN_240	= -0.86602540378443864676372317075294f;
	f32 COS_0	= +1.0f;
	f32 COS_120	= -0.5f;
	f32 COS_240	= -0.54f;
	
	auto out = (Mesh_Vertex*)&*vector_append(data, 4*3 * sizeof(Mesh_Vertex));
	
	*out++ = { transform * (r*v3(COS_0,		SIN_0,		-1.0f/3)),	DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, v3(1,0,0) };
	*out++ = { transform * (r*v3(COS_240,	SIN_240,	-1.0f/3)),	DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, v3(0,0,1) };
	*out++ = { transform * (r*v3(COS_120,	SIN_120,	-1.0f/3)),	DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, v3(0,1,0) };
	
	*out++ = { transform * (r*v3(COS_0,		SIN_0,		-1.0f/3)),	DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, v3(1,0,0) };
	*out++ = { transform * (r*v3(COS_120,	SIN_120,	-1.0f/3)),	DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, v3(0,1,0) };
	*out++ = { transform * (r*v3(0,			0,			+1.0f)	),	DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, v3(1,1,1) };
	
	*out++ = { transform * (r*v3(COS_120,	SIN_120,	-1.0f/3)),	DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, v3(0,1,0) };
	*out++ = { transform * (r*v3(COS_240,	SIN_240,	-1.0f/3)),	DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, v3(0,0,1) };
	*out++ = { transform * (r*v3(0,			0,			+1.0f)	),	DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, v3(1,1,1) };
	
	*out++ = { transform * (r*v3(COS_240,	SIN_240,	-1.0f/3)),	DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, v3(0,0,1) };
	*out++ = { transform * (r*v3(COS_0,		SIN_0,		-1.0f/3)),	DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, v3(1,0,0) };
	*out++ = { transform * (r*v3(0,			0,			+1.0f)	),	DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, v3(1,1,1) };
	
	dbg_assert(out == (Mesh_Vertex*)(data->data() +data->size())); // check size calculation above
}
static void gen_cube (std::vector<byte>* data, hm transform, f32 r) {
	
	transform = transform * translateH(v3(0,0,r));
	
	auto out = (Mesh_Vertex*)&*vector_append(data, 6*6 * sizeof(Mesh_Vertex));
	
	auto quad = [&] (v3 a, v3 b, v3 c, v3 d) {
		*out++ = { transform * (r*b), DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, b/2+0.5f };
		*out++ = { transform * (r*c), DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, c/2+0.5f };
		*out++ = { transform * (r*a), DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, a/2+0.5f };
		*out++ = { transform * (r*a), DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, a/2+0.5f };
		*out++ = { transform * (r*c), DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, c/2+0.5f };
		*out++ = { transform * (r*d), DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, d/2+0.5f };
	};
	
	v3 LLL = v3(-1,-1,-1);
	v3 HLL = v3(+1,-1,-1);
	v3 LHL = v3(-1,+1,-1);
	v3 HHL = v3(+1,+1,-1);
	v3 LLH = v3(-1,-1,+1);
	v3 HLH = v3(+1,-1,+1);
	v3 LHH = v3(-1,+1,+1);
	v3 HHH = v3(+1,+1,+1);
	
	quad(	LHL,
			LLL,
			LLH,
			LHH );
	
	quad(	HLL,
			HHL,
			HHH,
			HLH );
	
	quad(	LLL,
			HLL,
			HLH,
			LLH );
	
	quad(	HHL,
			LHL,
			LHH,
			HHH );
	
	quad(	HLL,
			LLL,
			LHL,
			HHL );
			
	quad(	LLH,
			HLH,
			HHH,
			LHH );
	
	dbg_assert(out == (Mesh_Vertex*)(data->data() +data->size())); // check size calculation above
}
static void gen_cylinder (std::vector<byte>* data, hm transform, f32 r, f32 l, u32 faces) {
	
	transform = transform * translateH(v3(0,0,l/2));
	
	auto out = (Mesh_Vertex*)&*vector_append(data, faces*(3 +6 +3) * sizeof(Mesh_Vertex));
	
	auto quad = [&] (v3 a, v3 b, v3 c, v3 d) {
		*out++ = { transform * (v3(r,r,l/2)*b), DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, b/2+0.5f };
		*out++ = { transform * (v3(r,r,l/2)*c), DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, c/2+0.5f };
		*out++ = { transform * (v3(r,r,l/2)*a), DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, a/2+0.5f };
		*out++ = { transform * (v3(r,r,l/2)*a), DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, a/2+0.5f };
		*out++ = { transform * (v3(r,r,l/2)*c), DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, c/2+0.5f };
		*out++ = { transform * (v3(r,r,l/2)*d), DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, d/2+0.5f };
	};
	auto tri = [&] (v3 a, v3 b, v3 c) {
		*out++ = { transform * (v3(r,r,l/2)*a), DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, a/2+0.5f };
		*out++ = { transform * (v3(r,r,l/2)*b), DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, b/2+0.5f };
		*out++ = { transform * (v3(r,r,l/2)*c), DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, c/2+0.5f };
	};
	
	for (u32 i=0; i<faces; ++i) {
		v2 a = rotate2( deg(360) * ((f32)(i+0) / (f32)faces) ) * v2(1,0);
		v2 b = rotate2( deg(360) * ((f32)(i+1) / (f32)faces) ) * v2(1,0);
		
		tri(	v3(b,	-1),
				v3(a,	-1),
				v3(0,0,	-1) );
		quad(	v3(a,	-1),
				v3(b,	-1),
				v3(b,	+1),
				v3(a,	+1) );
		tri(	v3(a,	+1),
				v3(b,	+1),
				v3(0,0,	+1) );
	}
	
	dbg_assert(out == (Mesh_Vertex*)(data->data() +data->size())); // check size calculation above
}
static void gen_iso_sphere (std::vector<byte>* data, hm transform, f32 r, u32 wfaces, u32 hfaces) {
	
	if (wfaces < 2 || hfaces < 2) return;
	
	transform = transform * translateH(v3(0,0,r));
	
	auto out = (Mesh_Vertex*)&*vector_append(data, ((hfaces-2)*wfaces*6 +2*wfaces*3) * sizeof(Mesh_Vertex));
	
	auto quad = [&] (v3 a, v3 b, v3 c, v3 d) {
		*out++ = { transform * (r*b), DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, b/2+0.5f };
		*out++ = { transform * (r*c), DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, c/2+0.5f };
		*out++ = { transform * (r*a), DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, a/2+0.5f };
		*out++ = { transform * (r*a), DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, a/2+0.5f };
		*out++ = { transform * (r*c), DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, c/2+0.5f };
		*out++ = { transform * (r*d), DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, d/2+0.5f };
	};
	auto tri = [&] (v3 a, v3 b, v3 c) {
		*out++ = { transform * (r*a), DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, a/2+0.5f };
		*out++ = { transform * (r*b), DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, b/2+0.5f };
		*out++ = { transform * (r*c), DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, c/2+0.5f };
	};
	
	for (u32 j=0; j<hfaces; ++j) {
		
		m3 rot_ha = rotate3_Y( deg(180) * ((f32)(j+0) / (f32)hfaces) );
		m3 rot_hb = rotate3_Y( deg(180) * ((f32)(j+1) / (f32)hfaces) );
		
		for (u32 i=0; i<wfaces; ++i) {
			m3 rot_wa = rotate3_Z( deg(360) * ((f32)(i+0) / (f32)wfaces) );
			m3 rot_wb = rotate3_Z( deg(360) * ((f32)(i+1) / (f32)wfaces) );
			
			if (j == 0) {
				tri(	v3(0,0,1),
						rot_wa * rot_hb * v3(0,0,1),
						rot_wb * rot_hb * v3(0,0,1) );
			} else if (j == (hfaces -1)) {
				tri(	rot_wb * rot_ha * v3(0,0,1),
						rot_wa * rot_ha * v3(0,0,1),
						v3(0,0,-1) );
			} else {
				quad(	rot_wb * rot_ha * v3(0,0,1),
						rot_wa * rot_ha * v3(0,0,1),
						rot_wa * rot_hb * v3(0,0,1),
						rot_wb * rot_hb * v3(0,0,1) );
			}
		}
	}
	
	dbg_assert(out == (Mesh_Vertex*)(data->data() +data->size())); // check size calculation above
}

static void gen_tile_floor (std::vector<byte>* data) {
	
	f32	Z = 0;
	f32	tile_dim = 1;
	iv2	floor_r = 20;
	
	auto out = (Mesh_Vertex*)&*vector_append(data, ((floor_r.y*2 * floor_r.x*2 * 6) / 2) * sizeof(Mesh_Vertex));
	
	auto emit_quad = [&] (v3 pos, bool checker) {
		if (checker) {
			v3 col = 0 ? srgb(224,226,228) : srgb(41,49,52);
			*out++ = { pos +v3(+0.5f,-0.5f,0), DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, col };
			*out++ = { pos +v3(+0.5f,+0.5f,0), DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, col };
			*out++ = { pos +v3(-0.5f,-0.5f,0), DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, col };
			
			*out++ = { pos +v3(-0.5f,-0.5f,0), DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, col };
			*out++ = { pos +v3(+0.5f,+0.5f,0), DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, col };
			*out++ = { pos +v3(-0.5f,+0.5f,0), DEFAULT_NORM, DEFAULT_TANG, DEFAULT_UV, col };
		}
	};
	
	for (s32 y=0; y<(floor_r.y*2); ++y) {
		for (s32 x=0; x<(floor_r.x*2); ++x) {
			emit_quad( v3(+0.5f,+0.5f,Z) +v3((f32)(x -floor_r.x)*tile_dim, (f32)(y -floor_r.y)*tile_dim, 0), BOOL_XOR(EVEN(x), EVEN(y)) );
		}
	}
	
	dbg_assert(out == (Mesh_Vertex*)(data->data() +data->size())); // check size calculation above
}
