
union V3 {
	struct {
		T	x, y, z;
	};
	T		arr[3];
	
	T& operator[] (u32 i) {					return arr[i]; }
	constexpr T operator[] (u32 i) const {	return arr[i]; }
	
	V3 () {}
	explicit constexpr V3 (T val):		x{val},	y{val},	z{val} {}
	constexpr V3 (T x, T y, T z):		x{x},	y{y},	z{z} {}
	constexpr V3 (V2 v, T z):			x{v.x},	y{v.y},	z{z} {}
	
	constexpr V2 xy () const { return V2(x,y) ;};
	
	V3& operator+= (V3 r) {
		return *this = V3(x +r.x, y +r.y, z +r.z);
	}
	V3& operator-= (V3 r) {
		return *this = V3(x -r.x, y -r.y, z -r.z);
	}
	V3& operator*= (V3 r) {
		return *this = V3(x * r.x, y * r.y, z * r.z);
	}
	V3& operator/= (V3 r) {
		return *this = V3(x / r.x, y / r.y, z / r.z);
	}
};

static constexpr V3 operator+ (V3 l, V3 r) {
	return V3(l.x +r.x, l.y +r.y, l.z +r.z);
}
static constexpr V3 operator- (V3 l, V3 r) {
	return V3(l.x -r.x, l.y -r.y, l.z -r.z);
}
static constexpr V3 operator* (V3 l, V3 r) {
	return V3(l.x * r.x, l.y * r.y, l.z * r.z);
}
static constexpr V3 operator/ (V3 l, V3 r) {
	return V3(l.x / r.x, l.y / r.y, l.z / r.z);
}

static constexpr V3 lerp (V3 a, V3 b, T t) {
	return (a * V3(T(1) -t)) +(b * V3(t));
}
static constexpr V3 lerp (V3 a, V3 b, V3 t) {
	return (a * (V3(1) -t)) +(b * t);
}

static constexpr T dot (V3 l, V3 r) {
	return l.x*r.x +l.y*r.y +l.z*r.z;
}
static constexpr V3 cross (V3 l, V3 r) {
	return V3(	l.y*r.z -l.z*r.y,
				l.z*r.x -l.x*r.z,
				l.x*r.y -l.y*r.x );
}

T length (V3 v) {
	return sqrt(v.x*v.x +v.y*v.y +v.z*v.z);
}
V3 normalize (V3 v) {
	return v / V3(length(v));
}

#if 1
static constexpr V3 MIN (V3 l, V3 r) { return V3( MIN(l.x, r.x), MIN(l.y, r.y), MIN(l.z, r.z) ); }
static constexpr V3 MAX (V3 l, V3 r) { return V3( MAX(l.x, r.x), MAX(l.y, r.y), MAX(l.z, r.z) ); }
#endif

static constexpr V3 clamp (V3 val, V3 l, V3 h) { return MIN( MAX(val,l), h ); }
