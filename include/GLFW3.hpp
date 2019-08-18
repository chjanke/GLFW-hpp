#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <string_view>
#include <type_traits>
#include <variant>
#include <optional>
#include <functional>
#include <unordered_map>
#include <cstdint>

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

	/************************************************************************************
	 *																					*
	 *									 WINDOW											*
	 *																					*
	 ************************************************************************************/

	 /* Window Options */
	namespace attributes {

		enum class client_api_type : int {
			OPENGL = GLFW_OPENGL_API,
			OPENGL_ES = GLFW_OPENGL_ES_API,
			NONE = GLFW_NO_API,
		};

		enum class context_creation_api_type : int {
			NATIVE = GLFW_NATIVE_CONTEXT_API,
			EGL = GLFW_EGL_CONTEXT_API,
			OSMESA = GLFW_OSMESA_CONTEXT_API,
		};

		struct context_version : public detail::version_base {
			context_version(int major, int minor, int rev) : version_base(major, minor, rev) {}
		};

		enum class opengl_profile_type : int {
			CORE = GLFW_OPENGL_CORE_PROFILE,
			COMPAT = GLFW_OPENGL_COMPAT_PROFILE,
			ANY = GLFW_OPENGL_ANY_PROFILE,
		};

		enum class context_robustness_type : int {
			LOSE_CONTEXT_ON_RESET = GLFW_LOSE_CONTEXT_ON_RESET,
			NO_RESET_NOTIFICATION = GLFW_NO_RESET_NOTIFICATION,
		};

		enum class context_release_behaviour_type : int {
			ANY_BEHAVIOUR = GLFW_ANY_RELEASE_BEHAVIOR,
			FLUSH = GLFW_RELEASE_BEHAVIOR_FLUSH,
			NONE = GLFW_RELEASE_BEHAVIOR_NONE,
		};


		/* window attribute / hint types */

		enum class hint_type : int { /* List of Hints */
			RESIZABLE = GLFW_RESIZABLE,
			VISIBLE = GLFW_VISIBLE,
			DECORATED = GLFW_DECORATED,
			FOCUSED = GLFW_FOCUSED,
			AUTO_ICONIFY = GLFW_AUTO_ICONIFY,
			FLOATING = GLFW_FLOATING,
			MAXIMIZED = GLFW_MAXIMIZED,
			CENTER_CURSOR = GLFW_CENTER_CURSOR,
			TRANSPARENT_FRAMEBUFFER = GLFW_TRANSPARENT_FRAMEBUFFER,
			FOCUS_ON_SHOW = GLFW_FOCUS_ON_SHOW,
			SCALE_TO_MONITOR = GLFW_SCALE_TO_MONITOR,
			STEREO = GLFW_STEREO,
			SRGB_CAPABLE = GLFW_SRGB_CAPABLE,
			DOUBLEBUFFER = GLFW_DOUBLEBUFFER,
			OPENGL_FORWARD_COMPAT = GLFW_OPENGL_FORWARD_COMPAT,
			OPENGL_DEBUG_CONTEXT = GLFW_OPENGL_DEBUG_CONTEXT,
			COCOA_RETINA_FRAMEBUFFER = GLFW_COCOA_RETINA_FRAMEBUFFER,
			COCOA_GRAPHICS_SWITCHING = GLFW_COCOA_GRAPHICS_SWITCHING,
			CONTEXT_NO_ERROR = GLFW_CONTEXT_NO_ERROR,
		};

		enum class value_hint_type : int {
			RED_BITS = GLFW_RED_BITS,
			GREEN_BITS = GLFW_GREEN_BITS,
			BLUE_BITS = GLFW_BLUE_BITS,
			ALPHA_BITS = GLFW_ALPHA_BITS,
			DEPTH_BITS = GLFW_DEPTH_BITS,
			STENCIL_BITS = GLFW_STENCIL_BITS,
			ACCUM_RED_BITS = GLFW_ACCUM_RED_BITS,
			ACCUM_GREEN_BITS = GLFW_ACCUM_GREEN_BITS,
			ACCUM_BLUE_BITS = GLFW_ACCUM_BLUE_BITS,
			ACCUM_ALPHA_BITS = GLFW_ACCUM_ALPHA_BITS,
			AUX_BUFFERS = GLFW_AUX_BUFFERS,
			SAMPLES = GLFW_SAMPLES,
			REFRESH_RATE = GLFW_REFRESH_RATE,
			CONTEXT_VERSION_MAJOR = GLFW_CONTEXT_VERSION_MAJOR,
			CONTEXT_VERSION_MINOR = GLFW_CONTEXT_VERSION_MINOR,
			CONTEXT_REVISION = GLFW_CONTEXT_REVISION,

		};

		enum class string_hint_type : int {
			COCOA_FRAME_NAME = GLFW_COCOA_FRAME_NAME,
			X11_CLASS_NAME = GLFW_X11_CLASS_NAME,
			X11_INSTANCE_NAME = GLFW_X11_INSTANCE_NAME,
		};

		/* window hints */

		struct string_hint {
			string_hint_type hint;
			std::string value;
		};

		struct hint {
			hint_type hint;
			bool value;
		};

		struct value_hint {
			value_hint_type hint;
			int value;
		};

		struct client_api_hint {
			client_api_type api;
		};

		struct context_creation_api_hint {
			context_creation_api_type api;
		};

		struct robustness_hint {
			context_robustness_type robustness;
		};

		struct opengl_profile_hint {
			opengl_profile_type profile;
		};

		struct context_release_behaviour_hint {
			context_release_behaviour_type behaviour;
		};

		using window_hints = std::variant<hint, string_hint, value_hint, client_api_hint, context_creation_api_hint, robustness_hint, opengl_profile_hint, context_release_behaviour_hint>;
	}

	/* window API types */

	struct window_size {
		int width, height;
	};

	struct window_position {
		int x, y;
	};

	struct window_frame {
		int left, top, right, bottom;
	};

	struct framebuffer {
		int width, height;
	};

	struct window_content_scale {
		float xScale, yScale;
	};

	struct window_size_limit {
		int minWidth, minHeight;
		int maxWidth, maxHeight;
	};

	struct aspect_ratio {
		int num, denom;
	};

	enum window_event_type : uint16_t {
		POSITION_CHANGED = 1 << 0,
		SIZE_CHANGED = 1 << 1,
		FRAMEBUFFER_SIZE_CHANGED = 1 << 2,
		CONTENT_SCALE_CHANGED = 1 << 3,
		FOCUS_CHANGED = 1 << 4,
		MINIMIZE_STATE_CHANGED = 1 << 5,
		MAXIMIZE_STATE_CHANGED = 1 << 6,
		CONTENT_NEEDS_REFRESH = 1 << 7,
		CLOSE_REQUESTED = 1 << 8,
	};
	//forward declaration
	namespace window_events {
		template<class WindowCallback>
		void set_event_callback(GLFWwindow*, WindowCallback&&, window_event_type mask);
		void set_event_callback(GLFWwindow*, std::nullptr_t);
	}


	class window {
	public:
		using client_api_type = attributes::client_api_type;
		using context_creation_api_type = attributes::context_creation_api_type;
		using context_version = attributes::context_version;
		using opengl_profile_type = attributes::opengl_profile_type;
		using context_robustness_type = attributes::context_robustness_type;

		window(window_size size, std::string_view title, std::optional<monitor> fullscreenLocation = std::nullopt, window* sharedContext = nullptr) {
			GLFWmonitor* fsLoc = fullscreenLocation ? fullscreenLocation.value() : (GLFWmonitor*)nullptr;
			GLFWwindow* share = sharedContext ? sharedContext->m_handle : nullptr;
			m_handle = glfwCreateWindow(size.width, size.height, title.data(), fsLoc, share);
		}
		//window is a unique handle
		window(window const&) = delete;
		window& operator=(window const&) = delete;
		//window handle is movable
		window(window&& w) noexcept : m_handle(w.m_handle) { w.m_handle = nullptr; }
		window& operator=(window&& w) noexcept {
			m_handle = w.m_handle;
			w.m_handle = nullptr;
		};

		~window() {
			glfwDestroyWindow(m_handle);
		}

		void make_fullscreen(monitor fsTargetMonitor, std::optional<video_mode> videoMode = std::nullopt) {
			if (videoMode.has_value()) {
				//fullscreen with specific video mode
				glfwSetWindowMonitor(m_handle, fsTargetMonitor, glfw::DONT_CARE, glfw::DONT_CARE, videoMode->resolution.width, videoMode->resolution.height, videoMode->refresh.rate);
			}
			else {
				//windowed fullscreen mode
				auto curVideoMode = fsTargetMonitor.get_current_video_mode(); //TODO: handle full screen -> windowed fullscreen, need a location to store the previous video mode
				glfwSetWindowMonitor(m_handle, fsTargetMonitor, glfw::DONT_CARE, glfw::DONT_CARE, curVideoMode.resolution.width, curVideoMode.resolution.height, curVideoMode.refresh.rate);
			}
		}

		void make_windowed_fullscreen(monitor fsTargetMonitor) { make_fullscreen(fsTargetMonitor, std::nullopt); }

		void make_windowed(window_position position, window_size size) { glfwSetWindowMonitor(m_handle, nullptr, position.x, position.y, size.width, size.height, glfw::DONT_CARE); }

		void minimize() { glfwIconifyWindow(m_handle); }

		bool is_minimized() const { return glfwGetWindowAttrib(m_handle, GLFW_ICONIFIED) == glfw::TRUE; }

		void maximize() { glfwMaximizeWindow(m_handle); }

		bool is_maximized() const { return glfwGetWindowAttrib(m_handle, GLFW_MAXIMIZED) == glfw::TRUE; }

		void restore() { glfwRestoreWindow(m_handle); }

		void hide() { glfwHideWindow(m_handle); }

		void show() { glfwShowWindow(m_handle); }

		bool is_visible() const { return glfwGetWindowAttrib(m_handle, GLFW_VISIBLE) == glfw::TRUE; }

		void set_focus() { glfwFocusWindow(m_handle); }

		bool has_focus() const { return glfwGetWindowAttrib(m_handle, GLFW_FOCUSED) == glfw::TRUE; }

		void request_attention() { glfwRequestWindowAttention(m_handle); }

		void set_resizable(bool canResize) { glfwSetWindowAttrib(m_handle, GLFW_RESIZABLE, canResize ? glfw::TRUE : glfw::FALSE); }

		bool is_resizable() const { return glfwGetWindowAttrib(m_handle, GLFW_RESIZABLE) == glfw::TRUE; }

		void set_decorated(bool hasDecoration) { glfwSetWindowAttrib(m_handle, GLFW_DECORATED, hasDecoration ? glfw::TRUE : glfw::FALSE); }

		bool is_decorated() const { return glfwGetWindowAttrib(m_handle, GLFW_DECORATED) == glfw::TRUE; }

		void set_floating(bool floating) { glfwSetWindowAttrib(m_handle, GLFW_FLOATING, floating ? glfw::TRUE : glfw::FALSE); }

		bool is_floating() const { return glfwGetWindowAttrib(m_handle, GLFW_FLOATING) == glfw::TRUE; }

		void set_minimize_on_focus_loss(bool minimizeOnFocusLoss) { glfwSetWindowAttrib(m_handle, GLFW_AUTO_ICONIFY, minimizeOnFocusLoss ? glfw::TRUE : glfw::FALSE); }

		bool is_minimized_on_focus_loss() const { return glfwGetWindowAttrib(m_handle, GLFW_AUTO_ICONIFY) == glfw::TRUE; }

		void set_focus_on_show(bool focusOnShow) { glfwSetWindowAttrib(m_handle, GLFW_FOCUS_ON_SHOW, focusOnShow ? glfw::TRUE : glfw::FALSE); }

		bool is_focused_on_show() const { return glfwGetWindowAttrib(m_handle, GLFW_FOCUS_ON_SHOW) == glfw::TRUE; }

		bool get_close_request() { return glfwWindowShouldClose(m_handle); }

		void set_close_request(bool value = true) { glfwSetWindowShouldClose(m_handle, value ? glfw::TRUE : glfw::FALSE); }

		bool is_hovered() const { return glfwGetWindowAttrib(m_handle, GLFW_HOVERED) == glfw::TRUE; }

		client_api_type get_client_api() const {
			return client_api_type{ glfwGetWindowAttrib(m_handle, GLFW_CLIENT_API) };
		}

		context_creation_api_type get_context_creation_api() const { return context_creation_api_type{ glfwGetWindowAttrib(m_handle, GLFW_CONTEXT_CREATION_API) }; }

		context_version get_context_version() const {
			return context_version{ glfwGetWindowAttrib(m_handle, GLFW_CONTEXT_VERSION_MAJOR), glfwGetWindowAttrib(m_handle, GLFW_CONTEXT_VERSION_MINOR), glfwGetWindowAttrib(m_handle, GLFW_CONTEXT_REVISION) };
		}

		bool is_context_forward_compatible() const { return glfwGetWindowAttrib(m_handle, GLFW_OPENGL_FORWARD_COMPAT) == glfw::TRUE; }

		bool is_debug_context() const { return glfwGetWindowAttrib(m_handle, GLFW_OPENGL_DEBUG_CONTEXT) == glfw::TRUE; }

		opengl_profile_type get_opengl_profile() const { return opengl_profile_type{ glfwGetWindowAttrib(m_handle, GLFW_OPENGL_PROFILE) }; }

		context_robustness_type get_context_robustness() const { return context_robustness_type{ glfwGetWindowAttrib(m_handle, GLFW_CONTEXT_ROBUSTNESS) }; }



		template<class T>
		std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<T>>>* get_user_pointer() const { return static_cast<T*>(glfwGetWindowUserPointer(m_handle)); }

		template<class T>
		T set_user_pointer(T userPointer) {
			static_assert(std::is_pointer_v<T>, "set_user_pointer only accepts pointer types");
			glfwSetWindowUserPointer(m_handle, static_cast<void*>(userPointer));
		}

		void resize(window_size size) { glfwSetWindowSize(m_handle, size.width, size.height); }

		window_size size() const {
			window_size size;
			glfwGetWindowSize(m_handle, &size.width, &size.height);
			return size;
		}

		window_frame get_window_frame() const {
			window_frame frame;
			glfwGetWindowFrameSize(m_handle, &frame.left, &frame.top, &frame.right, &frame.bottom);
			return frame;
		}

		framebuffer get_framebuffer() const {
			framebuffer fb;
			glfwGetFramebufferSize(m_handle, &fb.width, &fb.height);
			return fb;
		}

		bool has_framebuffer_alpha() const { return glfwGetWindowAttrib(m_handle, GLFW_TRANSPARENT_FRAMEBUFFER) == glfw::TRUE; }

		void swap_buffers() { glfwSwapBuffers(m_handle); }

		float get_opacity() const { return glfwGetWindowOpacity(m_handle); }

		void set_opacity(float opacity) { glfwSetWindowOpacity(m_handle, opacity); }

		window_content_scale get_content_scale() const {
			window_content_scale scale;
			glfwGetWindowContentScale(m_handle, &scale.xScale, &scale.yScale);
			return scale;
		}

		void set_size_limit(window_size_limit limit) { glfwSetWindowSizeLimits(m_handle, limit.minWidth, limit.minHeight, limit.maxWidth, limit.maxHeight); }

		void set_aspect_ratio(aspect_ratio aspect) { glfwSetWindowAspectRatio(m_handle, aspect.num, aspect.denom); }

		window_position get_position() const {
			window_position pos;
			glfwGetWindowPos(m_handle, &pos.x, &pos.y);
			return pos;
		}

		void set_position(window_position pos) { glfwSetWindowPos(m_handle, pos.x, pos.y); }

		void get_title(std::string_view title) const { glfwSetWindowTitle(m_handle, title.data()); }
		/* empty vector has .data = nullptr -> reset to default icon */
		void set_icon_image(std::vector<icon_image> imageCandidates) { glfwSetWindowIcon(m_handle, static_cast<int>(imageCandidates.size()), imageCandidates.data()); }

		std::optional<monitor> get_fullscreen_monitor() const {
			GLFWmonitor* mon = glfwGetWindowMonitor(m_handle);
			return mon ? monitor{ mon } : std::optional<monitor>{ std::nullopt };
		}

		template<class WindowCallback>
		void set_event_callback(WindowCallback&& callback, window_event_type mask) { window_events::set_event_callback(m_handle, std::forward<WindowCallback>(callback), mask); }
		void set_event_callback(std::nullptr_t) { window_events::set_event_callback(m_handle, nullptr); }

		operator GLFWwindow* () { return m_handle; }
	private:
		GLFWwindow* m_handle;
	};

	/* Non-owning window type - this won't destroy the window automatically.
	 * The window api is duplicated here */
	 //TODO: consider forwarding window/window_ref function calls to detail::window_impl to avoid maintainance effort due to duplicate functions
	class window_ref {
	public:
		using client_api_type = attributes::client_api_type;
		using context_creation_api_type = attributes::context_creation_api_type;
		using context_version = attributes::context_version;
		using opengl_profile_type = attributes::opengl_profile_type;
		using context_robustness_type = attributes::context_robustness_type;

		explicit window_ref(GLFWwindow* window) : m_handle(window) {}
		explicit window_ref(window window) : m_handle(window) {}

		void make_fullscreen(monitor fsTargetMonitor, std::optional<video_mode> videoMode = std::nullopt) {
			if (videoMode.has_value()) {
				//fullscreen with specific video mode
				glfwSetWindowMonitor(m_handle, fsTargetMonitor, glfw::DONT_CARE, glfw::DONT_CARE, videoMode->resolution.width, videoMode->resolution.height, videoMode->refresh.rate);
			}
			else {
				//windowed fullscreen mode
				auto curVideoMode = fsTargetMonitor.get_current_video_mode(); //TODO: handle full screen -> windowed fullscreen, need a location to store the previous video mode
				glfwSetWindowMonitor(m_handle, fsTargetMonitor, glfw::DONT_CARE, glfw::DONT_CARE, curVideoMode.resolution.width, curVideoMode.resolution.height, curVideoMode.refresh.rate);
			}
		}

		void make_windowed_fullscreen(monitor fsTargetMonitor) { make_fullscreen(fsTargetMonitor, std::nullopt); }

		void make_windowed(window_position position, window_size size) { glfwSetWindowMonitor(m_handle, nullptr, position.x, position.y, size.width, size.height, glfw::DONT_CARE); }

		void minimize() { glfwIconifyWindow(m_handle); }

		bool is_minimized() const { return glfwGetWindowAttrib(m_handle, GLFW_ICONIFIED) == glfw::TRUE; }

		void maximize() { glfwMaximizeWindow(m_handle); }

		bool is_maximized() const { return glfwGetWindowAttrib(m_handle, GLFW_MAXIMIZED) == glfw::TRUE; }

		void restore() { glfwRestoreWindow(m_handle); }

		void hide() { glfwHideWindow(m_handle); }

		void show() { glfwShowWindow(m_handle); }

		bool is_visible() const { return glfwGetWindowAttrib(m_handle, GLFW_VISIBLE) == glfw::TRUE; }

		void set_focus() { glfwFocusWindow(m_handle); }

		bool has_focus() const { return glfwGetWindowAttrib(m_handle, GLFW_FOCUSED) == glfw::TRUE; }

		void request_attention() { glfwRequestWindowAttention(m_handle); }

		void set_resizable(bool canResize) { glfwSetWindowAttrib(m_handle, GLFW_RESIZABLE, canResize ? glfw::TRUE : glfw::FALSE); }

		bool is_resizable() const { return glfwGetWindowAttrib(m_handle, GLFW_RESIZABLE) == glfw::TRUE; }

		void set_decorated(bool hasDecoration) { glfwSetWindowAttrib(m_handle, GLFW_DECORATED, hasDecoration ? glfw::TRUE : glfw::FALSE); }

		bool is_decorated() const { return glfwGetWindowAttrib(m_handle, GLFW_DECORATED) == glfw::TRUE; }

		void set_floating(bool floating) { glfwSetWindowAttrib(m_handle, GLFW_FLOATING, floating ? glfw::TRUE : glfw::FALSE); }

		bool is_floating() const { return glfwGetWindowAttrib(m_handle, GLFW_FLOATING) == glfw::TRUE; }

		void set_minimize_on_focus_loss(bool minimizeOnFocusLoss) { glfwSetWindowAttrib(m_handle, GLFW_AUTO_ICONIFY, minimizeOnFocusLoss ? glfw::TRUE : glfw::FALSE); }

		bool is_minimized_on_focus_loss() const { return glfwGetWindowAttrib(m_handle, GLFW_AUTO_ICONIFY) == glfw::TRUE; }

		void set_focus_on_show(bool focusOnShow) { glfwSetWindowAttrib(m_handle, GLFW_FOCUS_ON_SHOW, focusOnShow ? glfw::TRUE : glfw::FALSE); }

		bool is_focused_on_show() const { return glfwGetWindowAttrib(m_handle, GLFW_FOCUS_ON_SHOW) == glfw::TRUE; }

		bool get_close_request() { return glfwWindowShouldClose(m_handle); }

		void set_close_request(bool value = true) { glfwSetWindowShouldClose(m_handle, value ? glfw::TRUE : glfw::FALSE); }

		bool is_hovered() const { return glfwGetWindowAttrib(m_handle, GLFW_HOVERED) == glfw::TRUE; }

		client_api_type get_client_api() const {
			return client_api_type{ glfwGetWindowAttrib(m_handle, GLFW_CLIENT_API) };
		}

		context_creation_api_type get_context_creation_api() const { return context_creation_api_type{ glfwGetWindowAttrib(m_handle, GLFW_CONTEXT_CREATION_API) }; }

		context_version get_context_version() const {
			return context_version{ glfwGetWindowAttrib(m_handle, GLFW_CONTEXT_VERSION_MAJOR), glfwGetWindowAttrib(m_handle, GLFW_CONTEXT_VERSION_MINOR), glfwGetWindowAttrib(m_handle, GLFW_CONTEXT_REVISION) };
		}

		bool is_context_forward_compatible() const { return glfwGetWindowAttrib(m_handle, GLFW_OPENGL_FORWARD_COMPAT) == glfw::TRUE; }

		bool is_debug_context() const { return glfwGetWindowAttrib(m_handle, GLFW_OPENGL_DEBUG_CONTEXT) == glfw::TRUE; }

		opengl_profile_type get_opengl_profile() const { return opengl_profile_type{ glfwGetWindowAttrib(m_handle, GLFW_OPENGL_PROFILE) }; }

		context_robustness_type get_context_robustness() const { return context_robustness_type{ glfwGetWindowAttrib(m_handle, GLFW_CONTEXT_ROBUSTNESS) }; }



		template<class T>
		std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<T>>>* get_user_pointer() const { return static_cast<T*>(glfwGetWindowUserPointer(m_handle)); }

		template<class T>
		T set_user_pointer(T userPointer) {
			static_assert(std::is_pointer_v<T>, "set_user_pointer only accepts pointer types");
			glfwSetWindowUserPointer(m_handle, static_cast<void*>(userPointer));
		}

		void resize(window_size size) { glfwSetWindowSize(m_handle, size.width, size.height); }

		window_size size() const {
			window_size size;
			glfwGetWindowSize(m_handle, &size.width, &size.height);
			return size;
		}

		window_frame get_window_frame() const {
			window_frame frame;
			glfwGetWindowFrameSize(m_handle, &frame.left, &frame.top, &frame.right, &frame.bottom);
			return frame;
		}

		framebuffer get_framebuffer() const {
			framebuffer fb;
			glfwGetFramebufferSize(m_handle, &fb.width, &fb.height);
			return fb;
		}

		bool has_framebuffer_alpha() const { return glfwGetWindowAttrib(m_handle, GLFW_TRANSPARENT_FRAMEBUFFER) == glfw::TRUE; }

		void swap_buffers() { glfwSwapBuffers(m_handle); }

		float get_opacity() const { return glfwGetWindowOpacity(m_handle); }

		void set_opacity(float opacity) { glfwSetWindowOpacity(m_handle, opacity); }

		window_content_scale get_content_scale() const {
			window_content_scale scale;
			glfwGetWindowContentScale(m_handle, &scale.xScale, &scale.yScale);
			return scale;
		}

		void set_size_limit(window_size_limit limit) { glfwSetWindowSizeLimits(m_handle, limit.minWidth, limit.minHeight, limit.maxWidth, limit.maxHeight); }

		void set_aspect_ratio(aspect_ratio aspect) { glfwSetWindowAspectRatio(m_handle, aspect.num, aspect.denom); }

		window_position get_position() const {
			window_position pos;
			glfwGetWindowPos(m_handle, &pos.x, &pos.y);
			return pos;
		}

		void set_position(window_position pos) { glfwSetWindowPos(m_handle, pos.x, pos.y); }

		void get_title(std::string_view title) const { glfwSetWindowTitle(m_handle, title.data()); }
		/* empty vector has .data = nullptr -> reset to default icon */
		void set_icon_image(std::vector<icon_image> imageCandidates) { glfwSetWindowIcon(m_handle, static_cast<int>(imageCandidates.size()), imageCandidates.data()); }

		std::optional<monitor> get_fullscreen_monitor() const {
			GLFWmonitor* mon = glfwGetWindowMonitor(m_handle);
			return mon ? monitor{ mon } : std::optional<monitor>{ std::nullopt };
		}

		template<class WindowCallback>
		void set_event_callback(WindowCallback&& callback, window_event_type mask) { window_events::set_event_callback(m_handle, std::forward<WindowCallback>(callback), mask); }
		void set_event_callback(std::nullptr_t) { window_events::set_event_callback(m_handle, nullptr); }

	private:
		GLFWwindow* m_handle;
	};

	/* window builder */

	namespace detail {
		template<class T, class VAR_T>
		struct is_variant_member;

		template<class T, class... VAR_TS>
		struct is_variant_member<T, std::variant<VAR_TS...>>
			: public std::disjunction<std::is_same<T, VAR_TS>...> {};

		template<typename, typename>
		inline constexpr bool is_variant_member_v = false;

		template<class T, class... VAR_TS>
		inline constexpr bool is_variant_member_v<T, std::variant<VAR_TS...>> = is_variant_member < T, std::variant<VAR_TS...>>::value;
	}


	class window_builder {
	public:
		template<class ...hint_ts>
		explicit window_builder(hint_ts&& ... hints) {
			static_assert((detail::is_variant_member_v<hint_ts, attributes::window_hints> && ...));
			(m_hints.emplace_back(std::forward<hint_ts>(hints)), ...);
		}
		static void apply_hint(attributes::hint const& windowHint) { glfwWindowHint(static_cast<int>(windowHint.hint), static_cast<int>(windowHint.value)); }
		static void apply_hint(attributes::value_hint const& windowHint) { glfwWindowHint(static_cast<int>(windowHint.hint), static_cast<int>(windowHint.value)); }
		static void apply_hint(attributes::opengl_profile_hint const& windowHint) { glfwWindowHint(GLFW_OPENGL_PROFILE, static_cast<int>(windowHint.profile)); }
		static void apply_hint(attributes::robustness_hint const& windowHint) { glfwWindowHint(GLFW_CONTEXT_ROBUSTNESS, static_cast<int>(windowHint.robustness)); }
		static void apply_hint(attributes::client_api_hint const& windowHint) { glfwWindowHint(GLFW_CLIENT_API, static_cast<int>(windowHint.api)); }
		static void apply_hint(attributes::context_creation_api_hint const& windowHint) { glfwWindowHint(GLFW_CONTEXT_CREATION_API, static_cast<int>(windowHint.api)); }
		static void apply_hint(attributes::context_release_behaviour_hint const& windowHint) { glfwWindowHint(GLFW_CONTEXT_RELEASE_BEHAVIOR, static_cast<int>(windowHint.behaviour)); }
		static void apply_hint(attributes::string_hint const& windowHint) { glfwWindowHintString(static_cast<int>(windowHint.hint), windowHint.value.c_str()); }
		static void restore_defaults() { glfwDefaultWindowHints(); }

		window create(window_size size, std::string_view title, std::optional<monitor> fullscreenLocation = std::nullopt, window* sharedContext = nullptr) {
			for (auto& hint : m_hints) {
				std::visit([](auto& hint) { window_builder::apply_hint(hint);}, hint);
			}
			auto win = window{ size, title, fullscreenLocation, sharedContext };
			window_builder::restore_defaults();
			return win;
		}
	private:
		std::vector<attributes::window_hints> m_hints;
	};

	/************************************************************************************
	 *																					*
	 *								EVENTS & CALLBACKS									*
	 *																					*
	 ************************************************************************************/

	enum class monitor_event_type : int {
		CONNECTED = GLFW_CONNECTED,
		DISCONNECTED = GLFW_DISCONNECTED,
	};

	struct monitor_event {
		monitor monitorObject;
		monitor_event_type monitorStatus;
	};

	enum class error_type : int {
		NO_ERROR = GLFW_NO_ERROR,
		NOT_INITIALIZED = GLFW_NOT_INITIALIZED,
		NO_CURRENT_CONTEXT = GLFW_NO_CURRENT_CONTEXT,
		INVALID_ENUM = GLFW_INVALID_ENUM,
		INVALID_VALUE = GLFW_INVALID_VALUE,
		OUT_OF_MEMORY = GLFW_OUT_OF_MEMORY,
		API_UNAVAILABLE = GLFW_API_UNAVAILABLE,
		VERSION_UNAVAILABLE = GLFW_VERSION_UNAVAILABLE,
		PLATFORM_ERROR = GLFW_PLATFORM_ERROR,
		FORMAT_UNAVAILABLE = GLFW_FORMAT_UNAVAILABLE,
		NO_WINDOW_CONTEXT = GLFW_NO_WINDOW_CONTEXT,
	};

	struct error {
		error_type errorType;
		std::string_view description;
	};


	enum class key_type : int {
		KEY_SPACE = GLFW_KEY_SPACE,
		KEY_APOSTROPHE = GLFW_KEY_APOSTROPHE,
		KEY_COMMA = GLFW_KEY_COMMA,
		KEY_MINUS = GLFW_KEY_MINUS,
		KEY_PERIOD = GLFW_KEY_PERIOD,
		KEY_SLASH = GLFW_KEY_SLASH,
		KEY_0 = GLFW_KEY_0,
		KEY_1 = GLFW_KEY_1,
		KEY_2 = GLFW_KEY_2,
		KEY_3 = GLFW_KEY_3,
		KEY_4 = GLFW_KEY_4,
		KEY_5 = GLFW_KEY_5,
		KEY_6 = GLFW_KEY_6,
		KEY_7 = GLFW_KEY_7,
		KEY_8 = GLFW_KEY_8,
		KEY_9 = GLFW_KEY_9,
		KEY_SEMICOLON = GLFW_KEY_SEMICOLON,
		KEY_EQUAL = GLFW_KEY_EQUAL,
		KEY_A = GLFW_KEY_A,
		KEY_B = GLFW_KEY_B,
		KEY_C = GLFW_KEY_C,
		KEY_D = GLFW_KEY_D,
		KEY_E = GLFW_KEY_E,
		KEY_F = GLFW_KEY_F,
		KEY_G = GLFW_KEY_G,
		KEY_H = GLFW_KEY_H,
		KEY_I = GLFW_KEY_I,
		KEY_J = GLFW_KEY_J,
		KEY_K = GLFW_KEY_K,
		KEY_L = GLFW_KEY_L,
		KEY_M = GLFW_KEY_M,
		KEY_N = GLFW_KEY_N,
		KEY_O = GLFW_KEY_O,
		KEY_P = GLFW_KEY_P,
		KEY_Q = GLFW_KEY_Q,
		KEY_R = GLFW_KEY_R,
		KEY_S = GLFW_KEY_S,
		KEY_T = GLFW_KEY_T,
		KEY_U = GLFW_KEY_U,
		KEY_V = GLFW_KEY_V,
		KEY_W = GLFW_KEY_W,
		KEY_X = GLFW_KEY_X,
		KEY_Y = GLFW_KEY_Y,
		KEY_Z = GLFW_KEY_Z,
		KEY_LEFT_BRACKET = GLFW_KEY_LEFT_BRACKET,
		KEY_BACKSLASH = GLFW_KEY_BACKSLASH,
		KEY_RIGHT_BRACKET = GLFW_KEY_RIGHT_BRACKET,
		KEY_GRAVE_ACCENT = GLFW_KEY_GRAVE_ACCENT,
		KEY_WORLD_1 = GLFW_KEY_WORLD_1,
		KEY_WORLD_2 = GLFW_KEY_WORLD_2,

		KEY_ESCAPE = GLFW_KEY_ESCAPE,
		KEY_ENTER = GLFW_KEY_ENTER,
		KEY_TAB = GLFW_KEY_TAB,
		KEY_BACKSPACE = GLFW_KEY_BACKSPACE,
		KEY_INSERT = GLFW_KEY_INSERT,
		KEY_DELETE = GLFW_KEY_DELETE,
		KEY_RIGHT = GLFW_KEY_RIGHT,
		KEY_LEFT = GLFW_KEY_LEFT,
		KEY_DOWN = GLFW_KEY_DOWN,
		KEY_UP = GLFW_KEY_UP,
		KEY_PAGE_UP = GLFW_KEY_PAGE_UP,
		KEY_PAGE_DOWN = GLFW_KEY_PAGE_DOWN,
		KEY_HOME = GLFW_KEY_HOME,
		KEY_END = GLFW_KEY_END,
		KEY_CAPS_LOCK = GLFW_KEY_CAPS_LOCK,
		KEY_SCROLL_LOCK = GLFW_KEY_SCROLL_LOCK,
		KEY_NUM_LOCK = GLFW_KEY_NUM_LOCK,
		KEY_PRINT_SCREEN = GLFW_KEY_PRINT_SCREEN,
		KEY_PAUSE = GLFW_KEY_PAUSE,
		KEY_F1 = GLFW_KEY_F1,
		KEY_F2 = GLFW_KEY_F2,
		KEY_F3 = GLFW_KEY_F3,
		KEY_F4 = GLFW_KEY_F4,
		KEY_F5 = GLFW_KEY_F5,
		KEY_F6 = GLFW_KEY_F6,
		KEY_F7 = GLFW_KEY_F7,
		KEY_F8 = GLFW_KEY_F8,
		KEY_F9 = GLFW_KEY_F9,
		KEY_F10 = GLFW_KEY_F10,
		KEY_F11 = GLFW_KEY_F11,
		KEY_F12 = GLFW_KEY_F12,
		KEY_F13 = GLFW_KEY_F13,
		KEY_F14 = GLFW_KEY_F14,
		KEY_F15 = GLFW_KEY_F15,
		KEY_F16 = GLFW_KEY_F16,
		KEY_F17 = GLFW_KEY_F17,
		KEY_F18 = GLFW_KEY_F18,
		KEY_F19 = GLFW_KEY_F19,
		KEY_F20 = GLFW_KEY_F20,
		KEY_F21 = GLFW_KEY_F21,
		KEY_F22 = GLFW_KEY_F22,
		KEY_F23 = GLFW_KEY_F23,
		KEY_F24 = GLFW_KEY_F24,
		KEY_F25 = GLFW_KEY_F25,
		KEY_KP_0 = GLFW_KEY_KP_0,
		KEY_KP_1 = GLFW_KEY_KP_1,
		KEY_KP_2 = GLFW_KEY_KP_2,
		KEY_KP_3 = GLFW_KEY_KP_3,
		KEY_KP_4 = GLFW_KEY_KP_4,
		KEY_KP_5 = GLFW_KEY_KP_5,
		KEY_KP_6 = GLFW_KEY_KP_6,
		KEY_KP_7 = GLFW_KEY_KP_7,
		KEY_KP_8 = GLFW_KEY_KP_8,
		KEY_KP_9 = GLFW_KEY_KP_9,
		KEY_KP_DECIMAL = GLFW_KEY_KP_DECIMAL,
		KEY_KP_DIVIDE = GLFW_KEY_KP_DIVIDE,
		KEY_KP_MULTIPLY = GLFW_KEY_KP_MULTIPLY,
		KEY_KP_SUBTRACT = GLFW_KEY_KP_SUBTRACT,
		KEY_KP_ADD = GLFW_KEY_KP_ADD,
		KEY_KP_ENTER = GLFW_KEY_KP_ENTER,
		KEY_KP_EQUAL = GLFW_KEY_KP_EQUAL,
		KEY_LEFT_SHIFT = GLFW_KEY_LEFT_SHIFT,
		KEY_LEFT_CONTROL = GLFW_KEY_LEFT_CONTROL,
		KEY_LEFT_ALT = GLFW_KEY_LEFT_ALT,
		KEY_LEFT_SUPER = GLFW_KEY_LEFT_SUPER,
		KEY_RIGHT_SHIFT = GLFW_KEY_RIGHT_SHIFT,
		KEY_RIGHT_CONTROL = GLFW_KEY_RIGHT_CONTROL,
		KEY_RIGHT_ALT = GLFW_KEY_RIGHT_ALT,
		KEY_RIGHT_SUPER = GLFW_KEY_RIGHT_SUPER,
		KEY_MENU = GLFW_KEY_MENU,
	};

	enum class key_action_type {
		PRESS,
		HOLD,
		RELEASE,
	};

	enum key_modifier_flags : int {
		SHIFT,
		CONTROL,
		ALT,
		SUPER,
		CAPS_LOCK,
		NUM_LOCK,
	};

	struct key_event {
		window_ref window;
		key_type key;
		int scancode;
		key_action_type action;
		key_modifier_flags modifiers;
	};

	namespace detail {
		struct window_callback {
			std::function<void(window_ref)> callback;
			uint16_t mask;
		};

		namespace glfw_callbacks {
			static std::function<void(error)> error_callback;
			static std::function<void(monitor_event)> monitor_callback;
			static std::unordered_map<GLFWwindow*, window_callback> window_callbacks;
			static std::unordered_map<GLFWwindow*, std::function<void(key_event)>> key_callbacks;

			inline void glfw_monitor_callback(GLFWmonitor* glfwMonitor, int eventType) {
				glfw_callbacks::monitor_callback(monitor_event{ monitor{ glfwMonitor }, monitor_event_type{eventType} });
			}

			inline void glfw_error_callback(int error, const char* description) {
				glfw_callbacks::error_callback(glfw::error{ error_type{ error }, std::string_view{ description } });
			}

			inline void glfw_window_pos_callback(GLFWwindow* sourceWindow, int, int) {
				if (auto cb = window_callbacks.find(sourceWindow); cb != window_callbacks.end() && cb->second.mask & POSITION_CHANGED && cb->second.callback) cb->second.callback(window_ref{ sourceWindow });
			}

			inline void glfw_window_size_callback(GLFWwindow* sourceWindow, int, int) {
				if (auto cb = window_callbacks.find(sourceWindow); cb != window_callbacks.end() && cb->second.mask & SIZE_CHANGED && cb->second.callback) cb->second.callback(window_ref{ sourceWindow });
			}

			inline void glfw_framebuffer_size_callback(GLFWwindow* sourceWindow, int, int) {
				if (auto cb = window_callbacks.find(sourceWindow); cb != window_callbacks.end() && cb->second.mask & FRAMEBUFFER_SIZE_CHANGED && cb->second.callback) cb->second.callback(window_ref{ sourceWindow });
			}

			inline void glfw_window_content_scale_callback(GLFWwindow* sourceWindow, float, float) {
				if (auto cb = window_callbacks.find(sourceWindow); cb != window_callbacks.end() && cb->second.mask & CONTENT_SCALE_CHANGED && cb->second.callback) cb->second.callback(window_ref{ sourceWindow });
			}

			inline void glfw_window_focus_callback(GLFWwindow* sourceWindow, int) {
				if (auto cb = window_callbacks.find(sourceWindow); cb != window_callbacks.end() && cb->second.mask & FOCUS_CHANGED && cb->second.callback) cb->second.callback(window_ref{ sourceWindow });
			}

			inline void glfw_window_minimize_callback(GLFWwindow* sourceWindow, int) {
				if (auto cb = window_callbacks.find(sourceWindow); cb != window_callbacks.end() && cb->second.mask & MINIMIZE_STATE_CHANGED && cb->second.callback) cb->second.callback(window_ref{ sourceWindow });
			}

			inline void glfw_window_maximize_callback(GLFWwindow* sourceWindow, int) {
				if (auto cb = window_callbacks.find(sourceWindow); cb != window_callbacks.end() && cb->second.mask & MAXIMIZE_STATE_CHANGED && cb->second.callback) cb->second.callback(window_ref{ sourceWindow });
			}

			inline void glfw_window_refresh_callback(GLFWwindow* sourceWindow) {
				if (auto cb = window_callbacks.find(sourceWindow); cb != window_callbacks.end() && cb->second.mask & CONTENT_NEEDS_REFRESH && cb->second.callback) cb->second.callback(window_ref{ sourceWindow });
			}
			inline void glfw_window_close_callback(GLFWwindow* sourceWindow) {
				if (auto cb = window_callbacks.find(sourceWindow); cb != window_callbacks.end() && cb->second.mask & CLOSE_REQUESTED && cb->second.callback) cb->second.callback(window_ref{ sourceWindow });
			}
		};

	}


	namespace monitor_events {

		template<class MonitorCallback>
		inline void set_event_callback(MonitorCallback&& callback) {
			static_assert(std::is_invocable_v<MonitorCallback, monitor_event>);
			detail::glfw_callbacks::monitor_callback = std::forward<MonitorCallback>(callback);
			glfwSetMonitorCallback(&detail::glfw_callbacks::glfw_monitor_callback);
		}

		inline void set_event_callback(std::nullptr_t) {
			detail::glfw_callbacks::monitor_callback = nullptr;
			glfwSetMonitorCallback(nullptr);
		}

	};


	namespace window_events {

		template<class WindowCallback>
		inline void set_event_callback(GLFWwindow* window, WindowCallback&& callback, window_event_type mask) {
			static_assert(std::is_invocable_v<WindowCallback, window_ref>);

			detail::glfw_callbacks::window_callbacks[window] = detail::window_callback{ std::forward<WindowCallback>(callback), mask };

			glfwSetWindowPosCallback(window, (mask & POSITION_CHANGED) == 1 ? &detail::glfw_callbacks::glfw_window_pos_callback : nullptr);
			glfwSetWindowSizeCallback(window, (mask & SIZE_CHANGED) == 1 ? &detail::glfw_callbacks::glfw_window_size_callback : nullptr);
			glfwSetFramebufferSizeCallback(window, (mask & FRAMEBUFFER_SIZE_CHANGED) == 1 ? &detail::glfw_callbacks::glfw_framebuffer_size_callback : nullptr);
			glfwSetWindowContentScaleCallback(window, (mask & CONTENT_SCALE_CHANGED) == 1 ? &detail::glfw_callbacks::glfw_window_content_scale_callback : nullptr);
			glfwSetWindowFocusCallback(window, (mask & FOCUS_CHANGED) == 1 ? &detail::glfw_callbacks::glfw_window_focus_callback : nullptr);
			glfwSetWindowIconifyCallback(window, (mask & MINIMIZE_STATE_CHANGED) == 1 ? &detail::glfw_callbacks::glfw_window_minimize_callback : nullptr);
			glfwSetWindowMaximizeCallback(window, (mask & MAXIMIZE_STATE_CHANGED) == 1 ? &detail::glfw_callbacks::glfw_window_maximize_callback : nullptr);
			glfwSetWindowRefreshCallback(window, (mask & CONTENT_NEEDS_REFRESH) == 1 ? &detail::glfw_callbacks::glfw_window_refresh_callback : nullptr);
			glfwSetWindowCloseCallback(window, (mask & CLOSE_REQUESTED) == 1 ? &detail::glfw_callbacks::glfw_window_close_callback : nullptr);
		}

		inline void set_event_callback(GLFWwindow* window, std::nullptr_t) {
			detail::glfw_callbacks::window_callbacks[window].callback = nullptr;

			glfwSetWindowPosCallback(window, nullptr);
			glfwSetWindowSizeCallback(window, nullptr);
			glfwSetFramebufferSizeCallback(window, nullptr);
			glfwSetWindowContentScaleCallback(window, nullptr);
			glfwSetWindowFocusCallback(window, nullptr);
			glfwSetWindowIconifyCallback(window, nullptr);
			glfwSetWindowMaximizeCallback(window, nullptr);
			glfwSetWindowRefreshCallback(window, nullptr);
			glfwSetWindowCloseCallback(window, nullptr);
		}

	}


	namespace errors {

		template<class ErrorCallback>
		inline void set_callback(ErrorCallback&& callback) {
			static_assert(std::is_invocable_v<ErrorCallback, error>);
			detail::glfw_callbacks::error_callback = std::forward<ErrorCallback>(callback);
			glfwSetErrorCallback(&detail::glfw_callbacks::glfw_error_callback);
		}

		inline void set_callback(std::nullptr_t) {
			detail::glfw_callbacks::error_callback = nullptr;
			glfwSetErrorCallback(nullptr);
		}

		inline error getError() {
			const char* desc = nullptr;
			auto err = error_type{ glfwGetError(&desc) };
			if (desc) return error{ err, std::string_view{desc} };
			return error{ err, std::string_view{} };
		}

		inline error_type getErrorType() {
			return error_type{ glfwGetError(nullptr) };
		}

	};


}
