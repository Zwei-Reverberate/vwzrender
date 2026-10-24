#include <zwindow.h>
#include <qcoreapplication.h>
#include <qevent.h>
#include <qeventloop.h>
#include <qguiapplication.h>
#include <qscreen.h>
#include <qstring.h>
#include <qwindow.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string_view>

#if defined(Q_OS_WIN)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h> // 仅用于 GetModuleHandleW(nativeWindow().instance)
#endif

namespace
{
	// 匿名命名空间位于全局作用域:显式引入 zwin 中的类型供下方辅助函数使用
	using zwin::Key;
	using zwin::KeyMod;
	using zwin::MouseButton;

	std::unique_ptr<QGuiApplication> s_qtApplication; // 由 zwin::Application 创建(保证唯一)

	// Qt::Key -> 逻辑键(未覆盖的键映射为 Unknown)
	Key toKey(int qtKey)
	{
		switch (qtKey)
		{
		case Qt::Key_A: return Key::A;
		case Qt::Key_B: return Key::B;
		case Qt::Key_C: return Key::C;
		case Qt::Key_D: return Key::D;
		case Qt::Key_E: return Key::E;
		case Qt::Key_F: return Key::F;
		case Qt::Key_G: return Key::G;
		case Qt::Key_H: return Key::H;
		case Qt::Key_I: return Key::I;
		case Qt::Key_J: return Key::J;
		case Qt::Key_K: return Key::K;
		case Qt::Key_L: return Key::L;
		case Qt::Key_M: return Key::M;
		case Qt::Key_N: return Key::N;
		case Qt::Key_O: return Key::O;
		case Qt::Key_P: return Key::P;
		case Qt::Key_Q: return Key::Q;
		case Qt::Key_R: return Key::R;
		case Qt::Key_S: return Key::S;
		case Qt::Key_T: return Key::T;
		case Qt::Key_U: return Key::U;
		case Qt::Key_V: return Key::V;
		case Qt::Key_W: return Key::W;
		case Qt::Key_X: return Key::X;
		case Qt::Key_Y: return Key::Y;
		case Qt::Key_Z: return Key::Z;
		case Qt::Key_0: return Key::Num0;
		case Qt::Key_1: return Key::Num1;
		case Qt::Key_2: return Key::Num2;
		case Qt::Key_3: return Key::Num3;
		case Qt::Key_4: return Key::Num4;
		case Qt::Key_5: return Key::Num5;
		case Qt::Key_6: return Key::Num6;
		case Qt::Key_7: return Key::Num7;
		case Qt::Key_8: return Key::Num8;
		case Qt::Key_9: return Key::Num9;
		case Qt::Key_F1: return Key::F1;
		case Qt::Key_F2: return Key::F2;
		case Qt::Key_F3: return Key::F3;
		case Qt::Key_F4: return Key::F4;
		case Qt::Key_F5: return Key::F5;
		case Qt::Key_F6: return Key::F6;
		case Qt::Key_F7: return Key::F7;
		case Qt::Key_F8: return Key::F8;
		case Qt::Key_F9: return Key::F9;
		case Qt::Key_F10: return Key::F10;
		case Qt::Key_F11: return Key::F11;
		case Qt::Key_F12: return Key::F12;
		case Qt::Key_Escape: return Key::Escape;
		case Qt::Key_Return:
		case Qt::Key_Enter: return Key::Enter;
		case Qt::Key_Tab: return Key::Tab;
		case Qt::Key_Backspace: return Key::Backspace;
		case Qt::Key_Space: return Key::Space;
		case Qt::Key_Delete: return Key::Delete;
		case Qt::Key_Insert: return Key::Insert;
		case Qt::Key_Left: return Key::Left;
		case Qt::Key_Right: return Key::Right;
		case Qt::Key_Up: return Key::Up;
		case Qt::Key_Down: return Key::Down;
		case Qt::Key_Home: return Key::Home;
		case Qt::Key_End: return Key::End;
		case Qt::Key_PageUp: return Key::PageUp;
		case Qt::Key_PageDown: return Key::PageDown;
		case Qt::Key_Minus: return Key::Minus;
		case Qt::Key_Equal: return Key::Equal;
		case Qt::Key_Comma: return Key::Comma;
		case Qt::Key_Period: return Key::Period;
		case Qt::Key_Slash: return Key::Slash;
		case Qt::Key_Semicolon: return Key::Semicolon;
		case Qt::Key_Apostrophe: return Key::Apostrophe;
		case Qt::Key_QuoteLeft: return Key::Grave;
		case Qt::Key_BracketLeft: return Key::BracketLeft;
		case Qt::Key_BracketRight: return Key::BracketRight;
		case Qt::Key_Backslash: return Key::Backslash;
		case Qt::Key_Shift: return Key::Shift;
		case Qt::Key_Control: return Key::Ctrl;
		case Qt::Key_Alt: return Key::Alt;
		case Qt::Key_Meta: return Key::Super;
		default: return Key::Unknown;
		}
	}

