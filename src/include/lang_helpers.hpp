
#define RZ_COMP_GCC			1
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

#define RZ_ARCH_X64			1
#define RZ_ARCH_ARM_CORTEX_M4	2
#define RZ_ARCH_ARM_V6_HF		3

#define RZ_PLATF_GENERIC_WIN	1
#define RZ_PLATF_GENERIC_UNIX	2
#define RZ_PLATF_NONE			3

#undef FORCEINLINE

#if RZ_COMP == RZ_COMP_MSVC
	#define FORCEINLINE						__forceinline
	#define NOINLINE						__declspec(noinline)
	#define BUILTIN_F32_INF					((float)(1e+300 * 1e+300))
	#define BUILTIN_F64_INF					(1e+300 * 1e+300)
	#define BUILTIN_F32_QNAN				__builtin_nanf("0")
	#define BUILTIN_F64_QNAN				__builtin_nan("0")
	#define DBGBREAK						__debugbreak()
#elif RZ_COMP == RZ_COMP_LLVM
	#define FORCEINLINE						__attribute__((always_inline)) inline
	#define NOINLINE						__attribute__((noinline))
	#define BUILTIN_F32_INF					(__builtin_inff())
	#define BUILTIN_F64_INF					(__builtin_inf())
	#define BUILTIN_F32_QNAN				__builtin_nan("0")
	#define BUILTIN_F64_QNAN				__builtin_nan("0")
	#define DBGBREAK						do { asm volatile ("int3"); } while(0)
#elif RZ_COMP == RZ_COMP_GCC
	#define FORCEINLINE						__attribute__((always_inline)) inline
	#define NOINLINE						__attribute__((noinline))
	#define BUILTIN_F32_INF					(__builtin_inff())
	#define BUILTIN_F64_INF					(__builtin_inf())
	#define BUILTIN_F32_QNAN				__builtin_nan("0")
	#define BUILTIN_F64_QNAN				__builtin_nan("0")
	
	#if RZ_PLATF == RZ_PLATF_GENERIC_WIN
		#define DBGBREAK					do { __debugbreak(); } while(0)
	#elif RZ_PLATF == RZ_PLATF_GENERIC_UNIX
		#if RZ_ARCH == RZ_ARCH_ARM_V6_HF
			#define DBGBREAK				do { asm volatile ("bkpt #0"); } while(0)
		#endif
	#endif
#endif

#if RZ_PLATF == RZ_PLATF_GENERIC_WIN
	
	#if RZ_DBG // try to not use windows lib directly to simplify porting, but still allow for debugging (Sleep() for ex.)
		
		#define DWORD unsigned long
		#define BOOL int
		
		// For debugging
		__declspec(dllimport) BOOL __stdcall IsDebuggerPresent(void);
		
		__declspec(dllimport) void __stdcall Sleep(
		  DWORD dwMilliseconds
		);
		
		#define IS_DEBUGGER_PRESENT				IsDebuggerPresent()
		#define DBGBREAK_IF_DEBUGGER_PRESENT	if (IS_DEBUGGER_PRESENT) { DBGBREAK; }
		#define BREAK_IF_DEBUGGING_ELSE_STALL	if (IS_DEBUGGER_PRESENT) { DBGBREAK; } else { Sleep(100); }
		
		static void dbg_sleep (f32 sec) {
			Sleep( (DWORD)(sec * 1000.0f) );
		}
		
		#undef BOOL
		#undef DWORD
		
	#endif
	
#endif

////
#define STRINGIFY(x) #x
#define TO_STRING(x) STRINGIFY(x)

#define STATIC_ASSERT(cond) static_assert((cond), STRINGIFY(cond))

#define ANSI_COLOUR_CODE_RED	"\033[1;31m"
#define ANSI_COLOUR_CODE_NC		"\033[0m"


template <typename T, typename AT, uptr N>
static constexpr T arrlenof (AT (& arr)[N]) {
	//static_assert(); // TODO: implement proper check
	return (T)N;
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

#define defer auto _defer_helper##__COUNTER__ = _Defer_Helper() +[&] () 
// use like: defer { lambda code };
