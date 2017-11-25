
#define RZ_COMP_GCC				1
#define RZ_COMP_LLVM			2
#define RZ_COMP_MSVC			3
// Determining the compiler
#if !defined RZ_COMP
	#if _MSC_VER && !__INTELRZ_COMPILER && !__clang__
		#define RZ_COMP RZ_COMP_MSVC
	#elif __GNUC__ && !__clang__
		#define RZ_COMP RZ_COMP_GCC
	#elif __clang__
		#define RZ_COMP RZ_COMP_LLVM
	#else
		#warning Cannot determine compiler!.
	#endif
#endif

#define RZ_ARCH_X64				1
#define RZ_ARCH_ARM_CORTEX_M4	2
#define RZ_ARCH_ARM_V6_HF		3

#define RZ_PLATF_GENERIC_WIN	1
#define RZ_PLATF_GENERIC_UNIX	2
#define RZ_PLATF_NONE			3

#undef FORCEINLINE

#if RZ_COMP == RZ_COMP_MSVC
	#define FORCEINLINE						__forceinline
	#define NOINLINE						__declspec(noinline)
	#define DBGBREAK						__debugbreak()
	
	#define F32_INF							((float)(1e+300 * 1e+300))
	#define F64_INF							(1e+300 * 1e+300)
	#define F32_QNAN						std::numeric_limits<float>::quiet_NaN()
	#define F64_QNAN						std::numeric_limits<double>::quiet_NaN()
	
#elif RZ_COMP == RZ_COMP_LLVM
	#define FORCEINLINE						__attribute__((always_inline)) inline
	#define NOINLINE						__attribute__((noinline))
	#define DBGBREAK						do { asm volatile ("int3"); } while(0)
		
	#define F32_INF							(__builtin_inff())
	#define F64_INF							(__builtin_inf())
	#define F32_QNAN						((float)__builtin_nan("0"))
	#define F64_QNAN						(__builtin_nan("0"))
	
#elif RZ_COMP == RZ_COMP_GCC
	#define FORCEINLINE						__attribute__((always_inline)) inline
	#define NOINLINE						__attribute__((noinline))
	
	#if RZ_PLATF == RZ_PLATF_GENERIC_WIN
		#define DBGBREAK					do { __debugbreak(); } while(0)
	#elif RZ_PLATF == RZ_PLATF_GENERIC_UNIX
		#if RZ_ARCH == RZ_ARCH_ARM_V6_HF
			#define DBGBREAK				do { asm volatile ("bkpt #0"); } while(0)
		#endif
	#endif
	
	#define F32_INF							(__builtin_inff())
	#define F64_INF							(__builtin_inf())
	#define F32_QNAN						((float)__builtin_nan("0"))
	#define F64_QNAN						(__builtin_nan("0"))
	
#endif

////
#define STRINGIFY(x) #x
#define TO_STRING(x) STRINGIFY(x)

#define STATIC_ASSERT(cond) static_assert((cond), STRINGIFY(cond))

#include "types.hpp"

#define ANSI_COLOUR_CODE_RED	"\033[1;31m"
#define ANSI_COLOUR_CODE_YELLOW	"\033[1;33m"
#define ANSI_COLOUR_CODE_NC		"\033[0m"

#include <cstdarg>

#include "assert.hpp"

//
template<typename CAST_T, typename T>
static constexpr bool _safe_cast (T x);

template<> constexpr bool _safe_cast<u32, u64> (u64 x) { return x <= 0xffffffffull; }
template<> constexpr bool _safe_cast<s32, u64> (u64 x) { return x <= 0x7fffffffull; }

#define safe_cast(cast_t, val) _safe_cast<cast_t>(val)

//
#define ARRLEN(arr) (sizeof(arr) / sizeof(arr[0]))

#define EVEN(i)			(((i) % 2) == 0)
#define ODD(i)			(((i) % 2) == 1)

#define BOOL_XOR(a, b)	(((a) != 0) == ((b) != 0))

static u32 strlen (utf32 const* str) {
	u32 ret = 0;
	while (*str++) ++ret;
	return ret;
}
template <u32 N>
static u32 strlen (utf32 const (& str)[N]) {
	STATIC_ASSERT(N >= 1);
	return N -1;
}

template <typename FUNC>
struct At_Scope_Exit {
	FUNC	f;
	void operator= (At_Scope_Exit &) = delete;
	
	FORCEINLINE At_Scope_Exit (FUNC f): f(f) {}
	FORCEINLINE ~At_Scope_Exit () { f(); }
};

struct _Defer_Helper {};

template<typename FUNC>
static FORCEINLINE At_Scope_Exit<FUNC> operator+(_Defer_Helper, FUNC f) {
	return At_Scope_Exit<FUNC>(f);
}

#define CONCAT(a,b) a##b

