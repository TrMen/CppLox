#include "logging.hpp"
#include <cassert>

namespace Logging {
std::ostream &operator<<(std::ostream &os, LogLevel level) {
  switch (level) {
  case LogLevel::ERROR:
    return os << "Error";
  case LogLevel::WARNING:
    return os << "Warning";

  case LogLevel::INFO:
    return os << "Info";

  case LogLevel::DEBUG:
    return os << "Debug";
  }
  std::abort();
}

void newline(LogLevel level) {
  if (static_cast<int>(level) >= static_cast<int>(get_log_level()))
    std::cout << std::endl;
}

static LogLevel log_level = LogLevel::WARNING;

void set_log_level(LogLevel level) { log_level = level; }

LogLevel get_log_level() { return log_level; }

} // namespace Logging
