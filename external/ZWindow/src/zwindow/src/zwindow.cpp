#include <zwindow.h>

#if !defined(ZWINDOW_BACKEND_QT) && !defined(ZWINDOW_BACKEND_WIN32)
#error "ZWindow: must be ZWINDOW_BACKEND_QT or ZWINDOW_BACKEND_WIN32 (selected by src/zwindow/CMakeLists.txt)"
#endif

namespace zwin::detail
{
	std::unique_ptr<ZWindow> createBackendWindow(const WindowConfig& config);
}

namespace zwin
{
	std::unique_ptr<ZWindow> ZWindow::Create(const WindowConfig& config)
	{
		return detail::createBackendWindow(config);
	}

	const char* ZWindow::backendName()
	{
#if defined(ZWINDOW_BACKEND_QT)
		return "Qt";
#else
		return "Win32";
#endif
	}
}