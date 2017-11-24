
static struct Input {
	v2 mouse_look_diff = 0;
	iv3 cam_dir = 0;
} inp;

//
static GLFWwindow*	wnd;
static void toggle_fullscreen ();

static void start_mouse_look () {
	glfwSetInputMode(wnd, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}
static void stop_mouse_look () {
	glfwSetInputMode(wnd, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

//
static void glfw_cursor_move_relative (GLFWwindow* window, double dx, double dy) {
	v2 diff = v2((f32)dx,(f32)dy);
	inp.mouse_look_diff += diff;
}
static void glfw_key_event (GLFWwindow* window, int key, int scancode, int action, int mods) {
	dbg_assert(action == GLFW_PRESS || action == GLFW_RELEASE || action == GLFW_REPEAT);
	
	bool went_down = action == GLFW_PRESS;
	bool went_up = action == GLFW_RELEASE;
	
	bool repeated = !went_down && !went_up; // GLFW_REPEAT
	
	bool alt = (mods & GLFW_MOD_ALT) != 0;
	
	if (repeated) {
		
	} else {
		switch (key) {
			
			case GLFW_KEY_F11:			if (went_down) {		toggle_fullscreen(); }	break;
			case GLFW_KEY_ENTER:		if (went_down && alt) {	toggle_fullscreen(); }	break;
			
			case GLFW_KEY_A:			inp.cam_dir.x -= went_down ? +1 : -1;		break;
			case GLFW_KEY_D:			inp.cam_dir.x += went_down ? +1 : -1;		break;
			
			case GLFW_KEY_S:			inp.cam_dir.z += went_down ? +1 : -1;		break;
			case GLFW_KEY_W:			inp.cam_dir.z -= went_down ? +1 : -1;		break;
			
			case GLFW_KEY_LEFT_CONTROL:	inp.cam_dir.y -= went_down ? +1 : -1;		break;
			case GLFW_KEY_SPACE:		inp.cam_dir.y += went_down ? +1 : -1;		break;
			
		}
	}
}
static void glfw_mouse_button_event (GLFWwindow* window, int button, int action, int mods) {
    switch (button) {
		case GLFW_MOUSE_BUTTON_RIGHT:
			if (action == GLFW_PRESS) {
				start_mouse_look();
			} else {
				stop_mouse_look();
			}
			break;
	}
}

static void glfw_error_proc(int err, const char* msg) {
	fprintf(stderr, ANSI_COLOUR_CODE_RED "GLFW Error! 0x%x '%s'\n" ANSI_COLOUR_CODE_NC, err, msg);
}

static void set_vsync (int mode) {
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
}

#define GL_VER_MAJOR 3
#define GL_VER_MINOR 3

#define GL_VAOS_REQUIRED	GL_VER_MAJOR >= 3 && GL_VER_MINOR >= 2

#if GL_VAOS_REQUIRED
GLuint	g_vao;
#endif

static void platform_setup_context_and_open_window () {
	
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
			window_positioning.dim = iv2(1280, 720);
		}
		
		glfwWindowHint(GLFW_VISIBLE, 0);
		
		wnd = glfwCreateWindow(window_positioning.dim.x,window_positioning.dim.y, "GLFW test", NULL, NULL);
		dbg_assert(wnd);
		
		if (pos_restored) {
			// window_positioning restored
			printf("window_positioning restored from \"" WINDOW_POSITIONING_FILE "\".\n");
		} else {
			get_window_positioning();
			printf("window_positioning could not be restored from \"" WINDOW_POSITIONING_FILE "\", using default.\n");
		}
		
		position_window();
		
		glfwShowWindow(wnd);
	}
	
	glfwSetCursorPosRelativeCallback(wnd,	glfw_cursor_move_relative);
	glfwSetKeyCallback(wnd,					glfw_key_event);
	glfwSetMouseButtonCallback(wnd,			glfw_mouse_button_event);
	
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
