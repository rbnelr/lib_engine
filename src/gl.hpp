
template <typename T>
struct array {
	T*		arr;
	uptr	len;
	
	template <uptr N>
	array (T const (& arr)[N]): arr{arr}, len{N} {}
	array (std::vector<T> cr vec): arr{vec.begin()}, len{vec.size()} {}
	
	uptr bytes_size () const { return len * sizeof(T); }
	
	T const&	operator[] (uptr indx) const {
		dbg_assert(indx < len, "Index overflow in array");
		return arr[indx];
	}
	T&			operator[] (uptr indx) {
		dbg_assert(indx < len, "Index overflow in array");
		return arr[indx];
	}
};

struct Vertex {
	v3	pos_world;
	v3	col;
};

struct Vbo {
	GLuint	vbo;
	
	void gen () {
		glGenBuffers(1, &vbo);
	}
	
	void upload (array<Vertex const> cr data) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, data.bytes_size(), data.arr, GL_STATIC_DRAW);
	}
	
	void bind (GLuint shad) {
		
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		
		GLint pos_world =	glGetAttribLocation(shad, "pos_world");
		GLint col =			glGetAttribLocation(shad, "col");
		
		glEnableVertexAttribArray(pos_world);
		glVertexAttribPointer(pos_world,	3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos_world));
		
		glEnableVertexAttribArray(col);
		glVertexAttribPointer(col,			3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, col));
	}
};
