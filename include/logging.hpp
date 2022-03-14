#pragma once

#include <cstring>
#include <iostream>
#include <string>

namespace Logging {
enum class LogLevel {
  DEBUG = 0,
  INFO = 1,
  WARNING = 2,
  ERROR = 3,
};

std::ostream &operator<<(std::ostream &, LogLevel level);

void set_log_level(LogLevel level);

LogLevel get_log_level();

template <typename... Arg>
void log(const std::string &filename, int line, const Arg &...args) {
  std::cout << filename << ":" << line << ": ";

  ((std::cout << args), ...); // Print all variadic args

  std::cout << std::endl;
}

void newline(LogLevel);
} // namespace Logging

// Filename without full path
#define FILENAME_ONLY                                                          \
  (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define LOG_ERROR(...)                                                         \
  [&]() {                                                                      \
    if (static_cast<int>(Logging::LogLevel::ERROR) >=                          \
        static_cast<int>(Logging::get_log_level())) {                          \
      Logging::log(FILENAME_ONLY, __LINE__, __VA_ARGS__);                      \
    }                                                                          \
  }()
#define LOG_WARNING(...)                                                       \
  [&]() {                                                                      \
    if (static_cast<int>(Logging::LogLevel::WARNING) >=                        \
        static_cast<int>(Logging::get_log_level())) {                          \
      Logging::log(FILENAME_ONLY, __LINE__, __VA_ARGS__);                      \
    }                                                                          \
  }()
#define LOG_INFO(...)                                                          \
  [&]() {                                                                      \
    if (static_cast<int>(Logging::LogLevel::INFO) >=                           \
        static_cast<int>(Logging::get_log_level())) {                          \
      Logging::log(FILENAME_ONLY, __LINE__, __VA_ARGS__);                      \
    }                                                                          \
  }()
#define LOG_DEBUG(...)                                                         \
  [&]() {                                                                      \
    if (static_cast<int>(Logging::LogLevel::DEBUG) >=                          \
        static_cast<int>(Logging::get_log_level())) {                          \
      Logging::log(FILENAME_ONLY, __LINE__, __VA_ARGS__);                      \
    }                                                                          \
  }()
