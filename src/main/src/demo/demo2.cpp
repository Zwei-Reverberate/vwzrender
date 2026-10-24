#include <demo/demo2.h>
#include <log.h>

const char* demo::mouseButtonName(zwin::MouseButton button)
{
	switch (button) 
	{
	case zwin::MouseButton::Left:
		return "左";
	case zwin::MouseButton::Right:
		return "右";
	case zwin::MouseButton::Middle:
		return "中";
	case zwin::MouseButton::X1:
		return "侧1";
	case zwin::MouseButton::X2:
		return "侧2";
	}
	return "?";
}

std::unique_ptr<zwin::ZWindow> demo::createDemoWindow(const char* title)
{
	auto window = zwin::ZWindow::Create
	({
		.title = title,
		.width = 1920,
		.height = 1080,
	});
	if (window == nullptr) 
	{
		VWZ_LOG_ERROR("窗口创建失败");
		return nullptr;
	}

	window->onClose = [] 
	{
		VWZ_LOG_INFO("[事件] 关闭请求(WM_CLOSE)——延迟销毁:退出主循环后再销毁窗口");
	};

	window->onResize = [](std::uint32_t width, std::uint32_t height) 
	{
		VWZ_LOG_INFO("[事件] 尺寸变化: {}x{}", width, height);
	};

	zwin::ZWindow* const self = window.get();
	window->onKey = [self](zwin::Key key, bool down, bool repeat, zwin::KeyMod) 
	{
		if (down && !repeat && key == zwin::Key::Escape) 
		{
			VWZ_LOG_INFO("[事件] ESC 按下,请求关闭");
			self->requestClose();
			return;
		}
		VWZ_LOG_INFO("[事件] 按键 {}: Key=0x{:02X}{}", down ? "按下" : "抬起",
			static_cast<std::uint16_t>(key), repeat ? "(按住重复)" : "");
	};

	window->onChar = [](char32_t codepoint) 
	{
		if (codepoint < 128) 
		{
			VWZ_LOG_INFO("[事件] 字符输入: U+{:04X} ('{}')",
				static_cast<std::uint32_t>(codepoint), static_cast<char>(codepoint));
		}
		else 
		{
			VWZ_LOG_INFO("[事件] 字符输入: U+{:04X}", static_cast<std::uint32_t>(codepoint));
		}
	};

	window->onMouseButton = [](zwin::MouseButton button, bool down, zwin::KeyMod) 
	{
		VWZ_LOG_INFO("[事件] 鼠标{}键 {}", mouseButtonName(button), down ? "按下" : "抬起");
	};

	// 鼠标移动频率很高:节流记录(每 32 次记一条),证明事件在持续到达
	window->onMouseMove = [moveCounter = std::uint32_t{ 0 }](std::int32_t x, std::int32_t y, zwin::KeyMod ) mutable 
	{
		if (++moveCounter % 32 == 1) 
		{
			VWZ_LOG_INFO("[事件] 鼠标移动: ({}, {})  [累计 {} 次]", x, y, moveCounter);
		}
	};

	window->onMouseWheel = [](std::int32_t deltaX, std::int32_t deltaY) 
	{
		VWZ_LOG_INFO("[事件] 滚轮: dx={} dy={}", deltaX, deltaY);
	};

	if (!window->show()) 
	{
		VWZ_LOG_ERROR("窗口显示失败");
		return nullptr;
	}

	return window;
}



void demo::Demo2::runImpl()
{
	zwin::Application app{0, nullptr};

	auto window = createDemoWindow("ZWindow — 第 2 课:Win32 原生窗口");
	if (window == nullptr) 
	{
		return;
	}

	VWZ_LOG_INFO("进入事件循环(点击标题栏 X 或按 ESC 退出;拖动边框观察尺寸事件)");

	std::uint32_t rounds = 0;
	while (window->processEvents()) 
	{
		++rounds;
		window->waitEvents();
	}

	VWZ_LOG_INFO("主循环退出(共 {} 轮消息),销毁窗口", rounds);
}