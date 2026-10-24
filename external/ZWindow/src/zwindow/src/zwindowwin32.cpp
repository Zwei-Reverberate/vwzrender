#include <zwindow.h>
#include <windows.h>
#include <windowsx.h>
#include <cstddef>
#include <cstdio>
#include <string>
#include <string_view>

namespace zwin
{
	namespace
	{
		constexpr wchar_t kWindowClassName[] = L"ZWindowClass";
		constexpr COLORREF kBackgroundColor = RGB(24, 24, 32); // WM_PAINT 占位底色(防花屏)

		// DPI 感知只需设置一次(窗口类注册见 ensureWindowClass)
		bool s_dpiAwarenessSet = false;

		std::wstring toWide(std::string_view utf8)
		{
			if (utf8.empty())
			{
				return {};
			}
			const int length = ::MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
				static_cast<int>(utf8.size()), nullptr, 0);
			if (length <= 0)
			{
				return {};
			}
			std::wstring wide(static_cast<std::size_t>(length), L'\0');
			::MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
				static_cast<int>(utf8.size()), wide.data(), length);
			return wide;
		}

		// Per-Monitor V2 DPI 感知:像素即物理像素;跨显示器移动时由 WM_DPICHANGED 通知
		void ensureDpiAwareness()
		{
			if (s_dpiAwarenessSet)
			{
				return;
			}
			s_dpiAwarenessSet = true;

			if (::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) == FALSE)
			{
				std::fprintf(stderr, "[zwin] SetProcessDpiAwarenessContext failed (error %lu)\n",
					::GetLastError());
			}

		}

		// VK 虚拟键码 -> 逻辑键(未覆盖的物理键映射为 Unknown)
		Key mapVirtualKey(std::uint32_t virtualKey)
		{
			switch (virtualKey)
			{
			case 'A': return Key::A;
			case 'B': return Key::B;
			case 'C': return Key::C;
			case 'D': return Key::D;
			case 'E': return Key::E;
			case 'F': return Key::F;
			case 'G': return Key::G;
			case 'H': return Key::H;
			case 'I': return Key::I;
			case 'J': return Key::J;
			case 'K': return Key::K;
			case 'L': return Key::L;
			case 'M': return Key::M;
			case 'N': return Key::N;
			case 'O': return Key::O;
			case 'P': return Key::P;
			case 'Q': return Key::Q;
			case 'R': return Key::R;
			case 'S': return Key::S;
			case 'T': return Key::T;
			case 'U': return Key::U;
			case 'V': return Key::V;
			case 'W': return Key::W;
			case 'X': return Key::X;
			case 'Y': return Key::Y;
			case 'Z': return Key::Z;
			case '0': return Key::Num0;
			case '1': return Key::Num1;
			case '2': return Key::Num2;
			case '3': return Key::Num3;
			case '4': return Key::Num4;
			case '5': return Key::Num5;
			case '6': return Key::Num6;
			case '7': return Key::Num7;
			case '8': return Key::Num8;
			case '9': return Key::Num9;
			case VK_F1: return Key::F1;
			case VK_F2: return Key::F2;
			case VK_F3: return Key::F3;
			case VK_F4: return Key::F4;
			case VK_F5: return Key::F5;
			case VK_F6: return Key::F6;
			case VK_F7: return Key::F7;
			case VK_F8: return Key::F8;
			case VK_F9: return Key::F9;
			case VK_F10: return Key::F10;
			case VK_F11: return Key::F11;
			case VK_F12: return Key::F12;
			case VK_ESCAPE: return Key::Escape;
			case VK_RETURN: return Key::Enter;
			case VK_TAB: return Key::Tab;
			case VK_BACK: return Key::Backspace;
			case VK_SPACE: return Key::Space;
			case VK_DELETE: return Key::Delete;
			case VK_INSERT: return Key::Insert;
			case VK_LEFT: return Key::Left;
			case VK_RIGHT: return Key::Right;
			case VK_UP: return Key::Up;
			case VK_DOWN: return Key::Down;
			case VK_HOME: return Key::Home;
			case VK_END: return Key::End;
			case VK_PRIOR: return Key::PageUp;
			case VK_NEXT: return Key::PageDown;
			case VK_OEM_MINUS: return Key::Minus;
			case VK_OEM_PLUS: return Key::Equal;
			case VK_OEM_COMMA: return Key::Comma;
			case VK_OEM_PERIOD: return Key::Period;
			case VK_OEM_2: return Key::Slash;
			case VK_OEM_1: return Key::Semicolon;
			case VK_OEM_7: return Key::Apostrophe;
			case VK_OEM_3: return Key::Grave;
			case VK_OEM_4: return Key::BracketLeft;
			case VK_OEM_6: return Key::BracketRight;
			case VK_OEM_5: return Key::Backslash;
			case VK_SHIFT:
			case VK_LSHIFT:
			case VK_RSHIFT: return Key::Shift;
			case VK_CONTROL:
			case VK_LCONTROL:
			case VK_RCONTROL: return Key::Ctrl;
			case VK_MENU:
			case VK_LMENU:
			case VK_RMENU: return Key::Alt;
			case VK_LWIN:
			case VK_RWIN: return Key::Super;
			default: return Key::Unknown;
			}
		}

		// 修饰键实时状态(Win32 消息不带修饰位,统一查 GetKeyState)
		KeyMod currentModifiers()
		{
			KeyMod mods = KeyMod::None;
			if ((::GetKeyState(VK_SHIFT) & 0x8000) != 0)
			{
				mods = mods | KeyMod::Shift;
			}
			if ((::GetKeyState(VK_CONTROL) & 0x8000) != 0)
			{
				mods = mods | KeyMod::Ctrl;
			}
			if ((::GetKeyState(VK_MENU) & 0x8000) != 0)
			{
				mods = mods | KeyMod::Alt;
			}
			if (((::GetKeyState(VK_LWIN) | ::GetKeyState(VK_RWIN)) & 0x8000) != 0)
			{
				mods = mods | KeyMod::Super;
			}
			return mods;
		}

	}

	// 原生 Win32 窗口 + 消息泵 + 输入事件分发;线程约束:所有方法与回调都必须在
	// 创建窗口的线程上调用(Win32 消息队列为线程私有)。
	class ZWindowWin32 final : public ZWindow
	{
	public:
		explicit ZWindowWin32(const WindowConfig& config);
		~ZWindowWin32() override;

		bool show() override;
		void requestClose() override;
		bool isOpen() const override;
		bool processEvents() override;
		void waitEvents() override;
		void setTitle(std::string_view title) override;
		NativeWindow nativeWindow() const override;
		void presentImage(std::uint32_t width, std::uint32_t height,
			const std::uint8_t* pixels) override;

	private:
		static LRESULT CALLBACK wndProcThunk(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
		static bool ensureWindowClass(HINSTANCE instance);

		LRESULT handleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
		// 按下捕获鼠标:拖拽移出窗口仍持续上报位置;全部按键抬起后释放
		void updateMouseCapture(MouseButton button, bool down);
		void destroy();

		HINSTANCE m_instance = nullptr;
		HWND m_hwnd = nullptr;
		bool m_quit = false; // 收到 WM_QUIT(应用自行 PostQuitMessage)
		char32_t m_pendingHighSurrogate = 0;
		bool m_buttonDown[5] = {}; // 索引 = MouseButton 枚举值
		std::uint8_t m_capturedButtonCount = 0; // 已按下且参与鼠标捕获的按键数

	};

	Application::Application(int, char**)
	{
		ensureDpiAwareness();
	}

	Application::~Application() = default;

	ZWindowWin32::ZWindowWin32(const WindowConfig& config)
	{
		m_instance = ::GetModuleHandleW(nullptr);

		if (!ensureWindowClass(m_instance))
		{
			return;
		}

		DWORD style = WS_OVERLAPPEDWINDOW;
		if (!config.resizable)
		{
			style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
		}

		int x = CW_USEDEFAULT;
		int y = CW_USEDEFAULT;
		int width = 0;
		int height = 0;

		if (config.fullscreen)
		{
			// 全屏:无边框弹出窗口覆盖主显示器
			style = WS_POPUP;
			x = 0;
			y = 0;
			width = ::GetSystemMetrics(SM_CXSCREEN);
			height = ::GetSystemMetrics(SM_CYSCREEN);
		}
		else
		{
			// 反推:让"客户区"精确等于请求尺寸(系统会自动加上边框与标题栏)
			RECT rect{ 0, 0, static_cast<LONG>(config.width), static_cast<LONG>(config.height) };
			::AdjustWindowRectExForDpi(&rect, style, FALSE, 0, ::GetDpiForSystem());
			width = rect.right - rect.left;
			height = rect.bottom - rect.top;
		}

		m_width = config.width; // 初值;创建期间收到的 WM_SIZE 会覆盖为真实客户区
		m_height = config.height;

		const std::wstring title = toWide(config.title);
		m_hwnd = ::CreateWindowExW(0, kWindowClassName, title.c_str(), style,
			x, y, width, height, nullptr, nullptr, m_instance,
			this);

		if (m_hwnd == nullptr)
		{
			std::fprintf(stderr, "[zwin] CreateWindowExW failed (error %lu)\n", ::GetLastError());
			return;
		}

		std::fprintf(stderr, "[zwin] window created (client %ux%u, backend Win32)\n", m_width, m_height);

	}

	ZWindowWin32::~ZWindowWin32()
	{
		destroy();
	}

	bool ZWindowWin32::show()
	{
		// m_shouldClose 检查与 Qt 后端对齐:已经请求过关闭就不再显示
		if (m_hwnd == nullptr || m_shouldClose)
		{
			return false;
		}
		::ShowWindow(m_hwnd, SW_SHOW);
		::UpdateWindow(m_hwnd);
		return true;
	}

	void ZWindowWin32::requestClose()
	{
		if (m_hwnd != nullptr)
		{
			::PostMessageW(m_hwnd, WM_CLOSE, 0, 0);
		}
	}

	bool ZWindowWin32::isOpen() const
	{
		return m_hwnd != nullptr && !m_shouldClose && !m_quit;
	}

	bool ZWindowWin32::processEvents()
	{
		MSG msg{};
		while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE) != FALSE)
		{
			if (msg.message == WM_QUIT)
			{
				m_quit = true;
				break;
			}
			::TranslateMessage(&msg); // WM_KEYDOWN -> WM_CHAR 的翻译发生在这里
			::DispatchMessageW(&msg);
		}
		return isOpen();
	}

	void ZWindowWin32::waitEvents()
	{
		if (!isOpen())
		{
			return;
		}
		// 队列为空时休眠等待,不消耗 CPU
		::WaitMessage();
		processEvents();
	}

	void ZWindowWin32::setTitle(std::string_view title)
	{
		if (m_hwnd == nullptr)
		{
			return;
		}
		const std::wstring wide = toWide(title);
		::SetWindowTextW(m_hwnd, wide.c_str());
	}

	void ZWindowWin32::presentImage(std::uint32_t width, std::uint32_t height,
		const std::uint8_t* pixels)
	{
		if (width == 0 || height == 0 || pixels == nullptr)
		{
			std::fprintf(stderr, "[zwin] presentImage: 参数非法 (w=%u h=%u ptr=%p)\n",
				width, height, static_cast<const void*>(pixels));
			return;
		}
		if (m_hwnd == nullptr)
		{
			return;
		}

		// 直接把整帧像素铺到客户区。
		//
		// 用自顶向下的 32 位 DIB：biHeight 取负值，这样第 0 行就是屏幕最上面一行，
		// 与"行优先、自上而下"的约定一致，不需要额外翻转。
		// 源像素是 RGBA，而 DIB 要的是 BGRA —— 两者在内存里的前三个字节顺序相同
		// （R,G,B 与 B,G,R 的差别只在 32 位字的解读上），
		// 对 BI_RGB 的 32bpp DIB，Windows 按 0x00RRGGBB 解释，正好与我们的打包一致。
		BITMAPINFO info{};
		info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		info.bmiHeader.biWidth = static_cast<LONG>(width);
		info.bmiHeader.biHeight = -static_cast<LONG>(height);
		info.bmiHeader.biPlanes = 1;
		info.bmiHeader.biBitCount = 32;
		info.bmiHeader.biCompression = BI_RGB;

		HDC dc = ::GetDC(m_hwnd);
		if (dc == nullptr)
		{
			std::fprintf(stderr, "[zwin] presentImage: GetDC failed (error %lu)\n", ::GetLastError());
			return;
		}

		const int copied = ::StretchDIBits(dc,
			0, 0, static_cast<int>(width), static_cast<int>(height),
			0, 0, static_cast<int>(width), static_cast<int>(height),
			pixels, &info, DIB_RGB_COLORS, SRCCOPY);

		if (copied == GDI_ERROR)
		{
			std::fprintf(stderr, "[zwin] presentImage: StretchDIBits failed (error %lu)\n",
				::GetLastError());
		}

		::ReleaseDC(m_hwnd, dc);
	}

	NativeWindow ZWindowWin32::nativeWindow() const
	{
		NativeWindow native;
		native.window = m_hwnd;
		native.instance = m_instance;
		return native;
	}

	void ZWindowWin32::destroy()
	{
		if (m_hwnd == nullptr)
		{
			return;
		}
		// DestroyWindow 同步派发 WM_DESTROY / WM_NCDESTROY;刻意不 PostQuitMessage:
		// 关闭由 m_shouldClose 驱动,队列不留残余 WM_QUIT,消息泵仍可服务其他窗口
		::DestroyWindow(m_hwnd);
		m_hwnd = nullptr;
	}

	bool ZWindowWin32::ensureWindowClass(HINSTANCE instance)
	{
		// 函数内 static:进程内只注册一次(线程安全)
		static const bool registered = [instance]
			{
				WNDCLASSEXW wc{};
				wc.cbSize = sizeof(wc);
				wc.style = CS_HREDRAW | CS_VREDRAW; // 尺寸变化时整窗重绘
				wc.lpfnWndProc = &ZWindowWin32::wndProcThunk; // 静态 thunk,实例指针走 GWLP_USERDATA
				wc.hInstance = instance;
				wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
				wc.hbrBackground = nullptr; // 背景由 WM_PAINT 自绘,禁用系统擦除
				wc.lpszClassName = kWindowClassName;

				if (::RegisterClassExW(&wc) == 0)
				{
					const DWORD error = ::GetLastError();
					if (error == ERROR_CLASS_ALREADY_EXISTS)
					{
						return true; // 进程内已有同名类(多个 ZWindow 实例共享一个类)
					}
					std::fprintf(stderr, "[zwin] RegisterClassExW failed (error %lu)\n", error);
					return false;
				}
				return true;
			}();
		return registered;

	}

	LRESULT CALLBACK ZWindowWin32::wndProcThunk(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		// WM_NCCREATE 是第一条消息:必须先在 thunk 里把实例指针写入 GWLP_USERDATA,
		// 之后的消息才有实例可取(放进实例方法则永远到不了该分支:鸡生蛋问题)
		if (message == WM_NCCREATE)
		{
			auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
			auto* self = static_cast<ZWindowWin32*>(create->lpCreateParams);
			self->m_hwnd = hwnd;
			::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
		}

		auto* self = reinterpret_cast<ZWindowWin32*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
		if (self == nullptr)
		{
			return ::DefWindowProcW(hwnd, message, wParam, lParam);
		}
		return self->handleMessage(hwnd, message, wParam, lParam);

	}

	void ZWindowWin32::updateMouseCapture(MouseButton button, bool down)
	{
		const auto index = static_cast<std::size_t>(button);
		if (down && !m_buttonDown[index])
		{
			m_buttonDown[index] = true;
			if (m_capturedButtonCount++ == 0)
			{
				::SetCapture(m_hwnd);
			}
		}
		else if (!down && m_buttonDown[index])
		{
			m_buttonDown[index] = false;
			if (m_capturedButtonCount > 0)
			{
				--m_capturedButtonCount;
			}
			if (m_capturedButtonCount == 0)
			{
				::ReleaseCapture();
			}
		}
	}

	LRESULT ZWindowWin32::handleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_CLOSE:
			// 延迟销毁(与 GLFW 一致):只置标记并通知,不在此 DestroyWindow
			if (!m_shouldClose)
			{
				m_shouldClose = true;
				if (onClose)
				{
					onClose();
				}
			}
			return 0;

		case WM_NCDESTROY:
		{
			m_hwnd = nullptr; // 与 isOpen() 联动:原生窗口已不存在
			::SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
			return ::DefWindowProcW(hwnd, message, wParam, lParam);
		}

		case WM_SIZE:
		{
			m_width = static_cast<std::uint32_t>(LOWORD(lParam));
			m_height = static_cast<std::uint32_t>(HIWORD(lParam));
			// 最小化时客户区为 (0, 0),照实通知——渲染端必须显式处理
			if (onResize)
			{
				onResize(m_width, m_height);
			}
			return 0;
		}

		case WM_DPICHANGED:
		{
			// 跨 DPI 显示器:系统在 lParam 给出建议矩形,照做即可
			const auto* suggested = reinterpret_cast<const RECT*>(lParam);
			::SetWindowPos(hwnd, nullptr, suggested->left, suggested->top,
				suggested->right - suggested->left, suggested->bottom - suggested->top,
				SWP_NOZORDER | SWP_NOACTIVATE);
			return 0;
		}

		case WM_ERASEBKGND:
			return 1; // 背景由 WM_PAINT 覆盖,禁止系统擦除,避免闪烁

		case WM_PAINT:
		{
			PAINTSTRUCT ps{};
			HDC dc = ::BeginPaint(hwnd, &ps);
			// 占位底色;避免窗口诞生/缩放瞬间花屏
			::SetDCBrushColor(dc, kBackgroundColor);
			::FillRect(dc, &ps.rcPaint, static_cast<HBRUSH>(::GetStockObject(DC_BRUSH)));
			::EndPaint(hwnd, &ps);
			return 0;
		}

		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYUP:
		{
			// Alt+F4 交回系统:由 DefWindowProc 生成 WM_CLOSE,走统一关闭流程
			if ((message == WM_SYSKEYDOWN || message == WM_SYSKEYUP) && wParam == VK_F4
				&& (::GetKeyState(VK_MENU) & 0x8000) != 0)
			{
				return ::DefWindowProcW(hwnd, message, wParam, lParam);
			}
			const bool down = (message == WM_KEYDOWN || message == WM_SYSKEYDOWN);
			// bit30 = 前一个键状态:仅"按下且之前已按住"才算重复(auto-repeat)
			const bool repeat = down && ((lParam >> 30) & 1) != 0;
			if (onKey)
			{
				onKey(mapVirtualKey(static_cast<std::uint32_t>(wParam)), down, repeat, currentModifiers());
			}
			return 0;
		}

		case WM_CHAR:
		{
			// WM_CHAR 载荷为 UTF-16 码元;辅助平面字符(如 emoji)由代理对组成,在此组装
			const char32_t unit = static_cast<char32_t>(wParam);
			if (unit >= 0xD800 && unit <= 0xDBFF)
			{ // 高代理:先记下,等低代理
				m_pendingHighSurrogate = unit;
				return 0;
			}
			char32_t codepoint = unit;
			if (unit >= 0xDC00 && unit <= 0xDFFF && m_pendingHighSurrogate != 0)
			{
				codepoint = 0x10000 + ((m_pendingHighSurrogate - 0xD800) << 10) + (unit - 0xDC00);
			}
			m_pendingHighSurrogate = 0;
			if (onChar)
			{
				onChar(codepoint);
			}
			return 0;
		}

		case WM_MOUSEMOVE:
			if (onMouseMove)
			{
				onMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), currentModifiers());
			}
			return 0;

		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP:
		{
			MouseButton button = MouseButton::Left;
			if (message == WM_RBUTTONDOWN || message == WM_RBUTTONUP)
			{
				button = MouseButton::Right;
			}
			else if (message == WM_MBUTTONDOWN || message == WM_MBUTTONUP)
			{
				button = MouseButton::Middle;
			}
			else if (message == WM_XBUTTONDOWN || message == WM_XBUTTONUP)
			{
				button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? MouseButton::X1 : MouseButton::X2;
			}
			const bool down = message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN
				|| message == WM_MBUTTONDOWN || message == WM_XBUTTONDOWN;

			updateMouseCapture(button, down);
			if (onMouseButton)
			{
				onMouseButton(button, down, currentModifiers());
			}
			// XBUTTON 消息约定:已处理时返回 TRUE
			return (message == WM_XBUTTONDOWN || message == WM_XBUTTONUP) ? TRUE : 0;
		}

		case WM_MOUSEWHEEL:
			if (onMouseWheel)
			{
				onMouseWheel(0, GET_WHEEL_DELTA_WPARAM(wParam)); // 正 = 向上滚
			}
			return 0;

		case WM_MOUSEHWHEEL:
			if (onMouseWheel)
			{
				onMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam), 0); // 正 = 向右滚
			}
			return 0;

		default:
			return ::DefWindowProcW(hwnd, message, wParam, lParam);
		}

	}

	namespace detail
	{
		std::unique_ptr<ZWindow> createBackendWindow(const WindowConfig& config)
		{
			ensureDpiAwareness(); // 幂等:即使未构造 Application 也能获得正确 DPI 行为
			auto window = std::make_unique<ZWindowWin32>(config);
			if (!window->isOpen())
			{
				return nullptr; // 构造失败(窗口类注册 / CreateWindowExW 失败)
			}
			return window;
		}
	}
}