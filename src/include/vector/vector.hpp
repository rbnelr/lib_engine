
//
#define T	f32
#define V2	fv2
#define V3	fv3
#define V4	fv4
	
	#include "vector_tv2.hpp"
	#include "vector_tv3.hpp"
	#include "vector_tv4.hpp"
	
#undef T
#undef V2
#undef V3
#undef V4

//
#define T	f64
#define V2	dv2
#define V3	dv3
#define V4	dv4
	
	#include "vector_tv2.hpp"
	#include "vector_tv3.hpp"
	#include "vector_tv4.hpp"
	
#undef T
#undef V2
#undef V3
#undef V4

//
#define T	s32
#define V2	s32v2
#define V3	s32v3
#define V4	s32v4
	
	#include "vector_tv2.hpp"
	#include "vector_tv3.hpp"
	#include "vector_tv4.hpp"
	
#undef T
#undef V2
#undef V3
#undef V4

//
#define T	f32
#define V2	fv2
#define V3	fv3
#define V4	fv4
#define M2	fm2
#define M3	fm3
#define M4	fm4

struct M2 {
	V2 arr[2];
	
	explicit M2 () {}
private: // don't allow a contructor with column mayor order because it could be confusing, use static functions row and column instead, still need a contructor though, to implement the functions below
	explicit constexpr M2 (V2 a, V2 b): arr{a,b} {}
public:
	
	static constexpr M2 row (		V2 a, V2 b ) {				return M2{V2(a.x,b.x),V2(b.y,b.y)}; }
	static constexpr M2 column (	V2 a, V2 b ) {				return M2{a,b}; }
	static constexpr M2 row (		T a, T b,
									T e, T f ) {				return M2{V2(a,e),V2(b,f)}; }
	static constexpr M2 ident () {								return row(1,0, 0,1); }
	
	M2& operator*= (M2 r);
};
struct M3 {
	V3 arr[3];
	
	explicit M3 () {}
private: //
	explicit constexpr M3 (V3 a, V3 b, V3 c): arr{a,b,c} {}
public:
	
	static constexpr M3 row (		V3 a, V3 b, V3 c ) {		return M3{V3(a.x,b.x,c.x),V3(a.y,b.y,c.y),V3(a.z,b.z,c.z)}; }
	static constexpr M3 column (	V3 a, V3 b, V3 c ) {		return M3{a,b,c}; }
	static constexpr M3 row (		T a, T b, T c,
									T e, T f, T g,
									T i, T j, T k ) {			return M3{V3(a,e,i),V3(b,f,j),V3(c,g,k)}; }
	static constexpr M3 ident () {								return row(1,0,0, 0,1,0, 0,0,1); }
	explicit constexpr M3 (M2 m): arr{V3(m.arr[0], 0), V3(m.arr[1], 0), V3(0,0,1)} {}
	
	M2 m2 () const {											return M2::column( arr[0].xy(), arr[1].xy() ); }
	
	M3& operator*= (M3 r);
};
struct M4 {
	V4 arr[4];
	
	explicit M4 () {}
private: //
	explicit constexpr M4 (V4 a, V4 b, V4 c, V4 d): arr{a,b,c,d} {}
public:

	static constexpr M4 row (		V4 a, V4 b, V4 c, V4 d ) {	return M4{V4(a.x,b.x,c.x,d.x),V4(a.y,b.y,c.y,d.y),V4(a.z,b.z,c.z,d.z),V4(a.w,b.w,c.w,d.w)}; }
	static constexpr M4 column (	V4 a, V4 b, V4 c, V4 d ) {	return M4{a,b,c,d}; }
	static constexpr M4 row (		T a, T b, T c, T d,
									T e, T f, T g, T h,
									T i, T j, T k, T l,
									T m, T n, T o, T p ) {		return M4{V4(a,e,i,m),V4(b,f,j,n),V4(c,g,k,o),V4(d,h,l,p)}; }
	static constexpr M4 ident () {								return row(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1); }
	explicit constexpr M4 (M3 m): arr{V4(m.arr[0], 0), V4(m.arr[1], 0), V4(m.arr[2], 0), V4(0,0,0,1)} {}
	
	M2 m2 () const {											return M2::column( arr[0].xy(), arr[1].xy() ); }
	M3 m3 () const {											return M3::column( arr[0].xyz(), arr[1].xyz(), arr[2].xyz() ); }
	
	M4& operator*= (M4 r);
};

static V2 operator* (M2 m, V2 v) {
	V2 ret;
	ret.x = m.arr[0].x * v.x  +m.arr[1].x * v.y;
	ret.y = m.arr[0].y * v.x  +m.arr[1].y * v.y;
	return ret;
}
static M2 operator* (M2 l, M2 r) {
	M2 ret;
	ret.arr[0] = l * r.arr[0];
	ret.arr[1] = l * r.arr[1];
	return ret;
}