	KeyMod toKeyMod(Qt::KeyboardModifiers modifiers)
	{
		KeyMod mods = KeyMod::None;
		if (modifiers.testFlag(Qt::ShiftModifier))
		{
			mods = mods | KeyMod::Shift;
		}
		if (modifiers.testFlag(Qt::ControlModifier))
		{
			mods = mods | KeyMod::Ctrl;
		}
		if (modifiers.testFlag(Qt::AltModifier))
		{
			mods = mods | KeyMod::Alt;
		}
		if (modifiers.testFlag(Qt::MetaModifier))
		{
			mods = mods | KeyMod::Super;
		}
		return mods;
	}

	bool toMouseButton(Qt::MouseButton qtButton, MouseButton& out)
	{
		switch (qtButton)
		{
		case Qt::LeftButton: out = MouseButton::Left; return true;
		case Qt::RightButton: out = MouseButton::Right; return true;
		case Qt::MiddleButton: out = MouseButton::Middle; return true;
		case Qt::XButton1: out = MouseButton::X1; return true;
		case Qt::XButton2: out = MouseButton::X2; return true;
		default: return false;
		}
	}

	// 客户区坐标:Qt6 用 position(),Qt5 用 localPos()
	QPointF mouseLocalPosition(const QMouseEvent* event)
	{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		return event->position();
#else
		return event->localPos();
#endif
	}

} // namespace

namespace zwin
{

	// 一个 QWindow + 事件分发;线程约束:所有方法与回调都必须在创建窗口的线程上调用
	// (Qt 事件循环为线程私有)。
	class ZWindowQt final : public QWindow, public ZWindow
	{
	public:
		explicit ZWindowQt(const WindowConfig& config);
		~ZWindowQt() override = default;

		bool show() override;
		void requestClose() override;
		bool isOpen() const override;
		bool processEvents() override;
		void waitEvents() override;
		void setTitle(std::string_view title) override;
		NativeWindow nativeWindow() const override;

	protected:
		void resizeEvent(QResizeEvent* event) override;
		void keyPressEvent(QKeyEvent* event) override;
		void keyReleaseEvent(QKeyEvent* event) override;
		void mouseMoveEvent(QMouseEvent* event) override;
		void mousePressEvent(QMouseEvent* event) override;
		void mouseReleaseEvent(QMouseEvent* event) override;
		void wheelEvent(QWheelEvent* event) override;
		bool event(QEvent* event) override; // 拦截 Close:转入统一关闭流程

	private:
		bool m_fullscreen = false;
	};

	Application::Application(int argc, char** argv)
	{
		// 已存在 QGuiApplication(例如宿主本就是 Qt 应用)则复用
		if (QGuiApplication::instance() == nullptr)
		{
			s_qtApplication = std::make_unique<QGuiApplication>(argc, argv);
		}
	}

	Application::~Application()
	{
		s_qtApplication.reset();
	}

