#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace zwin
{
	enum class Key : std::uint16_t
	{
		Unknown = 0,

		A, B, C, D, E, F, G, H, I, J, K, L, M,
		N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
		Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,

		F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

		Escape, Enter, Tab, Backspace, Space, Delete, Insert,
		Left, Right, Up, Down, Home, End, PageUp, PageDown,

		Minus, Equal, Comma, Period, Slash, Semicolon, Apostrophe, Grave,
		BracketLeft, BracketRight, Backslash,

		Shift, Ctrl, Alt, Super,
	};

	enum class KeyMod : std::uint8_t
	{
		None = 0,
		Shift = 1 << 0,
		Ctrl = 1 << 1,
		Alt = 1 << 2,
		Super = 1 << 3,
	};

	inline KeyMod operator|(KeyMod a, KeyMod b)
	{
		return static_cast<KeyMod>(static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b));
	}

	inline KeyMod operator&(KeyMod a, KeyMod b)
	{
		return static_cast<KeyMod>(static_cast<std::uint8_t>(a) & static_cast<std::uint8_t>(b));
	}

	enum class MouseButton : std::uint8_t
	{
		Left,
		Right,
		Middle,
		X1,
		X2,
	};

	struct WindowConfig
	{
		std::string title = "ZWindow";
		std::uint32_t width = 1280;
		std::uint32_t height = 720;
		bool resizable = true;
		bool fullscreen = false;
	};

	struct NativeWindow
	{
		void* window = nullptr;
		void* instance = nullptr;
	};

	class Application
	{
	public:
		explicit Application(int argc, char** argv);
		~Application();

		Application(const Application&) = delete;
		Application& operator=(const Application&) = delete;
		Application(Application&&) = delete;
		Application& operator=(Application&&) = delete;
	};

	class ZWindow
	{
	public:
		static std::unique_ptr<ZWindow> Create(const WindowConfig& config);
		static const char* backendName();

		ZWindow(const ZWindow&) = delete;
		ZWindow& operator=(const ZWindow&) = delete;
		ZWindow(ZWindow&&) = delete;
		ZWindow& operator=(ZWindow&&) = delete;

		virtual ~ZWindow() = default;

		virtual bool show() = 0;
		virtual void requestClose() = 0;
		virtual bool isOpen() const = 0;
		virtual bool processEvents() = 0;
		virtual void waitEvents() = 0;
		virtual void setTitle(std::string_view title) = 0;
		virtual void presentImage(std::uint32_t , std::uint32_t, const std::uint8_t*) {}

		virtual NativeWindow nativeWindow() const = 0;

		std::uint32_t clientWidth() const { return m_width; }
		std::uint32_t clientHeight() const { return m_height; }

		std::function<void()> onClose;
		std::function<void(char32_t codepoint)> onChar;
		std::function<void(Key key, bool down, bool repeat, KeyMod mods)> onKey;
		std::function<void(std::uint32_t width, std::uint32_t height)> onResize;
		std::function<void(std::int32_t deltaX, std::int32_t deltaY)> onMouseWheel;
		std::function<void(std::int32_t x, std::int32_t y, KeyMod mods)> onMouseMove;
		std::function<void(MouseButton button, bool down, KeyMod mods)> onMouseButton;
		
	protected:
		ZWindow() = default;

		bool m_shouldClose = false;
		std::uint32_t m_width = 0;
		std::uint32_t m_height = 0;
	};
}
