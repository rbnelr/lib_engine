
#if RZ_DBG
////

#include <cstdio>
#include <cstdarg>

static void _failed_dbg_assert (cstr cond) {
	fprintf(stderr,	ANSI_COLOUR_CODE_RED
					"dbg_assert failed!\n"
					"  '%s'\n" ANSI_COLOUR_CODE_NC, cond);
	
	BREAK_IF_DEBUGGING_ELSE_STALL;
}
static void _failed_dbg_assert (cstr cond, cstr msg_format, ...) {
	
	fprintf(stderr,	ANSI_COLOUR_CODE_RED
					"dbg_assert failed!\n"
					"  '%s'\n"
					"  '", cond);
	
	va_list vl;
	va_start(vl, msg_format);
	
	vfprintf(stderr, msg_format, vl);
	
	va_end(vl);
	
	fprintf(stderr,	"'\n" ANSI_COLOUR_CODE_NC);
	
	BREAK_IF_DEBUGGING_ELSE_STALL;
}

#define dbg_assert(cond, ...)	if (!(cond)) _failed_dbg_assert(STRINGIFY(cond), __VA_ARGS__);
#else
#define dbg_assert(cond, ...)	do { (void)cond; } while (0)

////
#endif
