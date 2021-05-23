#include "error.hpp"
#include <cstdlib>

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

void CerrHandler::report(unsigned int line, std::string_view where,
                         std::string_view message, bool is_error)
{
  std::cerr << "[line " << std::to_string(line);
  std::cerr << (is_error ? "] \033[1;31mError\033[0m"
                         : "] \033[1;33mWarning\033[0m");
  std::cerr << where << ": " << message << "\n";
}

void FileErrorHandler::report(unsigned int line, std::string_view where,
                              std::string_view message, bool is_error)
{
  err_stream << "[line " << std::to_string(line);
  err_stream << (is_error ? "] Error" : "] Warning");
  err_stream << where << ": " << message << "\n";
}
