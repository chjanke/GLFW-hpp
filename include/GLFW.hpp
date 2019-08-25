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
#include <array>

namespace glfw {

	using image = GLFWimage;
	using gamma_ramp = GLFWgammaramp;

	inline constexpr int DONT_CARE = GLFW_DONT_CARE;
	inline constexpr int FALSE = GLFW_FALSE;
	inline constexpr int TRUE = GLFW_TRUE;

	inline void set_swap_interval(int swapInterval) {
		glfwSwapInterval(swapInterval);
	}
	/* Events */

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

	/* Time */
	inline double time() { return glfwGetTime(); }
	inline uint64_t time_raw() { return glfwGetTimerValue(); }
	inline uint64_t timer_frequency() { return glfwGetTimerFrequency(); }
	inline void set_current_time(double seconds) { glfwSetTime(seconds); }

	/* Clipboard Utility */
	std::string_view clip_text() {
		const char* text = glfwGetClipboardString(nullptr);
		if (text) return std::string_view(text);
		return std::string_view{};
	}

	void set_clip_text(std::string_view clipText) { glfwSetClipboardString(nullptr, clipText.data()); }



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
		bool hintEnabled;
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
			(glfwInitHint(static_cast<int>(initHints.hint), initHints.hintEnabled ? TRUE : FALSE), ...);
		}
		static void init() {
			static glfw_lib libInstance;
		}
	};

#ifndef GLFWHPP_MANUAL_INIT
	namespace detail {
		struct glfw_lib_auto_init {
			glfw_lib_auto_init() { glfw_lib::init(); }
		} static glfw_lib_auto_init;
	}