	ZWindowQt::ZWindowQt(const WindowConfig& config)
		: m_fullscreen(config.fullscreen)
	{
		// 注意:必须显式限定 QWindow::setTitle,否则解析到派生类覆盖的
		// setTitle(std::string_view)(它隐藏了基类的 setTitle(QString))
		QWindow::setTitle(QString::fromUtf8(config.title.data(), static_cast<int>(config.title.size())));

		// 对外尺寸统一为物理像素(与 Win32 后端一致):WindowConfig 的 width/height 就是
		// 客户区的物理像素数,而 QWindow::resize() 收的是逻辑像素,所以先按窗口所在屏幕的
		// 缩放比换算。此时窗口还没 show(),QWindow::devicePixelRatio() 尚不可靠,故取
		// screen() 的缩放比。screen() 拿不到时退回主屏幕(createBackendWindow 已确保存在)。
		const QScreen* targetScreen = screen();
		if (targetScreen == nullptr)
		{
			targetScreen = QGuiApplication::primaryScreen();
		}
		const qreal ratio = (targetScreen != nullptr && targetScreen->devicePixelRatio() > 0.0)
			? targetScreen->devicePixelRatio()
			: 1.0;
		const QSize logicalSize(
			std::max(1, static_cast<int>(std::lround(static_cast<double>(config.width) / ratio))),
			std::max(1, static_cast<int>(std::lround(static_cast<double>(config.height) / ratio))));

		resize(logicalSize);

		m_width = config.width; // 物理像素初值;后续 resizeEvent 会用真实值覆盖
		m_height = config.height;

		if (!config.resizable)
		{
			// 去掉最大化能力并锁定尺寸(与 Win32 后端去除 WS_THICKFRAME 语义一致);
			// 这里的 min/max 与 resize() 一样是逻辑像素,必须用换算后的尺寸
			setFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
			setMinimumSize(logicalSize);
			setMaximumSize(logicalSize);
		}

	}

	bool ZWindowQt::show()
	{
		// 与 Win32 后端一致:已经请求过关闭就不再显示
		if (m_shouldClose)
		{
			return false;
		}
		if (m_fullscreen)
		{
			QWindow::showFullScreen();
		}
		else
		{
			QWindow::show();
		}
		// show() 之后平台窗口应当已经建立;句柄缺失或窗口不可见都算显示失败。
		// 注意 handle() 在 show() 之前必然为空,所以这个检查只能放在 show() 之后。
		return handle() != nullptr && QWindow::isVisible();
	}

	void ZWindowQt::requestClose()
	{
		// 触发 QEvent::Close,经 event() 转入统一关闭流程
		QWindow::close();
	}

	bool ZWindowQt::isOpen() const
	{
		return !m_shouldClose;
	}

	bool ZWindowQt::processEvents()
	{
		// 非阻塞抽干事件队列(与 Win32 后端语义一致)
		QCoreApplication::processEvents(QEventLoop::AllEvents);
		return isOpen();
	}

	void ZWindowQt::waitEvents()
	{
		if (!isOpen())
		{
			return;
		}
		// 阻塞休眠至新事件到达,不消耗 CPU
		QCoreApplication::processEvents(QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents);
	}

	void ZWindowQt::setTitle(std::string_view title)
	{
		QWindow::setTitle(QString::fromUtf8(title.data(), static_cast<int>(title.size())));
	}

	NativeWindow ZWindowQt::nativeWindow() const
	{
		NativeWindow native;
		// Windows 下 winId() 即 HWND(Qt5 为句柄类型 / Qt6 为 quintptr,统一转 void*)
		native.window = reinterpret_cast<void*>(winId());
#if defined(Q_OS_WIN)
		native.instance = ::GetModuleHandleW(nullptr);
#endif
		return native;
	}

	void ZWindowQt::resizeEvent(QResizeEvent* event)
	{
		// 对外尺寸统一为物理像素(与 Win32 后端一致);Qt 的 size() 为逻辑像素
		const qreal ratio = devicePixelRatio();
		m_width = static_cast<std::uint32_t>(std::lround(event->size().width() * ratio));
		m_height = static_cast<std::uint32_t>(std::lround(event->size().height() * ratio));
		if (onResize)
		{
			onResize(m_width, m_height);
		}
		QWindow::resizeEvent(event);
	}

