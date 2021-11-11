#pragma once

#include <cstring>
#include <iostream>
#include <string>

namespace Logging
{
    enum class LogLevel
    {
        debug = 0,
        info = 1,
        warning = 2,
        error = 3,
    };

    std::ostream &operator<<(std::ostream &, LogLevel level);

    void set_log_level(LogLevel level);

    LogLevel get_log_level();

    template <typename... Arg>
    void log(LogLevel level, const std::string &filename, int line, const Arg &...args)
    {
        if (static_cast<int>(level) >= static_cast<int>(get_log_level()))
        {
            std::cout << filename << ":" << line << ": ";

            ((std::cout << args), ...); // Print all variadic args

            std::cout << std::endl;
        }
    }

    void newline(LogLevel);
}

// Filename without full path
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define LOG_ERROR(...) Logging::log(Logging::LogLevel::error, __FILENAME__, __LINE__, __VA_ARGS__)
#define LOG_WARNING(...) Logging::log(Logging::LogLevel::warning, __FILENAME__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...) Logging::log(Logging::LogLevel::info, __FILENAME__, __LINE__, __VA_ARGS__)
#define LOG_DEBUG(...) Logging::log(Logging::LogLevel::debug, __FILENAME__, __LINE__, __VA_ARGS__)