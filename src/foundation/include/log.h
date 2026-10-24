#pragma once

#include <cstdint>
#include <format>
#include <source_location>
#include <string_view>
#include <utility>

namespace vwz
{
	enum class LogLevel : std::uint8_t 
	{
		Debug = 0,
		Info = 1,
		Warn = 2,
		Error = 3,
	};

#ifdef NDEBUG
	inline constexpr LogLevel kDefaultLogLevel{ LogLevel::Info };
#else
	inline constexpr LogLevel kDefaultLogLevel{ LogLevel::Debug };
#endif

	namespace log
	{
		void init(LogLevel minLevel = kDefaultLogLevel);

		void setLevel(LogLevel level);
		LogLevel level();

		bool isEnabled(LogLevel level);

		namespace detail
		{
			void write(LogLevel level, const std::source_location& src, std::string_view message);

			template <typename... Args>
			void dispatch(LogLevel level, const std::source_location& src,
				std::format_string<Args...> fmt, Args&&... args)
			{
				if (!isEnabled(level)) 
				{
					return;
				}
				write(level, src, std::format(fmt, std::forward<Args>(args)...));
			}
		}
	}
}

#define VWZ_LOG_DEBUG(...) ::vwz::log::detail::dispatch(::vwz::LogLevel::Debug, ::std::source_location::current(), __VA_ARGS__)
#define VWZ_LOG_INFO(...)  ::vwz::log::detail::dispatch(::vwz::LogLevel::Info,  ::std::source_location::current(), __VA_ARGS__)
#define VWZ_LOG_WARN(...)  ::vwz::log::detail::dispatch(::vwz::LogLevel::Warn,  ::std::source_location::current(), __VA_ARGS__)
#define VWZ_LOG_ERROR(...) ::vwz::log::detail::dispatch(::vwz::LogLevel::Error, ::std::source_location::current(), __VA_ARGS__)