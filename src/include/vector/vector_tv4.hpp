
union V4 {
	struct {
		T	x, y, z, w;
	};
	T		arr[4];
	
	T& operator[] (u32 i) {					return arr[i]; }
	constexpr T operator[] (u32 i) const {	return arr[i]; }
	
	V4 () {}
	explicit constexpr V4 (T val):		x{val},	y{val},	z{val},	w{val} {}
	constexpr V4 (T x, T y, T z, T w):	x{x},	y{y},	z{z},	w{w} {}
	constexpr V4 (V2 v, T z, T w):		x{v.x},	y{v.y},	z{z},	w{w} {}
	constexpr V4 (V3 v, T w):			x{v.x},	y{v.y},	z{v.z},	w{w} {}
	
	constexpr V2 xy () const { return V2(x,y) ;};
	constexpr V3 xyz () const { return V3(x,y,z) ;};
	
	V4& operator+= (V4 r) {
		return *this = V4(x +r.x, y +r.y, z +r.z, w +r.w);
	}
	V4& operator-= (V4 r) {
		return *this = V4(x -r.x, y -r.y, z -r.z, w -r.w);
	}
	V4& operator*= (V4 r) {
		return *this = V4(x * r.x, y * r.y, z * r.z, w * r.w);
	}
	V4& operator/= (V4 r) {
		return *this = V4(x / r.x, y / r.y, z / r.z, w / r.w);
	}
};

static constexpr V4 operator+ (V4 l, V4 r) {
	return V4(l.x +r.x, l.y +r.y, l.z +r.z, l.w +r.w);
}
static constexpr V4 operator- (V4 l, V4 r) {
	return V4(l.x -r.x, l.y -r.y, l.z -r.z, l.w -r.w);
}
static constexpr V4 operator* (V4 l, V4 r) {
	return V4(l.x * r.x, l.y * r.y, l.z * r.z, l.w	* r.w);
}
static constexpr V4 operator/ (V4 l, V4 r) {
	return V4(l.x / r.x, l.y / r.y, l.z / r.z, l.w / r.w);
}

static constexpr V4 lerp (V4 a, V4 b, T t) {
	return (a * V4(T(1) -t)) +(b * V4(t));
}
static constexpr V4 lerp (V4 a, V4 b, V4 t) {
	return (a * (V4(1) -t)) +(b * t);
}

static constexpr T dot (V4 l, V4 r) {
	return l.x*r.x +l.y*r.y +l.z*r.z +l.w*r.w;
}

T length (V4 v) {
	return sqrt(v.x*v.x +v.y*v.y +v.z*v.z +v.w*v.w);
}
V4 normalize (V4 v) {
	return v / V4(length(v));
}

#if 1
static constexpr V4 MIN (V4 l, V4 r) { return V4( MIN(l.x, r.x), MIN(l.y, r.y), MIN(l.z, r.z), MIN(l.w, r.w) ); }
static constexpr V4 MAX (V4 l, V4 r) { return V4( MAX(l.x, r.x), MAX(l.y, r.y), MAX(l.z, r.z), MAX(l.w, r.w) ); }
#endif

static constexpr V4 clamp (V4 val, V4 l, V4 h) { return MIN( MAX(val,l), h ); }