	void ZWindowQt::keyPressEvent(QKeyEvent* event)
	{
		if (onKey)
		{
			onKey(toKey(event->key()), true, event->isAutoRepeat(), toKeyMod(event->modifiers()));
		}
		// 文本输入:text() 可能包含多个码点(如 emoji 代理对),逐个上报完整码点
		if (onChar)
		{
			const QString text = event->text();
			for (const unsigned int codepoint : text.toUcs4())
			{
				onChar(static_cast<char32_t>(codepoint));
			}
		}
		QWindow::keyPressEvent(event);
	}

	void ZWindowQt::keyReleaseEvent(QKeyEvent* event)
	{
		if (onKey)
		{
			onKey(toKey(event->key()), false, false, toKeyMod(event->modifiers()));
		}
		QWindow::keyReleaseEvent(event);
	}

	void ZWindowQt::mouseMoveEvent(QMouseEvent* event)
	{
		if (onMouseMove)
		{
			// 对外坐标统一为物理像素(与 Win32 后端一致)
			const QPointF position = mouseLocalPosition(event) * devicePixelRatio();
			onMouseMove(static_cast<std::int32_t>(std::lround(position.x())),
				static_cast<std::int32_t>(std::lround(position.y())),
				toKeyMod(event->modifiers()));
		}
		QWindow::mouseMoveEvent(event);
	}

	void ZWindowQt::mousePressEvent(QMouseEvent* event)
	{
		MouseButton button = MouseButton::Left;
		if (toMouseButton(event->button(), button) && onMouseButton)
		{
			onMouseButton(button, true, toKeyMod(event->modifiers()));
		}
		QWindow::mousePressEvent(event);
	}

	void ZWindowQt::mouseReleaseEvent(QMouseEvent* event)
	{
		MouseButton button = MouseButton::Left;
		if (toMouseButton(event->button(), button) && onMouseButton)
		{
			onMouseButton(button, false, toKeyMod(event->modifiers()));
		}
		QWindow::mouseReleaseEvent(event);
	}

	void ZWindowQt::wheelEvent(QWheelEvent* event)
	{
		if (onMouseWheel)
		{
			// angleDelta 是 120 单位/格的刻度,与 Win32 后端约定一致
			const QPoint delta = event->angleDelta();
			onMouseWheel(static_cast<std::int32_t>(delta.x()), static_cast<std::int32_t>(delta.y()));
		}
		QWindow::wheelEvent(event);
	}

	bool ZWindowQt::event(QEvent* event)
	{
		if (event->type() == QEvent::Close)
		{
			// 延迟销毁(与 Win32 后端一致):只置标记并通知,不在此释放对象;
			// 平台窗口由 Qt 关闭,ZWindow 对象留存至析构
			if (!m_shouldClose)
			{
				m_shouldClose = true;
				if (onClose)
				{
					onClose();
				}
			}
		}
		return QWindow::event(event);
	}

	namespace detail
	{

		std::unique_ptr<ZWindow> createBackendWindow(const WindowConfig& config)
		{
			if (QGuiApplication::instance() == nullptr)
			{
				std::fprintf(stderr,
					"[zwin] cannot create window: construct zwin::Application first (it initializes QGuiApplication)\n");
				return nullptr;
			}
			// 没有可用屏幕(例如无头会话)时窗口无从谈起。语义与 Win32 的
			// CreateWindowExW 失败一致:返回 nullptr,而不是给出一个永远不显示的窗口。
			if (QGuiApplication::primaryScreen() == nullptr)
			{
				std::fprintf(stderr, "[zwin] cannot create window: no screen available\n");
				return nullptr;
			}

			auto window = std::make_unique<ZWindowQt>(config);
			std::fprintf(stderr, "[zwin] window created (client %ux%u, backend Qt)\n",
				window->clientWidth(), window->clientHeight());
			return window;

		}

	}
}
