#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <string_view>
#include <type_traits>

namespace glfw {

	using icon_image = GLFWimage;
	using gamma_ramp = GLFWgammaramp;
	
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
	
	
	namespace detail {
		struct version_base {
			version_base() = default;
			version_base(int majorV, int minorV, int rev) : major(majorV), minor(minorV), revision(rev) {}
			bool operator==(version_base const& rhs) { return major == rhs.major && minor == rhs.minor && revision == rhs.revision; }
			bool operator!=(version_base const& rhs) { return !(*this == rhs); }
			bool operator<(version_base const& rhs) {
				if (major < rhs.major) return true;
				if (minor < rhs.minor) return true;
				if (revision < rhs.revision) return true;
				return false;
			}
			bool operator>(version_base const& rhs) {
				if (major > rhs.major) return true;
				if (minor > rhs.minor) return true;
				if (revision > rhs.revision) return true;
				return false;
			}
			bool operator<=(version_base const& rhs) {
				if (major <= rhs.major) return true;
				if (minor <= rhs.minor) return true;
				if (revision <= rhs.revision) return true;
				return false;
			}
			bool operator>=(version_base const& rhs) {
				if (major >= rhs.major) return true;
				if (minor >= rhs.minor) return true;
				if (revision >= rhs.revision) return true;
				return false;
			}
			int major, minor, revision;
		};
	}


	struct glfw_version : public detail::version_base {
		glfw_version() {
			glfwGetVersion(&major, &minor, &revision);
		}
		glfw_version(int major, int minor, int revision) : version_base(major, minor, revision) {}
	};
	
	/************************************************************************************
	 *																					*
	 *									 MONITOR										*
	 *																					*
	 ************************************************************************************/

	struct monitor_size {
		int width, height;
	};

	struct monitor_content_scale {
		float xScale, yScale;
	};

	struct monitor_position {
		int x, y;
	};

	struct monitor_work_area {
		int x, y, width, height;
	};

	struct monitor_color_depth {
		int redBits, greenBits, blueBits;
	};

	struct monitor_resolution {
		int width, height;
	};

	struct monitor_refresh_rate {
		int rate;
	};

	struct video_mode {
		monitor_resolution resolution;
		monitor_refresh_rate refresh;
		monitor_color_depth color;
	};

	class monitor {
	public:
		explicit monitor(GLFWmonitor* handle) : m_handle(handle) {};

		static monitor get_primary_monitor() {
			return monitor{ glfwGetPrimaryMonitor() };
		}

		static std::vector<monitor> get_monitors() {
			int count = 0;
			GLFWmonitor** monitorHandles = glfwGetMonitors(&count);
			auto monitors = std::vector<monitor>{};
			monitors.reserve(count);
			for (int i = 0; i < count; ++i) {
				monitors.emplace_back(monitorHandles[i]);
			}
			return monitors;
		}


		video_mode get_current_video_mode() const {
			GLFWvidmode const* curMode = glfwGetVideoMode(m_handle);
			return { {curMode->width, curMode->height},{curMode->refreshRate}, {curMode->redBits, curMode->greenBits, curMode->blueBits} };
		}

		std::vector<video_mode> const get_video_modes() const {
			int count = 0;
			GLFWvidmode const* modes = glfwGetVideoModes(m_handle, &count);
			auto video_modes = std::vector<video_mode>{};
			video_modes.reserve(count);
			for (int i = 0; i < count; ++i) {
				video_modes.push_back(video_mode{ { modes[i].width, modes[i].height }, { modes[i].refreshRate }, { modes[i].redBits, modes[i].greenBits, modes[i].blueBits } });
			}
			return video_modes;
		}

		monitor_size get_physical_size()  const {
			monitor_size size;
			glfwGetMonitorPhysicalSize(m_handle, &size.width, &size.height);
			return size;
		}

		monitor_content_scale get_content_Scale() const {
			monitor_content_scale scale;
			glfwGetMonitorContentScale(m_handle, &scale.xScale, &scale.yScale);
		}

		monitor_position get_virtual_position() const {
			monitor_position pos;
			glfwGetMonitorPos(m_handle, &pos.x, &pos.y);
			return pos;
		}

		monitor_work_area get_work_area() const {
			monitor_work_area workArea;
			glfwGetMonitorWorkarea(m_handle, &workArea.x, &workArea.y, &workArea.width, &workArea.height);
			return workArea;
		}

		std::string_view name() const {
			const char* name = glfwGetMonitorName(m_handle);
			if (name) return std::string_view{ name };
			return std::string_view{};
		}

		template<class T>
		std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<T>>>* get_user_pointer() const {
			return static_cast<T*>(glfwGetMonitorUserPointer(m_handle));
		}

		template<class T>
		T set_user_pointer(T userPointer) {
			static_assert(std::is_pointer_v<T>, "set_user_pointer only accepts pointer types");
			glfwSetMonitorUserPointer(m_handle, static_cast<void*>(userPointer));
		}

		const gamma_ramp get_gamma_ramp() const { return *glfwGetGammaRamp(m_handle); }

		void set_gamma_ramp(gamma_ramp newRamp) { glfwSetGammaRamp(m_handle, &newRamp); }

		void set_gamma(float gamma) { glfwSetGamma(m_handle, gamma); }

		bool operator==(monitor const& rhs) { return m_handle == rhs.m_handle; }
		bool operator!=(monitor const& rhs) { return !(*this == rhs); }

		operator GLFWmonitor* () { return m_handle; }
	private:
		GLFWmonitor* m_handle;
	};
	
}