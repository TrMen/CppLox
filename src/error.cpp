#include "error.hpp"
#include <cstdlib>

Exit::Exit(const std::string &msg) : std::runtime_error(msg) {}

RuntimeError::RuntimeError(Token _token, const std::string &msg)
    : std::runtime_error("Runtime error at: '" + _token.lexeme + "' in line " +
                         std::to_string(_token.line) + ": " + msg),
      token(std::move(_token)) {}

RuntimeError::RuntimeError(Token::Value value, const std::string &msg, unsigned int line)
    : std::runtime_error("Runtime error at: '" + stringify(value) + "' in line " +
                         std::to_string(line) + ": " + msg),
      token(Token{Token::TokenType::NIL, "RUNTIME_ERROR", value, line}) {}

CompiletimeError::CompiletimeError(Token _token, const std::string &msg)
    : std::runtime_error("Compile-time error at: '" + _token.lexeme + "' in line " +
                         std::to_string(_token.line) + ": " + msg),
      token(std::move(_token)) {}

CompiletimeError::CompiletimeError(Token::Value value, const std::string &msg, unsigned int line)
    : std::runtime_error("Compile-time error at: '" + stringify(value) + "' in line " +
                         std::to_string(line) + ": " + msg),
      token(Token{Token::TokenType::NIL, "COMPILETIME_ERROR", value, line}) {}

//--------------------Error-handler---------------------------------------
ErrorHandler::~ErrorHandler() = default;

void ErrorHandler::reset_error() { had_error = false; }
bool ErrorHandler::has_error() { return had_error; }

bool ErrorHandler::has_runtime_error() { return had_runtime_error; }

void ErrorHandler::error(unsigned int line, std::string_view message)
{
  report(line, "", message, true);
  had_error = true;
}

void ErrorHandler::error(const Token &token, std::string_view message)
{
  if (token.type == Token::TokenType::_EOF)
  {
    report(token.line, " at end", message, true);
  }
  else
  {
    report(token.line, " at '" + token.lexeme + "'", message, true);
  }
  had_error = true;
}

void ErrorHandler::warn(const Token &token, std::string_view message)
{
  if (token.type == Token::TokenType::_EOF)
  {
    report(token.line, " at end", message, false);
  }
  else
  {
    report(token.line, " at '" + token.lexeme + "'", message, false);
  }
}

void ErrorHandler::warn(unsigned int line, std::string_view message)
{
  report(line, "", message, false);
}

void ErrorHandler::runtime_error(const Token &token, std::string_view message)
{
  report(token.line, "", message, true);
  had_runtime_error = true;
}

void ErrorHandler::runtime_error(unsigned int line, std::string_view message)
{
  report(line, "", message, true);
  had_runtime_error = true;
}

//---------------Derived classes-------------------------------------

CerrHandler::CerrHandler() = default;

void CerrHandler::report(unsigned int line, std::string_view where,
                         std::string_view message, bool is_error)
{
  std::cerr << "[line " << std::to_string(line);
  std::cerr << (is_error ? "] \033[1;31mError\033[0m"
                         : "] \033[1;33mWarning\033[0m");
  std::cerr << where << ": " << message << "\n";
}

FileErrorHandler::FileErrorHandler(std::string_view filename) : err_stream(std::ofstream(filename.data())) {}

void FileErrorHandler::report(unsigned int line, std::string_view where,
                              std::string_view message, bool is_error)
{
  err_stream << "[line " << std::to_string(line);
  err_stream << (is_error ? "] Error" : "] Warning");
  err_stream << where << ": " << message << "\n";
}
