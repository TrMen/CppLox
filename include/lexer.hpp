#pragma once

#include "error.hpp"
#include "token.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

class Lexer
{
public:
  using Type = Token::TokenType;

  explicit Lexer(std::string _source,
                 std::shared_ptr<ErrorHandler> _err_handler =
                     std::make_shared<CerrHandler>())
      : source(std::move(_source)),
        err_handler(std::move(_err_handler))
  {
    tokens.reserve(source.size() / 3);
  }

  std::vector<Token> lex();

  // clang-format off
  std::unordered_map<std::string, Type> keywords{
      {"and", Type::AND}, {"class", Type::CLASS}, {"else", Type::ELSE}, {"false", Type::FALSE}, 
      {"for", Type::FOR}, {"fun", Type::FUN}, {"fn", Type::FUN}, {"if", Type::IF}, {"nil", Type::NIL}, {"or", Type::OR}, 
      {"print", Type::PRINT}, {"return", Type::RETURN}, {"super", Type::SUPER}, {"this", Type::THIS}, 
      {"true", Type::TRUE}, {"var", Type::VAR}, {"while", Type::WHILE}, {"let", Type::VAR}};
  // clang-format on

private:
  bool is_at_end();
  char advance();
  void add_token(Type t, Token::literal_t literal = NullType());
  void scan_token();
  bool expect(char expected);
  char peek();
  void string();
  void number();
  char peek_next();
  void identifier();
  void slash_or_comment();

  std::string source;
  std::vector<Token> tokens;
  unsigned int start = 0;
  unsigned int current = 0;
  unsigned int line = 1;

  // Syntax error handling
  std::shared_ptr<ErrorHandler> err_handler;
  unsigned int syntax_error_start_line = 0;
  bool last_character_expected = true;
  std::string last_syntax_error;

  void report_last_syntax_error();
};
