
static GLFWwindow*	wnd;

static void glfw_error_proc(int err, const char* msg) {
	fprintf(stderr, ANSI_COLOUR_CODE_RED "GLFW Error! 0x%x '%s'\n" ANSI_COLOUR_CODE_NC, err, msg);
}

static void cursor_position_callback (GLFWwindow* wnd, double posx, double posy) {
	
}

static void setup_graphics_context () {
	
	glfwSetErrorCallback(glfw_error_proc);
	
	dbg_assert( glfwInit() );
	
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,	3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,	1);
	//glfwWindowHint(GLFW_OPENGL_PROFILE,			GLFW_OPENGL_CORE_PROFILE);
	
	wnd = glfwCreateWindow(1280, 720, "GLFW test", NULL, NULL);
	dbg_assert(wnd);
	
//	glfwSetCursorPosCallback(wnd, );
	
	glfwMakeContextCurrent(wnd);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	
}

static void set_vsync (int mode) {
	glfwSwapInterval(mode);
}