#endif


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

	struct cursor_hotspot_position {
		int x, y;
	};

	enum class standard_cursor_shape : int {
		ARROW = GLFW_ARROW_CURSOR,
		IBEAM = GLFW_IBEAM_CURSOR,
		CROSSHAIR = GLFW_CROSSHAIR_CURSOR,
		HAND = GLFW_HAND_CURSOR,
		HRESIZE_ARROW = GLFW_HRESIZE_CURSOR,
		VRESIZE_ARROW = GLFW_VRESIZE_CURSOR,
	};

	class cursor {
		cursor(GLFWcursor* handle) : m_handle(handle) {}

	public:
		~cursor() { glfwDestroyCursor(m_handle); }

		static std::optional<cursor> create(image cursorImage, cursor_hotspot_position hotspot = { 0,0 }) {
			auto cursorHandle = glfwCreateCursor(&cursorImage, hotspot.x, hotspot.y);
			if (!cursorHandle) return std::nullopt;
			return cursor{ cursorHandle };
		}

		static cursor create_standard_cursor(standard_cursor_shape shape) {
			return cursor{ glfwCreateStandardCursor(static_cast<int>(shape)) };
		}

		static cursor get_default_cursor() { return cursor{ nullptr }; }

		operator GLFWcursor* () { return m_handle; }

	private:
		GLFWcursor* m_handle;
	};

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
			std::string text;
		};

		struct hint {
			hint_type hint;
			bool enabled;
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
	enum class code_point : uint32_t;
	namespace input {
		template<class KeyCallback>
		inline void set_key_callback(GLFWwindow*, KeyCallback&&);
		inline void set_key_callback(GLFWwindow*, std::nullptr_t);
	}
	/* TODO: add set_xxx_callback to window api */

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

		void set_resizable(bool canResize = true) { glfwSetWindowAttrib(m_handle, GLFW_RESIZABLE, canResize ? glfw::TRUE : glfw::FALSE); }

		bool is_resizable() const { return glfwGetWindowAttrib(m_handle, GLFW_RESIZABLE) == glfw::TRUE; }

		void set_decorated(bool hasDecoration = true) { glfwSetWindowAttrib(m_handle, GLFW_DECORATED, hasDecoration ? glfw::TRUE : glfw::FALSE); }

		bool is_decorated() const { return glfwGetWindowAttrib(m_handle, GLFW_DECORATED) == glfw::TRUE; }

		void set_floating(bool floating = true) { glfwSetWindowAttrib(m_handle, GLFW_FLOATING, floating ? glfw::TRUE : glfw::FALSE); }

		bool is_floating() const { return glfwGetWindowAttrib(m_handle, GLFW_FLOATING) == glfw::TRUE; }

		void set_minimize_on_focus_loss(bool minimizeOnFocusLoss = true) { glfwSetWindowAttrib(m_handle, GLFW_AUTO_ICONIFY, minimizeOnFocusLoss ? glfw::TRUE : glfw::FALSE); }

		bool is_minimized_on_focus_loss() const { return glfwGetWindowAttrib(m_handle, GLFW_AUTO_ICONIFY) == glfw::TRUE; }

		void set_focus_on_show(bool focusOnShow = true) { glfwSetWindowAttrib(m_handle, GLFW_FOCUS_ON_SHOW, focusOnShow ? glfw::TRUE : glfw::FALSE); }

		bool is_focused_on_show() const { return glfwGetWindowAttrib(m_handle, GLFW_FOCUS_ON_SHOW) == glfw::TRUE; }

		bool get_close_request() { return glfwWindowShouldClose(m_handle); }

		void set_close_request(bool enabled = true) { glfwSetWindowShouldClose(m_handle, enabled ? glfw::TRUE : glfw::FALSE); }

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

		void set_cursor(cursor newCursor) { glfwSetCursor(m_handle, newCursor); }

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
		void set_icon_image(std::vector<image> imageCandidates) { glfwSetWindowIcon(m_handle, static_cast<int>(imageCandidates.size()), imageCandidates.data()); }

		std::optional<monitor> get_fullscreen_monitor() const {
			GLFWmonitor* mon = glfwGetWindowMonitor(m_handle);
			return mon ? monitor{ mon } : std::optional<monitor>{ std::nullopt };
		}

		template<class WindowCallback>
		void set_event_callback(WindowCallback&& callback, window_event_type mask) { window_events::set_event_callback(m_handle, std::forward<WindowCallback>(callback), mask); }
		void set_event_callback(std::nullptr_t) { window_events::set_event_callback(m_handle, nullptr); }

		template<class KeyCallback>
		void set_key_callback(KeyCallback&& callback) { input::set_key_callback(m_handle, std::forward<KeyCallback>); }
		void set_key_callback(std::nullptr_t) { input::set_key_callback(m_handle, nullptr); }

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

		void set_resizable(bool canResize = true) { glfwSetWindowAttrib(m_handle, GLFW_RESIZABLE, canResize ? glfw::TRUE : glfw::FALSE); }

		bool is_resizable() const { return glfwGetWindowAttrib(m_handle, GLFW_RESIZABLE) == glfw::TRUE; }

		void set_decorated(bool hasDecoration = true) { glfwSetWindowAttrib(m_handle, GLFW_DECORATED, hasDecoration ? glfw::TRUE : glfw::FALSE); }

		bool is_decorated() const { return glfwGetWindowAttrib(m_handle, GLFW_DECORATED) == glfw::TRUE; }

		void set_floating(bool floating = true) { glfwSetWindowAttrib(m_handle, GLFW_FLOATING, floating ? glfw::TRUE : glfw::FALSE); }

		bool is_floating() const { return glfwGetWindowAttrib(m_handle, GLFW_FLOATING) == glfw::TRUE; }

		void set_minimize_on_focus_loss(bool minimizeOnFocusLoss = true) { glfwSetWindowAttrib(m_handle, GLFW_AUTO_ICONIFY, minimizeOnFocusLoss ? glfw::TRUE : glfw::FALSE); }

		bool is_minimized_on_focus_loss() const { return glfwGetWindowAttrib(m_handle, GLFW_AUTO_ICONIFY) == glfw::TRUE; }

		void set_focus_on_show(bool focusOnShow = true) { glfwSetWindowAttrib(m_handle, GLFW_FOCUS_ON_SHOW, focusOnShow ? glfw::TRUE : glfw::FALSE); }

		bool is_focused_on_show() const { return glfwGetWindowAttrib(m_handle, GLFW_FOCUS_ON_SHOW) == glfw::TRUE; }

		bool get_close_request() { return glfwWindowShouldClose(m_handle); }

		void set_close_request(bool enabled = true) { glfwSetWindowShouldClose(m_handle, enabled ? glfw::TRUE : glfw::FALSE); }

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

		void set_cursor(cursor newCursor) { glfwSetCursor(m_handle, newCursor); }

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
		void set_icon_image(std::vector<image> imageCandidates) { glfwSetWindowIcon(m_handle, static_cast<int>(imageCandidates.size()), imageCandidates.data()); }

		std::optional<monitor> get_fullscreen_monitor() const {
			GLFWmonitor* mon = glfwGetWindowMonitor(m_handle);
			return mon ? monitor{ mon } : std::optional<monitor>{ std::nullopt };
		}

		template<class WindowCallback>
		void set_event_callback(WindowCallback&& callback, window_event_type mask) { window_events::set_event_callback(m_handle, std::forward<WindowCallback>(callback), mask); }
		void set_event_callback(std::nullptr_t) { window_events::set_event_callback(m_handle, nullptr); }

		template<class KeyCallback>
		void set_key_callback(KeyCallback&& callback) { input::set_key_callback(m_handle, std::forward<KeyCallback>); }
		void set_key_callback(std::nullptr_t) { input::set_key_callback(m_handle, nullptr); }

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
		static void apply_hint(attributes::hint const& windowHint) { glfwWindowHint(static_cast<int>(windowHint.hint), windowHint.enabled ? TRUE : FALSE); }
		static void apply_hint(attributes::value_hint const& windowHint) { glfwWindowHint(static_cast<int>(windowHint.hint), static_cast<int>(windowHint.value)); }
		static void apply_hint(attributes::opengl_profile_hint const& windowHint) { glfwWindowHint(GLFW_OPENGL_PROFILE, static_cast<int>(windowHint.profile)); }
		static void apply_hint(attributes::robustness_hint const& windowHint) { glfwWindowHint(GLFW_CONTEXT_ROBUSTNESS, static_cast<int>(windowHint.robustness)); }
		static void apply_hint(attributes::client_api_hint const& windowHint) { glfwWindowHint(GLFW_CLIENT_API, static_cast<int>(windowHint.api)); }
		static void apply_hint(attributes::context_creation_api_hint const& windowHint) { glfwWindowHint(GLFW_CONTEXT_CREATION_API, static_cast<int>(windowHint.api)); }
		static void apply_hint(attributes::context_release_behaviour_hint const& windowHint) { glfwWindowHint(GLFW_CONTEXT_RELEASE_BEHAVIOR, static_cast<int>(windowHint.behaviour)); }
		static void apply_hint(attributes::string_hint const& windowHint) { glfwWindowHintString(static_cast<int>(windowHint.hint), windowHint.text.c_str()); }
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


	enum class key : int {
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
		KEY_UNKNOWN = GLFW_KEY_UNKNOWN,
	};

	enum class key_action : int {
		PRESS = GLFW_PRESS,
		HOLD = GLFW_REPEAT,
		RELEASE = GLFW_RELEASE,
	};

	enum modifier_flags : int {
		SHIFT,
		CONTROL,
		ALT,
		SUPER,
		CAPS_LOCK,
		NUM_LOCK,
	};

	struct key_event {
		window_ref window;
		key key;
		int scancode;
		key_action action;
		modifier_flags modifiers;
	};

	struct char_event {
		window_ref window;
		code_point codepoint;
	};

	struct cursor_position {
		double x, y;
	};

	struct cursor_event {
		window_ref window;
		cursor_position pos;
	};

	struct cursor_enter_event {
		window_ref window;
		bool entered;
	};

	enum class mouse_button : int {
		BUTTON_LEFT = GLFW_MOUSE_BUTTON_LEFT,
		BUTTON_RIGHT = GLFW_MOUSE_BUTTON_RIGHT,
		BUTTON_MIDDLE = GLFW_MOUSE_BUTTON_MIDDLE,
		BUTTON_1 = GLFW_MOUSE_BUTTON_1,
		BUTTON_2 = GLFW_MOUSE_BUTTON_2,
		BUTTON_3 = GLFW_MOUSE_BUTTON_3,
		BUTTON_4 = GLFW_MOUSE_BUTTON_4,
		BUTTON_5 = GLFW_MOUSE_BUTTON_5,
		BUTTON_6 = GLFW_MOUSE_BUTTON_6,
		BUTTON_7 = GLFW_MOUSE_BUTTON_7,
		BUTTON_8 = GLFW_MOUSE_BUTTON_8,
	};

	enum class mouse_button_action : int {
		PRESSED = GLFW_PRESS,
		RELEASED = GLFW_RELEASE,
	};

	struct mouse_button_event {
		window_ref window;
		mouse_button button;
		mouse_button_action action;
		modifier_flags modifiers;
	};

	struct mouse_scroll_offset {
		double xOffset, yOffset;
	};

	struct mouse_scroll_event {
		window_ref window;
		mouse_scroll_offset scroll;
	};

	enum class joystick_id : int {
		JOYSTICK_1 = GLFW_JOYSTICK_1,
		JOYSTICK_2 = GLFW_JOYSTICK_2,
		JOYSTICK_3 = GLFW_JOYSTICK_3,
		JOYSTICK_4 = GLFW_JOYSTICK_4,
		JOYSTICK_5 = GLFW_JOYSTICK_5,
		JOYSTICK_6 = GLFW_JOYSTICK_6,
		JOYSTICK_7 = GLFW_JOYSTICK_7,
		JOYSTICK_8 = GLFW_JOYSTICK_8,
		JOYSTICK_9 = GLFW_JOYSTICK_9,
		JOYSTICK_10 = GLFW_JOYSTICK_10,
		JOYSTICK_11 = GLFW_JOYSTICK_11,
		JOYSTICK_12 = GLFW_JOYSTICK_12,
		JOYSTICK_13 = GLFW_JOYSTICK_13,
		JOYSTICK_14 = GLFW_JOYSTICK_14,
		JOYSTICK_15 = GLFW_JOYSTICK_15,
		JOYSTICK_16 = GLFW_JOYSTICK_16,
	};

	struct joystick_axes {
		using axis_state = float;
		axis_state const* axes;
		int count;
		axis_state operator[](size_t axis) { return axes[axis]; }
		static constexpr float axis_state_min = -1.0f;
		static constexpr float axis_state_max = 1.0f;
	};

	enum class joystick_button_action : unsigned char {
		PRESSED = GLFW_PRESS,
		RELEASED = GLFW_RELEASE,
	};

	enum class joystick_hat_action : unsigned char {
		CENTERED = GLFW_HAT_CENTERED,
		UP = GLFW_HAT_UP,
		RIGHT = GLFW_HAT_RIGHT,
		DOWN = GLFW_HAT_DOWN,
		LEFT = GLFW_HAT_LEFT,
		RIGHT_UP = GLFW_HAT_RIGHT_UP,
		RIGHT_DOWN = GLFW_HAT_RIGHT_DOWN,
		LEFT_UP = GLFW_HAT_LEFT_UP,
		LEFT_DOWN = GLFW_HAT_LEFT_DOWN,
	};

	struct joystick_buttons {
		using joystick_button = unsigned char;
		joystick_button const* buttons;
		int count;
		joystick_button_action operator[](size_t buttonIndex) { return joystick_button_action{ buttons[buttonIndex] }; }
	};

	struct joystick_hats {
		using joystick_hat = unsigned char;
		joystick_hat const* hats;
		int count;
		joystick_hat_action operator[](size_t hatIndex) { return joystick_hat_action{ hats[hatIndex] }; }
	};

	enum class joystick_state : int {
		CONNECTED = GLFW_CONNECTED,
		DISCONNECTED = GLFW_DISCONNECTED,
	};

	struct joystick_event {
		joystick_id joystick;
		joystick_state state;
	};

	enum class gamepad_button : size_t {
		BUTTON_A = GLFW_GAMEPAD_BUTTON_A,
		BUTTON_B = GLFW_GAMEPAD_BUTTON_B,
		BUTTON_X = GLFW_GAMEPAD_BUTTON_X,
		BUTTON_Y = GLFW_GAMEPAD_BUTTON_Y,
		LEFT_BUMPER = GLFW_GAMEPAD_BUTTON_LEFT_BUMPER,
		RIGHT_BUMPER = GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER,
		BACK = GLFW_GAMEPAD_BUTTON_BACK,
		START = GLFW_GAMEPAD_BUTTON_START,
		GUIDE = GLFW_GAMEPAD_BUTTON_GUIDE,
		LEFT_THUMB = GLFW_GAMEPAD_BUTTON_LEFT_THUMB,
		RIGHT_THUMB = GLFW_GAMEPAD_BUTTON_RIGHT_THUMB,
		DPAD_UP = GLFW_GAMEPAD_BUTTON_DPAD_UP,
		DPAD_RIGHT = GLFW_GAMEPAD_BUTTON_DPAD_RIGHT,
		DPAD_DOWN = GLFW_GAMEPAD_BUTTON_DPAD_DOWN,
		DPAD_LEFT = GLFW_GAMEPAD_BUTTON_DPAD_LEFT,
		CROSS = GLFW_GAMEPAD_BUTTON_CROSS,
		CIRCLE = GLFW_GAMEPAD_BUTTON_CIRCLE,
		SQUARE = GLFW_GAMEPAD_BUTTON_SQUARE,
		TRIANGLE = GLFW_GAMEPAD_BUTTON_TRIANGLE,
	};

	enum class gamepad_axis : size_t {
		LEFT_X = GLFW_GAMEPAD_AXIS_LEFT_X,
		LEFT_Y = GLFW_GAMEPAD_AXIS_LEFT_Y,
		RIGHT_X = GLFW_GAMEPAD_AXIS_RIGHT_X,
		RIGHT_Y = GLFW_GAMEPAD_AXIS_RIGHT_Y,
		LEFT_TRIGGER = GLFW_GAMEPAD_AXIS_LEFT_TRIGGER,
		RIGHT_TRIGGER = GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER,
	};

	struct drop_event {
		window_ref window;
		std::vector<std::string_view> paths;
	};


	namespace detail {
		struct window_callback {
			std::function<void(window_ref)> callback;
			uint16_t mask;
		};

		namespace glfw_callbacks {
			inline static std::function<void(error)> error_callback;

			inline static std::function<void(monitor_event)> monitor_callback;

			inline static std::unordered_map<GLFWwindow*, window_callback> window_callbacks;

			inline static std::unordered_map<GLFWwindow*, std::function<void(key_event)>> key_callbacks;
			inline static std::unordered_map<GLFWwindow*, std::function<void(char_event)>> char_callbacks;

			inline static std::unordered_map<GLFWwindow*, std::function<void(cursor_event)>> cursor_callbacks;
			inline static std::unordered_map<GLFWwindow*, std::function<void(cursor_enter_event)>> cursor_enter_callbacks;
			inline static std::unordered_map<GLFWwindow*, std::function<void(mouse_button_event)>> mouse_button_callbacks;
			inline static std::unordered_map<GLFWwindow*, std::function<void(mouse_scroll_event)>> mouse_scroll_callbacks;

			inline static std::unordered_map < GLFWwindow*, std::function<void(drop_event)>> drop_callbacks;

			inline static std::function<void(joystick_event)> joystick_callback;



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

			inline void glfw_drop_callback(GLFWwindow* sourceWindow, int count, const char** paths) {
				if (auto cb = drop_callbacks.find(sourceWindow); cb != drop_callbacks.end() && cb->second) {
					drop_event dropEvent{ window_ref{sourceWindow} };
					dropEvent.paths.reserve(count);
					for (size_t i = 0; i < count; ++i) {
						dropEvent.paths.emplace_back(paths[i]);
					}
					cb->second(std::move(dropEvent));
				}
			}


			inline void glfw_key_callback(GLFWwindow* sourceWindow, int key, int scanCode, int action, int modifiers) {
				if (auto cb = key_callbacks.find(sourceWindow); cb != key_callbacks.end() && cb->second) cb->second(key_event{ window_ref{sourceWindow}, glfw::key{key}, scanCode, key_action{action}, modifier_flags{modifiers} });
			}

			inline void glfw_char_callback(GLFWwindow* sourceWindow, uint32_t codepoint) {
				if (auto cb = char_callbacks.find(sourceWindow); cb != char_callbacks.end() && cb->second) cb->second(char_event{ window_ref{sourceWindow}, code_point{codepoint} });
			}


			inline void glfw_cursor_callback(GLFWwindow* sourceWindow, double xpos, double ypos) {
				if (auto cb = cursor_callbacks.find(sourceWindow); cb != cursor_callbacks.end() && cb->second) cb->second(cursor_event{ window_ref{sourceWindow}, cursor_position{xpos,ypos} });
			}

			inline void glfw_cursor_enter_callback(GLFWwindow* sourceWindow, int entered) {
				if (auto cb = cursor_enter_callbacks.find(sourceWindow); cb != cursor_enter_callbacks.end() && cb->second) cb->second(cursor_enter_event{ window_ref{sourceWindow}, entered == GLFW_TRUE });
			}

			inline void glfw_mouse_button_callback(GLFWwindow* sourceWindow, int button, int action, int mods) {
				if (auto cb = mouse_button_callbacks.find(sourceWindow); cb != mouse_button_callbacks.end() && cb->second) cb->second(mouse_button_event{ window_ref{sourceWindow}, mouse_button{button}, mouse_button_action{action}, modifier_flags{mods} });
			}

			inline void glfw_mouse_scroll_callback(GLFWwindow* sourceWindow, double xOffset, double yOffset) {
				if (auto cb = mouse_scroll_callbacks.find(sourceWindow); cb != mouse_scroll_callbacks.end() && cb->second) cb->second(mouse_scroll_event{ window_ref{sourceWindow}, mouse_scroll_offset{xOffset, yOffset} });
			}



			inline void glfw_joystick_callback(int id, int event) {
				if (joystick_callback) joystick_callback(joystick_event{ joystick_id{id}, joystick_state{event} });
			}
		}
	}

	enum class key_input_mode : int {
		STICKY_KEYS,
		LOCK_KEY_MODIFIERS,
	};

	enum class cursor_input_mode : int {
		NORMAL = GLFW_CURSOR_NORMAL,
		DISABLED = GLFW_CURSOR_DISABLED,
		HIDDEN = GLFW_CURSOR_HIDDEN,
	};

	namespace detail {
		//internal gamepad mappings
		inline static std::array<GLFWgamepadstate, GLFW_JOYSTICK_LAST> gamepad_states;
	}

	enum class gamepad_button_state : unsigned char {
		PRESSED = GLFW_PRESS,
		RELEASED = GLFW_RELEASE,
	};

	struct gamepad_state {
		struct gamepad_buttons {
			using button = unsigned char;
			button* gamepad_buttons;
			gamepad_button_state operator[](gamepad_button buttonIndex) { return gamepad_button_state{ gamepad_buttons[static_cast<size_t>(buttonIndex)] }; }
		} buttons;
		struct gamepad_axes {
			using axis_state = float;
			axis_state* gamepad_axes;
			axis_state operator[](gamepad_axis axis) { return gamepad_axes[static_cast<size_t>(axis)]; }
			static constexpr float axis_state_min = -1.0f;
			static constexpr float axis_state_max = 1.0f;
		} axes;
		static constexpr size_t BUTTON_COUNT = GLFW_GAMEPAD_BUTTON_LAST + 1;
		static constexpr size_t AXES_COUNT = GLFW_GAMEPAD_AXIS_LAST + 1;
	};

	namespace input {

		/* Keyboard and Mouse */

		inline int to_scancode(key key) { return glfwGetKeyScancode(static_cast<int>(key)); }
		inline std::string_view key_name(key key) { return std::string_view{ glfwGetKeyName(static_cast<int>(key),0) }; }
		inline std::string_view key_name(int scancode) { return std::string_view{ glfwGetKeyName(0, scancode) }; }

		inline void set_key_input_mode(GLFWwindow* window, key_input_mode mode, bool enabled) { glfwSetInputMode(window, static_cast<int>(mode), enabled ? TRUE : FALSE); }
		inline key_action last_key_action(GLFWwindow* window, key key) { return key_action{ glfwGetKey(window, static_cast<int>(key)) }; }
		inline cursor_position current_get_cursor_position(GLFWwindow* window) {
			cursor_position pos;
			glfwGetCursorPos(window, &pos.x, &pos.y);
			return pos;
		}
		inline mouse_button_action get_mouse_button_action(GLFWwindow* window, mouse_button button) {
			return mouse_button_action{ glfwGetMouseButton(window, static_cast<int>(button)) };
		}
		inline void set_sticky_mouse_input_mode(GLFWwindow* window, bool enabled) { glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, enabled ? TRUE : FALSE); }
		inline void set_cursor_input_mode(GLFWwindow* window, cursor_input_mode mode) {
			glfwSetInputMode(window, GLFW_CURSOR, static_cast<int>(mode));
		}
		inline void use_raw_cursor(GLFWwindow* window, bool enabled = true) {
			glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, enabled ? TRUE : FALSE);
		}

		template<class KeyCallback>
		inline void set_key_callback(GLFWwindow* window, KeyCallback&& callback) {
			static_assert(std::is_invocable_v<KeyCallback, key_event>);
			detail::glfw_callbacks::key_callbacks[window] = std::forward<KeyCallback>(callback);
			glfwSetKeyCallback(window, &detail::glfw_callbacks::glfw_key_callback);
		}

		inline void set_key_callback(GLFWwindow* window, std::nullptr_t) {
			detail::glfw_callbacks::key_callbacks[window] = nullptr;
			glfwSetKeyCallback(window, nullptr);
		}

		template<class CharCallback>
		inline void set_char_callback(GLFWwindow* window, CharCallback&& callback) {
			static_assert(std::is_invocable_v<CharCallback, char_event>);
			detail::glfw_callbacks::char_callbacks[window] = std::forward<CharCallback>(callback);
			glfwSetCharCallback(window, &detail::glfw_callbacks::glfw_char_callback);
		}

		inline void set_char_callback(GLFWwindow* window, std::nullptr_t) {
			detail::glfw_callbacks::char_callbacks[window] = nullptr;
			glfwSetCharCallback(window, nullptr);
		}

		template<class CursorCallback>
		inline void set_cursor_callback(GLFWwindow* window, CursorCallback&& callback) {
			static_assert(std::is_invocable_v<CursorCallback, cursor_event>);
			detail::glfw_callbacks::cursor_callbacks[window] = std::forward<CursorCallback>(callback);
			glfwSetCursorPosCallback(window, &detail::glfw_callbacks::glfw_cursor_callback);
		}

		inline void set_cursor_callback(GLFWwindow* window, std::nullptr_t) {
			detail::glfw_callbacks::cursor_callbacks[window] = nullptr;
			glfwSetCursorPosCallback(window, nullptr);
		}

		template<class CursorEnterCallback>
		inline void set_cursor_enter_callback(GLFWwindow* window, CursorEnterCallback&& callback) {
			static_assert(std::is_invocable_v<CursorEnterCallback, cursor_enter_event>);
			detail::glfw_callbacks::cursor_enter_callbacks[window] = std::forward<CursorEnterCallback>(callback);
			glfwSetCursorEnterCallback(window, &detail::glfw_callbacks::glfw_cursor_enter_callback);
		}

		inline void set_cursor_enter_callback(GLFWwindow* window, std::nullptr_t) {
			detail::glfw_callbacks::cursor_enter_callbacks[window] = nullptr;
			glfwSetCursorEnterCallback(window, nullptr);
		}

		template<class MouseButtonCallback>
		inline void set_mouse_button_callback(GLFWwindow* window, MouseButtonCallback&& callback) {
			static_assert(std::is_invocable_v<MouseButtonCallback, mouse_button_event>);
			detail::glfw_callbacks::mouse_button_callbacks[window] = std::forward<MouseButtonCallback>(callback);
			glfwSetMouseButtonCallback(window, &detail::glfw_callbacks::glfw_mouse_button_callback);
		}

		inline void set_mouse_button_callback(GLFWwindow* window, std::nullptr_t) {
			detail::glfw_callbacks::mouse_button_callbacks[window] = nullptr;
			glfwSetMouseButtonCallback(window, nullptr);
		}

		template<class MouseScrollCallback>
		inline void set_mouse_scroll_callback(GLFWwindow* window, MouseScrollCallback&& callback) {
			static_assert(std::is_invocable_v<MouseScrollCallback, mouse_scroll_event>);
			detail::glfw_callbacks::mouse_scroll_callbacks[window] = std::forward<MouseScrollCallback>(callback);
			glfwSetScrollCallback(window, &detail::glfw_callbacks::glfw_mouse_scroll_callback);
		}

		inline void set_mouse_scroll_callback(GLFWwindow* window, std::nullptr_t) {
			detail::glfw_callbacks::mouse_scroll_callbacks[window] = nullptr;
			glfwSetScrollCallback(window, nullptr);
		}

		/* Joystick / Controllers */

		inline bool is_joystick_present(joystick_id joystick) { return glfwJoystickPresent(static_cast<int>(joystick)) == GLFW_TRUE; }
		inline std::string_view joystick_name(joystick_id joystick) {
			const char* name = glfwGetJoystickName(static_cast<int>(joystick));
			if (name) return std::string_view{ name };
			return std::string_view{};
		}
		template<class T>
		std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<T>>>* get_joystick_user_pointer(joystick_id joystick) { return static_cast<T*>(glfwGetJoystickUserPointer(static_cast<int>(joystick))); }

		template<class T>
		T set_joystick_user_pointer(joystick_id joystick, T userPointer) {
			static_assert(std::is_pointer_v<T>, "set_user_pointer only accepts pointer types");
			glfwSetJoystickUserPointer(static_cast<int>(joystick), static_cast<void*>(userPointer));
		}

		template<class JoystickCallback>
		inline void set_joystick_callback(JoystickCallback&& callback) {
			static_assert(std::is_invocable_v<JoystickCallback, joystick_event>);
			detail::glfw_callbacks::joystick_callback = std::forward<JoystickCallback>(callback);
			glfwSetJoystickCallback(&detail::glfw_callbacks::glfw_joystick_callback);
		}

		inline void set_joystick_callback(std::nullptr_t) {
			detail::glfw_callbacks::joystick_callback = nullptr;
			glfwSetJoystickCallback(nullptr);
		}

		/* Gamepad */

		inline bool is_gamepad(joystick_id joystick) { return glfwJoystickIsGamepad(static_cast<int>(joystick)); }
		inline std::string_view gamepad_name(joystick_id joystick) {
			const char* name = glfwGetGamepadName(static_cast<int>(joystick));
			if (name) return std::string_view{ name };
			return std::string_view{};
		}
		inline void update_mappings(std::string_view mappings) { glfwUpdateGamepadMappings(mappings.data()); }

		inline gamepad_state current_gamepad_state(joystick_id joystick) {
			GLFWgamepadstate* state = &detail::gamepad_states[static_cast<int>(joystick)];
			glfwGetGamepadState(static_cast<int>(joystick), state);
			return gamepad_state{ state->buttons, state->axes };
		}
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

		template<class DropCallback>
		inline void set_drop_callback(GLFWwindow* window, DropCallback&& callback) {
			static_assert(std::is_invocable_v<DropCallback, drop_event>);

			detail::glfw_callbacks::drop_callbacks[window] = std::forward<DropCallback>(callback);
			glfwSetDropCallback(window, &detail::glfw_callbacks::glfw_drop_callback);
		}

		inline void set_drop_callback(GLFWwindow* window, std::nullptr_t) {
			detail::glfw_callbacks::drop_callbacks[window] = nullptr;
			glfwSetDropCallback(window, nullptr);
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
