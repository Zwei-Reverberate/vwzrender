#include <log.h>
#include <windows.h>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <string>

namespace vwz::log 
{
	namespace 
	{
		std::atomic<LogLevel> g_minLevel{ kDefaultLogLevel };
		std::mutex g_writeMutex;

		bool g_colorEnabled{ false };

		constexpr std::array<const char*, 4> s_levelNames{ "DEBUG", "INFO ", "WARN ", "ERROR" };
		constexpr std::array<const char*, 4> s_levelColors{ "\x1b[90m", "\x1b[36m", "\x1b[33m", "\x1b[31m" };
		constexpr const char* s_colorReset{ "\x1b[0m" };

		std::string_view baseName(std::string_view path)
		{
			const auto pos = path.find_last_of("\\/");
			return (pos == std::string_view::npos) ? path : path.substr(pos + 1);
		}

		std::string formatTimestamp()
		{
			const auto now = std::chrono::system_clock::now();
			const std::time_t seconds = std::chrono::system_clock::to_time_t(now);
			const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
				now.time_since_epoch()) % 1000;

			std::tm local{};
			localtime_s(&local, &seconds);

			return std::format("{:02}:{:02}:{:02}.{:03}",
				local.tm_hour, local.tm_min, local.tm_sec, millis.count());
		}

	}

	void init(LogLevel minLevel)
	{
		g_minLevel.store(minLevel, std::memory_order_relaxed);

		const HANDLE stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
		DWORD mode = 0;
		if (stdoutHandle != INVALID_HANDLE_VALUE && GetConsoleMode(stdoutHandle, &mode)) 
		{
			SetConsoleOutputCP(CP_UTF8);
			SetConsoleMode(stdoutHandle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
			g_colorEnabled = true;
		}
	}

	void setLevel(LogLevel level)
	{
		g_minLevel.store(level, std::memory_order_relaxed);
	}

	LogLevel level()
	{
		return g_minLevel.load(std::memory_order_relaxed);
	}

	bool isEnabled(LogLevel level)
	{
		return level >= g_minLevel.load(std::memory_order_relaxed);
	}

	namespace detail 
	{

		void write(LogLevel level, const std::source_location& src, std::string_view message)
		{
			const auto index = static_cast<std::size_t>(level);

			std::lock_guard lock(g_writeMutex);

			// [HH:MM:SS.mmm] [LEVEL] [file:line] message
			std::string line = std::format("[{}] ", formatTimestamp());
			if (g_colorEnabled) 
			{
				line += s_levelColors[index];
			}
			line += std::format("[{}]", s_levelNames[index]);
			if (g_colorEnabled) 
			{
				line += s_colorReset;
			}
			line += std::format(" [{}:{}] ", baseName(src.file_name()), src.line());
			line += message;
			line += '\n';

			std::fwrite(line.data(), 1, line.size(), stdout);
			std::fflush(stdout);
		}

	}

}
