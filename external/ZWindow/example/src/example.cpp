#include <zwindow.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

namespace
{
	int parseSmokeFrames(int argc, char** argv)
	{
		for (int i = 1; i + 1 < argc; ++i)
		{
			if (std::strcmp(argv[i], "--smoke") == 0)
			{
				return std::atoi(argv[i + 1]);
			}
		}
		return 0;
	}
}

//   main                 正常交互运行;ESC 或关闭按钮退出
//   main --smoke N       冒烟测试:运行 N 帧后自动退出(供构建验证)
int main(int argc, char** argv)
{
	zwin::Application app(argc, argv);

	const int smokeFrames = parseSmokeFrames(argc, argv);

	auto window = zwin::ZWindow::Create({ .title = "ZWindow Demo" });
	if (window == nullptr)
	{
		std::fprintf(stderr, "[demo] failed to create window\n");
		return 1;
	}

	std::printf("[demo] backend: %s, initial client area: %u x %u\n",
		zwin::ZWindow::backendName(), window->clientWidth(), window->clientHeight());

	window->onResize = [](std::uint32_t width, std::uint32_t height)
		{
			std::printf("[demo] client area: %u x %u\n", width, height);
		};
	window->onClose = []
		{
			std::printf("[demo] close requested\n");
		};
	window->onKey = [&window](zwin::Key key, bool down, bool repeat, zwin::KeyMod mods)
		{
			if (!down)
			{
				return;
			}
			std::printf("[demo] key key=%d repeat=%d mods=0x%X\n",
				static_cast<int>(key), repeat ? 1 : 0, static_cast<unsigned>(mods));
			if (key == zwin::Key::Escape)
			{
				window->requestClose();
			}
		};
	window->onChar = [](char32_t codepoint)
		{
			std::printf("[demo] text: U+%04X\n", static_cast<unsigned>(codepoint));
		};
	window->onMouseButton = [](zwin::MouseButton button, bool down, zwin::KeyMod /*mods*/)
		{
			std::printf("[demo] mouse button %d %s\n", static_cast<int>(button), down ? "down" : "up");
		};
	window->onMouseWheel = [](std::int32_t deltaX, std::int32_t deltaY)
		{
			std::printf("[demo] wheel dx=%d dy=%d\n", deltaX, deltaY);
		};

	if (!window->show())
	{
		std::fprintf(stderr, "[demo] failed to show window\n");
		return 1;
	}

	// 主循环:非阻塞抽干事件;无渲染需求时可改用 waitEvents() 阻塞等待
	int frames = 0;
	while (window->processEvents())
	{
		++frames;
		if (smokeFrames > 0 && frames >= smokeFrames)
		{
			std::printf("[demo] smoke test complete (%d frames), requesting exit\n", frames);
			window->requestClose();
			continue; // 再泵一轮,走完整关闭流程
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}

	std::printf("[demo] window closed, ran %d frames\n", frames);
	return 0;
}
