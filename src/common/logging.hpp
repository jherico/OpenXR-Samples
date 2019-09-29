//
//  Created by Bradley Austin Davis on 2019/09/18
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <string>
#include <chrono>
#include <ctime>
#include <fmt/format.h>
#include <iostream>

namespace logging {
using Time = std::chrono::time_point<std::chrono::system_clock>;
}

template <>
struct fmt::formatter<logging::Time> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const logging::Time& t, FormatContext& ctx) {
        auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(t);
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now_ms.time_since_epoch()).count() % 1000;
        time_t tt = std::chrono::system_clock::to_time_t(t);
        tm local_tm = *localtime(&tt);
        return format_to(ctx.out(), "{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}.{:03d}", local_tm.tm_year + 1900,
                         local_tm.tm_mon + 1, local_tm.tm_mday, local_tm.tm_hour, local_tm.tm_min, local_tm.tm_sec,
                         duration_ms);
    }
};

namespace logging {

// Values picked to match the OpenXR XrDebugUtilsMessageSeverityFlagBitsEXT values
enum class Level : uint32_t
{
    Debug = 0x00000001,
    Info = 0x00000010,
    Warning = 0x00000100,
    Error = 0x00001000,
};

inline std::string to_string(Level level) {
    uint32_t levelRaw = reinterpret_cast<uint32_t&>(level);
    if (0x00001000 == (levelRaw & 0x00001000)) {
        return "ERROR";
    } else if (0x00000100 == (levelRaw & 0x00000100)) {
        return "WARNING";
    } else if (0x00000010 == (levelRaw & 0x00000010)) {
        return "INFO";
    }
    return "DEBUG";
}

inline void log(Level level, const std::string& message) {
    //auto output = fmt::format("{} {}: {}", std::chrono::system_clock::now(), to_string(level), message);
    auto output = fmt::format("{} {}: {}", std::chrono::system_clock::now(), to_string(level), message);
#ifdef WIN32
    OutputDebugStringA(output.c_str());
    OutputDebugStringA("\n");
#endif
    std::cout << output << std::endl;
}

}  // namespace logging

#define LOG_FORMATTED(level, str, ...) logging::log(level, fmt::format(str, __VA_ARGS__))

#define LOG_DEBUG(str, ...) logging::log(logging::Level::Debug, fmt::format(str, __VA_ARGS__))
#define LOG_INFO(str, ...) logging::log(logging::Level::Info, fmt::format(str, __VA_ARGS__))
#define LOG_WARN(str, ...) logging::log(logging::Level::Warning, fmt::format(str, __VA_ARGS__))
#define LOG_ERROR(str, ...) logging::log(logging::Level::Error, fmt::format(str, __VA_ARGS__))
