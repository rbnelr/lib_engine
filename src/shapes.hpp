
static void gen_tetrahedron (std::vector<Mesh_Vertex>* data, f32 r, v3 pos_world, m3 ori) {
	
	pos_world += v3(0,0, r * (1.0f/3));
	
	f32 SIN_0	= +0.0f;
	f32 SIN_120	= +0.86602540378443864676372317075294f;
	f32 SIN_240	= -0.86602540378443864676372317075294f;
	f32 COS_0	= +1.0f;
	f32 COS_120	= -0.5f;
	f32 COS_240	= -0.54f;
	
	auto out = vector_append(data, 4*3);
	
	*out++ = { ori * (r*v3(COS_0,	SIN_0,		-1.0f/3))	+pos_world,	MESH_DEFAULT_NORM, MESH_DEFAULT_UV, v3(1,0,0) };
	*out++ = { ori * (r*v3(COS_240,	SIN_240,	-1.0f/3))	+pos_world,	MESH_DEFAULT_NORM, MESH_DEFAULT_UV, v3(0,0,1) };
	*out++ = { ori * (r*v3(COS_120,	SIN_120,	-1.0f/3))	+pos_world,	MESH_DEFAULT_NORM, MESH_DEFAULT_UV, v3(0,1,0) };
	
	*out++ = { ori * (r*v3(COS_0,	SIN_0,		-1.0f/3))	+pos_world,	MESH_DEFAULT_NORM, MESH_DEFAULT_UV, v3(1,0,0) };
	*out++ = { ori * (r*v3(COS_120,	SIN_120,	-1.0f/3))	+pos_world,	MESH_DEFAULT_NORM, MESH_DEFAULT_UV, v3(0,1,0) };
	*out++ = { ori * (r*v3(0,		0,			+1.0f)	)	+pos_world,	MESH_DEFAULT_NORM, MESH_DEFAULT_UV, v3(1,1,1) };
	
	*out++ = { ori * (r*v3(COS_120,	SIN_120,	-1.0f/3))	+pos_world,	MESH_DEFAULT_NORM, MESH_DEFAULT_UV, v3(0,1,0) };
	*out++ = { ori * (r*v3(COS_240,	SIN_240,	-1.0f/3))	+pos_world,	MESH_DEFAULT_NORM, MESH_DEFAULT_UV, v3(0,0,1) };
	*out++ = { ori * (r*v3(0,		0,			+1.0f)	)	+pos_world,	MESH_DEFAULT_NORM, MESH_DEFAULT_UV, v3(1,1,1) };
	
	*out++ = { ori * (r*v3(COS_240,	SIN_240,	-1.0f/3))	+pos_world,	MESH_DEFAULT_NORM, MESH_DEFAULT_UV, v3(0,0,1) };
	*out++ = { ori * (r*v3(COS_0,	SIN_0,		-1.0f/3))	+pos_world,	MESH_DEFAULT_NORM, MESH_DEFAULT_UV, v3(1,0,0) };
	*out++ = { ori * (r*v3(0,		0,			+1.0f)	)	+pos_world,	MESH_DEFAULT_NORM, MESH_DEFAULT_UV, v3(1,1,1) };
	
	dbg_assert(out == data->end()); // check size calculation above
}
static void gen_cube (std::vector<Mesh_Vertex>* data, f32 r, v3 pos_world, m3 ori) {
	
	pos_world += v3(0,0,r);
	
	auto out = vector_append(data, 6*6);
	
	auto quad = [&] (v3 a, v3 b, v3 c, v3 d) {
		*out++ = { ori * (r*b) +pos_world, MESH_DEFAULT_NORM, MESH_DEFAULT_UV, b/2+0.5f };
		*out++ = { ori * (r*c) +pos_world, MESH_DEFAULT_NORM, MESH_DEFAULT_UV, c/2+0.5f };
		*out++ = { ori * (r*a) +pos_world, MESH_DEFAULT_NORM, MESH_DEFAULT_UV, a/2+0.5f };
		*out++ = { ori * (r*a) +pos_world, MESH_DEFAULT_NORM, MESH_DEFAULT_UV, a/2+0.5f };
		*out++ = { ori * (r*c) +pos_world, MESH_DEFAULT_NORM, MESH_DEFAULT_UV, c/2+0.5f };
		*out++ = { ori * (r*d) +pos_world, MESH_DEFAULT_NORM, MESH_DEFAULT_UV, d/2+0.5f };
	};
	
	v3 LLL = v3(-1,-1,-1);
	v3 HLL = v3(+1,-1,-1);
	v3 LHL = v3(-1,+1,-1);
	v3 HHL = v3(+1,+1,-1);
	v3 LLH = v3(-1,-1,+1);
	v3 HLH = v3(+1,-1,+1);
	v3 LHH = v3(-1,+1,+1);
	v3 HHH = v3(+1,+1,+1);
		
	quad(	LLH,
			HLH,
			HHH,
			LHH );
	
	quad(	HLL,
			LLL,
			LHL,
			HHL );
	
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
	
	dbg_assert(out == data->end()); // check size calculation above
}
static void gen_cylinder (std::vector<Mesh_Vertex>* data, f32 r, f32 l, u32 faces, v3 pos_world) {
	
	pos_world += v3(0,0,l/2);
	
	auto out = vector_append(data, faces*(3 +6 +3));
	
	auto quad = [&] (v3 a, v3 b, v3 c, v3 d) {
		*out++ = { v3(r,r,l/2)*b +pos_world, MESH_DEFAULT_NORM, MESH_DEFAULT_UV, b/2+0.5f };
		*out++ = { v3(r,r,l/2)*c +pos_world, MESH_DEFAULT_NORM, MESH_DEFAULT_UV, c/2+0.5f };
		*out++ = { v3(r,r,l/2)*a +pos_world, MESH_DEFAULT_NORM, MESH_DEFAULT_UV, a/2+0.5f };
		*out++ = { v3(r,r,l/2)*a +pos_world, MESH_DEFAULT_NORM, MESH_DEFAULT_UV, a/2+0.5f };
		*out++ = { v3(r,r,l/2)*c +pos_world, MESH_DEFAULT_NORM, MESH_DEFAULT_UV, c/2+0.5f };
		*out++ = { v3(r,r,l/2)*d +pos_world, MESH_DEFAULT_NORM, MESH_DEFAULT_UV, d/2+0.5f };
	};
	auto tri = [&] (v3 a, v3 b, v3 c) {
		*out++ = { v3(r,r,l/2)*a +pos_world, MESH_DEFAULT_NORM, MESH_DEFAULT_UV, a/2+0.5f };
		*out++ = { v3(r,r,l/2)*b +pos_world, MESH_DEFAULT_NORM, MESH_DEFAULT_UV, b/2+0.5f };
		*out++ = { v3(r,r,l/2)*c +pos_world, MESH_DEFAULT_NORM, MESH_DEFAULT_UV, c/2+0.5f };
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
	
	dbg_assert(out == data->end()); // check size calculation above
}
static void gen_iso_shphere (std::vector<Mesh_Vertex>* data, f32 r, u32 wfaces, u32 hfaces, v3 pos_world) {
	
	if (wfaces < 2 || hfaces < 2) return;
	
	pos_world += v3(0,0,r);
	
	auto out = vector_append(data, (hfaces-2)*wfaces*6 +2*wfaces*3);
	
	auto quad = [&] (v3 a, v3 b, v3 c, v3 d) {
		*out++ = { r*b +pos_world, MESH_DEFAULT_NORM, MESH_DEFAULT_UV, b/2+0.5f };
		*out++ = { r*c +pos_world, MESH_DEFAULT_NORM, MESH_DEFAULT_UV, c/2+0.5f };
		*out++ = { r*a +pos_world, MESH_DEFAULT_NORM, MESH_DEFAULT_UV, a/2+0.5f };
		*out++ = { r*a +pos_world, MESH_DEFAULT_NORM, MESH_DEFAULT_UV, a/2+0.5f };
		*out++ = { r*c +pos_world, MESH_DEFAULT_NORM, MESH_DEFAULT_UV, c/2+0.5f };
		*out++ = { r*d +pos_world, MESH_DEFAULT_NORM, MESH_DEFAULT_UV, d/2+0.5f };
	};
	auto tri = [&] (v3 a, v3 b, v3 c) {
		*out++ = { r*a +pos_world, MESH_DEFAULT_NORM, MESH_DEFAULT_UV, a/2+0.5f };
		*out++ = { r*b +pos_world, MESH_DEFAULT_NORM, MESH_DEFAULT_UV, b/2+0.5f };
		*out++ = { r*c +pos_world, MESH_DEFAULT_NORM, MESH_DEFAULT_UV, c/2+0.5f };
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
	
	dbg_assert(out == data->end()); // check size calculation above
}

static void gen_shapes (std::vector<Mesh_Vertex>* data) {
	gen_cube(data,			1,						v3( 0,+4,0), rotate3_Z(deg(37)));
	gen_tetrahedron(data,	2.0f / (1 +1.0f/3),		v3(+4,+4,0), rotate3_Z(deg(13)));
	gen_cylinder(data,		1, 2, 24, 				v3(-4,+4,0));
	gen_iso_shphere(data,	1, 64, 32,				v3(-4, 0,0));
}

static void gen_grid_floor (std::vector<Mesh_Vertex>* data) {
	
	f32	Z = 0;
	f32	tile_dim = 1;
	iv2	floor_r = 16;
	
	auto out = vector_append(data, floor_r.y*2 * floor_r.x*2 * 6);
	
	auto emit_quad = [&] (v3 pos, v3 col) {
		*out++ = { pos +v3(+0.5f,-0.5f,0), MESH_DEFAULT_NORM, MESH_DEFAULT_UV, col };
		*out++ = { pos +v3(+0.5f,+0.5f,0), MESH_DEFAULT_NORM, MESH_DEFAULT_UV, col };
		*out++ = { pos +v3(-0.5f,-0.5f,0), MESH_DEFAULT_NORM, MESH_DEFAULT_UV, col };
		
		*out++ = { pos +v3(-0.5f,-0.5f,0), MESH_DEFAULT_NORM, MESH_DEFAULT_UV, col };
		*out++ = { pos +v3(+0.5f,+0.5f,0), MESH_DEFAULT_NORM, MESH_DEFAULT_UV, col };
		*out++ = { pos +v3(-0.5f,+0.5f,0), MESH_DEFAULT_NORM, MESH_DEFAULT_UV, col };
	};
	
	for (s32 y=0; y<(floor_r.y*2); ++y) {
		for (s32 x=0; x<(floor_r.x*2); ++x) {
			v3 col = BOOL_XOR(EVEN(x), EVEN(y)) ? srgb(224,226,228) : srgb(41,49,52);
			emit_quad( v3(+0.5f,+0.5f,Z) +v3((f32)(x -floor_r.x)*tile_dim, (f32)(y -floor_r.y)*tile_dim, 0), col );
		}
	}
	
	dbg_assert(out == data->end()); // check size calculation above
}
