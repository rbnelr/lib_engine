
//
static GLFWwindow*	wnd;
static void toggle_fullscreen ();

static void glfw_key_event (GLFWwindow* window, int key, int scancode, int action, int mods);
static void glfw_mouse_button_event (GLFWwindow* window, int button, int action, int mods);
static void glfw_mouse_scroll (GLFWwindow* window, double xoffset, double yoffset);
static void glfw_cursor_move_relative (GLFWwindow* window, double dx, double dy);

static void start_mouse_look () {
	glfwSetInputMode(wnd, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}
static void stop_mouse_look () {
	glfwSetInputMode(wnd, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

//

static void glfw_error_proc(int err, const char* msg) {
	fprintf(stderr, ANSI_COLOUR_CODE_RED "GLFW Error! 0x%x '%s'\n" ANSI_COLOUR_CODE_NC, err, msg);
}

int vsync_mode = 1;
static void set_vsync (int mode) {
	vsync_mode = mode;
	glfwSwapInterval(mode);
}

struct Rect {
	iv2 pos;
	iv2 dim;
};

static bool			fullscreen = false;

static GLFWmonitor*	primary_monitor;
static Rect			window_positioning;

#define WINDOW_POSITIONING_FILE	"saves/window_positioning.bin"

static void get_window_positioning () {
	auto& r = window_positioning;
	glfwGetWindowPos(wnd, &r.pos.x,&r.pos.y);
	glfwGetWindowSize(wnd, &r.dim.x,&r.dim.y);
}
static void position_window () {
	auto& r = window_positioning;
	
	if (fullscreen) {
		auto* vm = glfwGetVideoMode(primary_monitor);
		glfwSetWindowMonitor(wnd, primary_monitor, 0,0, vm->width,vm->height, vm->refreshRate);
		
	} else {
		glfwSetWindowMonitor(wnd, NULL, r.pos.x,r.pos.y, r.dim.x,r.dim.y, GLFW_DONT_CARE);
	}
	
}
static void toggle_fullscreen () {
	if (!fullscreen) {
		get_window_positioning();
	}
	fullscreen = !fullscreen;
	
	position_window();
	
	set_vsync(vsync_mode);
}

#define GL_VER_MAJOR 3
#define GL_VER_MINOR 3

#define GL_VAOS_REQUIRED	GL_VER_MAJOR >= 3 && GL_VER_MINOR >= 2

#if GL_VAOS_REQUIRED
GLuint	g_vao;
#endif

static void platform_setup_context_and_open_window (cstr inital_wnd_title, iv2 default_wnd_dim) {
	
	glfwSetErrorCallback(glfw_error_proc);
	
	dbg_assert( glfwInit() );
	
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,	GL_VER_MAJOR);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,	GL_VER_MINOR);
	if (GL_VER_MAJOR >= 3 && GL_VER_MINOR >= 2) {
		glfwWindowHint(GLFW_OPENGL_PROFILE,		GLFW_OPENGL_CORE_PROFILE);
	}
	
	{ // open and postion window
		primary_monitor = glfwGetPrimaryMonitor();
		
		bool pos_restored = read_entire_file(WINDOW_POSITIONING_FILE, &window_positioning, sizeof(window_positioning));
		if (!pos_restored) {
			window_positioning.dim = default_wnd_dim;
		}
		
		glfwWindowHint(GLFW_VISIBLE, 0);
		
		wnd = glfwCreateWindow(window_positioning.dim.x,window_positioning.dim.y, inital_wnd_title, NULL, NULL);
		dbg_assert(wnd);
		
		if (pos_restored) {
			printf("window_positioning restored from \"" WINDOW_POSITIONING_FILE "\".\n");
		} else {
			get_window_positioning();
			printf("window_positioning could not be restored from \"" WINDOW_POSITIONING_FILE "\", using default.\n");
		}
		
		position_window();
		
		glfwShowWindow(wnd);
	}
	
	glfwSetKeyCallback(wnd,					glfw_key_event);
	glfwSetMouseButtonCallback(wnd,			glfw_mouse_button_event);
	glfwSetScrollCallback(wnd,				glfw_mouse_scroll);
	glfwSetCursorPosRelativeCallback(wnd,	glfw_cursor_move_relative);
	
	glfwMakeContextCurrent(wnd);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	
	if (GL_VAOS_REQUIRED) {
		glGenVertexArrays(1, &g_vao);
		glBindVertexArray(g_vao);
	}
	
}
static void platform_terminate () {
	
	{
		if (!fullscreen) {
			get_window_positioning();
		}
		
		bool pos_saved = overwrite_file(WINDOW_POSITIONING_FILE, &window_positioning, sizeof(window_positioning));
		if (pos_saved) {
			printf("window_positioning saved to \"" WINDOW_POSITIONING_FILE "\".\n");
		} else {
			log_warning("could not write \"" WINDOW_POSITIONING_FILE "\", window_positioning won't be restored on next launch.");
		}
	}
	
	glfwDestroyWindow(wnd);
	glfwTerminate();
}
