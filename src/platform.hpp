
static struct Input {
	v2 mouse_look_diff = 0;
	iv3 cam_dir = 0;
} inp;

//
static GLFWwindow*	wnd;

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

static void glfw_cursor_move_relative (GLFWwindow* window, double dx, double dy) {
	v2 diff = v2((f32)dx,(f32)dy);
	inp.mouse_look_diff += diff;
}
static void glfw_key_event (GLFWwindow* window, int key, int scancode, int action, int mods) {
	bool down = action == GLFW_PRESS;
	bool up = action == GLFW_RELEASE;
	
	switch (key) {
		
		case GLFW_KEY_A:			if (down)		inp.cam_dir.x -= 1;
									else if (up)	inp.cam_dir.x += 1;
			break;
		case GLFW_KEY_D:			if (down)		inp.cam_dir.x += 1;
									else if (up)	inp.cam_dir.x -= 1;
			break;
		
		case GLFW_KEY_S:			if (down)		inp.cam_dir.z += 1;
									else if (up)	inp.cam_dir.z -= 1;
			break;
		case GLFW_KEY_W:			if (down)		inp.cam_dir.z -= 1;
									else if (up)	inp.cam_dir.z += 1;
			break;
		
		case GLFW_KEY_LEFT_CONTROL:	if (down)		inp.cam_dir.y -= 1;
									else if (up)	inp.cam_dir.y += 1;
			break;
		case GLFW_KEY_SPACE:		if (down)		inp.cam_dir.y += 1;
									else if (up)	inp.cam_dir.y -= 1;
			break;
		
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

static void platform_setup_context () {
	
	glfwSetErrorCallback(glfw_error_proc);
	
	dbg_assert( glfwInit() );
	
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,	3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,	1);
	//glfwWindowHint(GLFW_OPENGL_PROFILE,			GLFW_OPENGL_CORE_PROFILE);
	
	wnd = glfwCreateWindow(1280, 720, "GLFW test", NULL, NULL);
	dbg_assert(wnd);
	
	glfwSetCursorPosRelativeCallback(wnd,	glfw_cursor_move_relative);
	glfwSetKeyCallback(wnd,					glfw_key_event);
	glfwSetMouseButtonCallback(wnd,			glfw_mouse_button_event);
	
	glfwMakeContextCurrent(wnd);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	
}

static void set_vsync (int mode) {
	glfwSwapInterval(mode);
}