static V3 operator* (M3 m, V3 v) {
	V3 ret;
	ret.x = m.arr[0].x * v.x  +m.arr[1].x * v.y  +m.arr[2].x * v.z;
	ret.y = m.arr[0].y * v.x  +m.arr[1].y * v.y  +m.arr[2].y * v.z;
	ret.z = m.arr[0].z * v.x  +m.arr[1].z * v.y  +m.arr[2].z * v.z;
	return ret;
}
static M3 operator* (M3 l, M3 r) {
	M3 ret;
	ret.arr[0] = l * r.arr[0];
	ret.arr[1] = l * r.arr[1];
	ret.arr[2] = l * r.arr[2];
	return ret;
}

static V4 operator* (M4 m, V4 v) {
	V4 ret;
	ret.x = m.arr[0].x * v.x  +m.arr[1].x * v.y  +m.arr[2].x * v.z  +m.arr[3].x * v.w;
	ret.y = m.arr[0].y * v.x  +m.arr[1].y * v.y  +m.arr[2].y * v.z  +m.arr[3].y * v.w;
	ret.z = m.arr[0].z * v.x  +m.arr[1].z * v.y  +m.arr[2].z * v.z  +m.arr[3].z * v.w;
	ret.w = m.arr[0].w * v.x  +m.arr[1].w * v.y  +m.arr[2].w * v.z  +m.arr[3].w * v.w;
	return ret;
}
static M4 operator* (M4 l, M4 r) {
	M4 ret;
	ret.arr[0] = l * r.arr[0];
	ret.arr[1] = l * r.arr[1];
	ret.arr[2] = l * r.arr[2];
	ret.arr[3] = l * r.arr[3];
	return ret;
}
M2& M2::operator*= (M2 r) {
	return *this = *this * r;
}
M3& M3::operator*= (M3 r) {
	return *this = *this * r;
}
M4& M4::operator*= (M4 r) {
	return *this = *this * r;
}

static M2 scale2 (V2 v) {
	return M2::column(	V2(v.x,0),
						V2(0,v.y) );
}
static M2 rotate2_Z (T ang) {
	auto sc = sin_cos(ang);
	return M2::row(	+sc.c,	-sc.s,
					+sc.s,	+sc.c );
}

static M3 scale3 (V3 v) {
	return M3::column(	V3(v.x,0,0),
						V3(0,v.y,0),
						V3(0,0,v.z) );
}
static M3 rotate3_X (T ang) {
	auto sc = sin_cos(ang);
	return M3::row(	1,		0,		0,
					0,		+sc.c,	-sc.s,
					0,		+sc.s,	+sc.c);
}
static M3 rotate3_Y (T ang) {
	auto sc = sin_cos(ang);
	return M3::row(	+sc.c,	0,		+sc.s,
					0,		1,		0,
					-sc.s,	0,		+sc.c);
}
static M3 rotate3_Z (T ang) {
	auto sc = sin_cos(ang);
	return M3::row(	+sc.c,	-sc.s,	0,
					+sc.s,	+sc.c,	0,
					0,		0,		1);
}

static M4 translate4 (V3 v) {
	return M4::column(	V4(1,0,0,0),
						V4(0,1,0,0),
						V4(0,0,1,0),
						V4(v,1) );
}
static M4 scale4 (V4 v) {
	return M4::column(	V4(v.x,0,0,0),
						V4(0,v.y,0,0),
						V4(0,0,v.z,0),
						V4(0,0,0,v.w) );
}
static M4 rotate4_X (T ang) {
	auto sc = sin_cos(ang);
	return M4::row(	1,		0,		0,		0,
					0,		+sc.c,	-sc.s,	0,
					0,		+sc.s,	+sc.c,	0,
					0,		0,		0,		1 );
}
static M4 rotate4_Y (T ang) {
	auto sc = sin_cos(ang);
	return M4::row(	+sc.c,	0,		+sc.s,	0,
					0,		1,		0,		0,
					-sc.s,	0,		+sc.c,	0,
					0,		0,		0,		1 );
}
static M4 rotate4_Z (T ang) {
	auto sc = sin_cos(ang);
	return M4::row(	+sc.c,	-sc.s,	0,		0,
					+sc.s,	+sc.c,	0,		0,
					0,		0,		1,		0,
					0,		0,		0,		1 );
}

#undef T
#undef V2
#undef V3
#undef V4
#undef M2
#undef M3
#undef M4

//
#define cast(T, val)	_cast<T>(val)
#define round(T, val)	_round<T>(val)

template <typename T> static T _round (fv2 v);
template<> s32v2 _round<s32v2> (fv2 v) {
	fv2 tmp = v +fv2(0.5f);
	return s32v2((s32)tmp.x, (s32)tmp.y);
}

template <typename T> static T _cast (s32v2 v);
template<> fv2 _cast<fv2> (s32v2 v) { return fv2((f32)v.x, (f32)v.y); }

//
#if 0
static constexpr fv3 srgb (f32 r, f32 g, f32 b) {
	return fv3(r/255, g/255, b/255);
}
static constexpr fv3 srgb (f32 all) {
	return fv3(all/255);
}
#endif
