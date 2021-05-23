#pragma once

#include "token.hpp"
#include <fstream>
#include <memory>
#include <string_view>

// Thrown by builtin exit() function to stop execution of the interpreter
struct Exit : public std::runtime_error
{
  Exit(const std::string &message)
      : std::runtime_error(message) {}
};

struct RuntimeError : public std::runtime_error
{
  RuntimeError(Token _token, const std::string &msg)
      : std::runtime_error("Runtime error at: '" + _token.lexeme + "' in line " +
                           std::to_string(_token.line) + ": " + msg),
        token(std::move(_token)) {}

  const Token token;
};

class ErrorHandler
{
public:
  ErrorHandler() = default;
  virtual ~ErrorHandler();
  ErrorHandler &operator=(const ErrorHandler &) = default;
  ErrorHandler &operator=(ErrorHandler &&) = default;
  ErrorHandler(const ErrorHandler &) = default;
  ErrorHandler(ErrorHandler &&) = default;
  virtual void error(const Token &token, std::string_view message);
  virtual void error(unsigned int line, std::string_view message);

  virtual void warn(const Token &token, std::string_view message);
  virtual void warn(unsigned int line, std::string_view message);

  virtual void runtime_error(const Token &token, std::string_view message);
  virtual void runtime_error(unsigned int line, std::string_view message);

  virtual void reset_error();
  virtual bool has_error();

  virtual bool has_runtime_error();

private:
  virtual void report(unsigned int line, std::string_view where,
                      std::string_view message, bool is_error) = 0;
  bool had_error = false;
  bool had_runtime_error = false;
};

class CerrHandler : public ErrorHandler
{
public:
  CerrHandler() = default;

private:
  void report(unsigned int line, std::string_view where,
              std::string_view message, bool is_error) override;
};

class FileErrorHandler : public ErrorHandler
{
public:
  explicit FileErrorHandler(std::string_view filename)
      : err_stream(std::ofstream(filename.data())) {}

private:
  void report(unsigned int line, std::string_view where,
              std::string_view message, bool is_error) override;

private:
  std::ofstream err_stream;
};
