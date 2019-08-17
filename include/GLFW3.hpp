#include <GLFW/glfw3.h>

#include <stdexcept>

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
	
		/************************************************************************************
	 *																					*
	 *									 LIBRARY INIT									*
	 *																					*
	 ************************************************************************************/

	enum class init_hint_type : int {
		JOYSTICK_HAT_BUTTONS = GLFW_JOYSTICK_HAT_BUTTONS,
		COCOA_CHDIR_RESOURCES = GLFW_COCOA_CHDIR_RESOURCES,
		COCOA_MENUBAR = GLFW_COCOA_MENUBAR,
	};

	struct init_hint {
		init_hint_type hint;
		bool hintValue;
	};

	class glfw_lib {
		glfw_lib() {
			if (!glfwInit()) throw std::runtime_error("Failed to init GLFW");
		}
		~glfw_lib() {
			glfwTerminate();
		}

	public:
		template<class ...hints>
		static void init_hints(hints... initHints) {
			static_assert((std::is_same_v<init_hint, hints> && ...));
			(glfwInitHint(static_cast<int>(initHints.hint), initHints.hintValue ? TRUE : FALSE), ...);
		}
		static void init() {
			static glfw_lib libInstance;
		}
	};
}