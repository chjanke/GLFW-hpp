#include <GLFW/glfw3.h>

namespace glfw {


	inline constexpr int DONT_CARE = GLFW_DONT_CARE;
	inline constexpr int FALSE = GLFW_FALSE;
	inline constexpr int TRUE = GLFW_TRUE;

	inline void set_swap_interval(int swapInterval) {
		glfwSwapInterval(swapInterval);
	}

	inline void poll_events() {
		glfwPollEvents();
	}

	inline void wait_events() {
		glfwWaitEvents();
	}

	inline void wait_events(double timeout) {
		glfwWaitEventsTimeout(timeout);
	}

	inline void post_empty_event() {
		glfwPostEmptyEvent();
	}
}