#define _defer(counter) auto CONCAT(_defer_helper, counter) = _Defer_Helper() +[&] () 
#define defer _defer(__COUNTER__)
// use like: defer { lambda code };

#undef DEFINE_ENUM_FLAG_OPS
#define DEFINE_ENUM_FLAG_OPS(TYPE, UNDERLYING_TYPE) \
	static FORCEINLINE TYPE& operator|= (TYPE& l, TYPE r) { \
		return l = (TYPE)((UNDERLYING_TYPE)l | (UNDERLYING_TYPE)r); \
	} \
	static FORCEINLINE TYPE& operator&= (TYPE& l, TYPE r) { \
		return l = (TYPE)((UNDERLYING_TYPE)l & (UNDERLYING_TYPE)r); \
	} \
	static FORCEINLINE TYPE operator| (TYPE l, TYPE r) { \
		return (TYPE)((UNDERLYING_TYPE)l | (UNDERLYING_TYPE)r); \
	} \
	static FORCEINLINE TYPE operator& (TYPE l, TYPE r) { \
		return (TYPE)((UNDERLYING_TYPE)l & (UNDERLYING_TYPE)r); \
	} \
	static FORCEINLINE TYPE operator~ (TYPE e) { \
		return (TYPE)(~(UNDERLYING_TYPE)e); \
	}

#define DEFINE_ENUM_ITER_OPS(TYPE, UNDERLYING_TYPE) \
	static FORCEINLINE TYPE& operator++ (TYPE& val) { \
		return val = (TYPE)((UNDERLYING_TYPE)val +1); \
	}

template<typename T, typename FUNC>
static T* lsearch (std::vector<T>& arr, FUNC comp_with) {
	for (T& x : arr) {
		if (comp_with(&x)) return &x; // found
	}
	return nullptr; // not found
}

template <typename T> static typename std::vector<T>::iterator vector_append (std::vector<T>* vec) {
	uptr old_len = vec->size();
	vec->resize( old_len +1 );
	return vec->begin() +old_len;
	
}
template <typename T> static typename std::vector<T>::iterator vector_append (std::vector<T>* vec, uptr n) {
	uptr old_len = vec->size();
	vec->resize( old_len +n );
	return vec->begin() +old_len;
}

template <typename T> static uptr vector_size_bytes (std::vector<T> cr vec) {
	return vec.size() * sizeof(T);
}

// Printf that outputs to a std::string
static void _prints (std::string* s, cstr format, va_list vl) { // print 
	for (;;) {
		auto ret = vsnprintf(&(*s)[0], s->length()+1, format, vl); // i think i'm technically not allowed to overwrite the null terminator
		dbg_assert(ret >= 0);
		bool was_big_enough = (u32)ret < s->length()+1;
		s->resize((u32)ret);
		if (was_big_enough) break;
		// buffer was to small, buffer size was increased
		// now snprintf has to succeed, so call it again
	}
}
static void prints (std::string* s, cstr format, ...) {
	va_list vl;
	va_start(vl, format);
	
	_prints(s, format, vl);
	
	va_end(vl);
}
static std::string prints (cstr format, ...) {
	va_list vl;
	va_start(vl, format);
	
	std::string ret;
	ret.reserve(128); // overallocate to prevent calling printf twice in most cases
	ret.resize(ret.capacity());
	_prints(&ret, format, vl);
	
	va_end(vl);
	
	return ret;
}

static u64 get_file_size (FILE* f) {
	fseek(f, 0, SEEK_END);
	u64 file_size = ftell(f); // only 32 support for now
	rewind(f);
	return file_size;
}

// reads entire file into already allocated buffer
static bool read_entire_file (cstr filename, void* buf, u64 expected_file_size) {
	FILE* f = fopen(filename, "rb"); // read binary
	if (!f) return false; // fail
	
	defer { fclose(f); };
	
	u64 file_size = get_file_size(f);
	if (file_size != expected_file_size) return false; // fail
	
	auto ret = fread(buf, 1,file_size, f);
	dbg_assert(ret == file_size);
	
	return true;
}
// reads text file into a std::string by overwriting it's previous contents
static bool read_text_file (cstr filename, std::string* out) {
	FILE* f = fopen(filename, "rb"); // read binary because i don't want to convert "\r\n" to "\n"
	if (!f) return false; // fail
	
	defer { fclose(f); };
	
	u64 file_size = get_file_size(f);
	
	out->resize(file_size);
	
	auto ret = fread(&(*out)[0], 1,file_size, f);
	dbg_assert(ret == file_size);
	
	return true;
}

// overwrites or creates a file with buf
static bool overwrite_file (cstr filename, void const* buf, u64 write_size) {
	FILE* f = fopen(filename, "wb"); // write binary (overwrite file if exists / create if not exists)
	if (!f) return false; // fail
	
	defer { fclose(f); };
	
	auto ret = fwrite(buf, 1,write_size, f);
	dbg_assert(ret == write_size);
	
	return true;
}
