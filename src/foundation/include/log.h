#pragma once

#include <cstdint>
#include <cctype>
#include <format>
#include <memory>
#include <source_location>
#include <string_view>
#include <utility>

#include <spdlog/spdlog.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#if defined(_WIN32)
#	ifndef WIN32_LEAN_AND_MEAN
#		define WIN32_LEAN_AND_MEAN
#	endif
#	ifndef NOMINMAX
#		define NOMINMAX
#	endif
#	include <windows.h>
#endif

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
		namespace detail
		{
			inline spdlog::level::level_enum toSpdlogLevel(LogLevel level) noexcept
			{
				switch (level)
				{
				case LogLevel::Debug: return spdlog::level::debug;
				case LogLevel::Info:  return spdlog::level::info;
				case LogLevel::Warn:  return spdlog::level::warn;
				case LogLevel::Error: return spdlog::level::err;
				}
				return spdlog::level::info;
			}

			inline LogLevel fromSpdlogLevel(spdlog::level::level_enum level) noexcept
			{
				switch (level)
				{
				case spdlog::level::trace:
				case spdlog::level::debug:    return LogLevel::Debug;
				case spdlog::level::info:     return LogLevel::Info;
				case spdlog::level::warn:     return LogLevel::Warn;
				case spdlog::level::err:
				case spdlog::level::critical: return LogLevel::Error;
				default:                      return LogLevel::Info;
				}
			}

			class UpperLevelFlag final : public spdlog::custom_flag_formatter
			{
			public:
				void format(const spdlog::details::log_msg& msg, const std::tm&,
					spdlog::memory_buf_t& dest) override
				{
					const auto name = spdlog::level::to_string_view(msg.level);
					const std::size_t count = (name.size() < 5) ? name.size() : 5;

					char buffer[5] = { ' ', ' ', ' ', ' ', ' ' };
					for (std::size_t i = 0; i < count; ++i) 
					{
						buffer[i] = static_cast<char>(
							std::toupper(static_cast<unsigned char>(name[i])));
					}
					dest.append(buffer, buffer + 5);
				}

				std::unique_ptr<spdlog::custom_flag_formatter> clone() const override
				{
					return std::make_unique<UpperLevelFlag>();
				}
			};

			inline void setup(LogLevel minLevel)
			{
#if defined(_WIN32)
				::SetConsoleOutputCP(CP_UTF8);
#endif
				auto logger = spdlog::get("vwz");
				if (!logger) 
				{
					logger = spdlog::stdout_color_mt("vwz");
				}

				auto formatter = std::make_unique<spdlog::pattern_formatter>();
				// [HH:MM:SS.mmm] [LEVEL] [file:line] message
				formatter->add_flag<UpperLevelFlag>('L')
					.set_pattern("[%H:%M:%S.%e] [%^%L%$] [%s:%#] %v");
				logger->set_formatter(std::move(formatter));

				spdlog::set_default_logger(std::move(logger));
				spdlog::set_level(toSpdlogLevel(minLevel));
				spdlog::flush_on(spdlog::level::info);
			}
		}

		inline void init(LogLevel minLevel = kDefaultLogLevel)
		{
			detail::setup(minLevel);
		}

		inline void setLevel(LogLevel level)
		{
			spdlog::set_level(detail::toSpdlogLevel(level));
		}

		inline LogLevel level()
		{
			return detail::fromSpdlogLevel(spdlog::default_logger()->level());
		}

		inline bool isEnabled(LogLevel level)
		{
			return spdlog::default_logger()->should_log(detail::toSpdlogLevel(level));
		}

		namespace detail
		{
			inline void write(LogLevel level, const std::source_location& src, const std::string& message)
			{
				spdlog::default_logger()->log(
					spdlog::source_loc{ src.file_name(),
						static_cast<int>(src.line()), src.function_name() },
					toSpdlogLevel(level), "{}", message);
			}

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
