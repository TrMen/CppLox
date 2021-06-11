#include "logging.hpp"
#include <cassert>

namespace Logging
{
    std::ostream &operator<<(std::ostream &os, LogLevel level)
    {
        switch (level)
        {
        case LogLevel::error:
            return os << "Error";
        case LogLevel::warning:
            return os << "Warning";

        case LogLevel::info:
            return os << "Info";

        case LogLevel::debug:
            return os << "Debug";
        }
        assert(false);
    }

    void newline(LogLevel level)
    {
        if (static_cast<int>(level) >= static_cast<int>(get_log_level()))
            std::cout << std::endl;
    }

    static LogLevel log_level = LogLevel::warning;

    void set_log_level(LogLevel level)
    {
        log_level = level;
    }

    LogLevel get_log_level()
    {
        return log_level;
    }
}