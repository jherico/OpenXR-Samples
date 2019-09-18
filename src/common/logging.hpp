#pragma once

#include <string>
#include <fmt/format.h>
#include <openxr/openxr.h>

namespace logging {

enum class Level : uint64_t
{
    Debug = XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT,
    Info = XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT,
    Warning = XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
    Error = XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
};

inline void log(Level level, const std::string& message) {
#ifdef WIN32
    OutputDebugStringA(message.c_str());
    OutputDebugStringA("\n");
#endif
    std::cout << message << std::endl;
}

}  // namespace logging

#define LOG_FORMATTED(level, str, ...) logging::log(level, fmt::format(str, __VA_ARGS__))

#define LOG_DEBUG(str, ...) logging::log(logging::Level::Debug, fmt::format(str, __VA_ARGS__))
#define LOG_INFO(str, ...) logging::log(logging::Level::Info, fmt::format(str, __VA_ARGS__))
#define LOG_WARN(str, ...) logging::log(logging::Level::Warning, fmt::format(str, __VA_ARGS__))
#define LOG_ERROR(str, ...) logging::log(logging::Level::Error, fmt::format(str, __VA_ARGS__))
