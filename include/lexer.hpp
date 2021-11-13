#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "error.hpp"
#include "token.hpp"

struct Lexer {
  using Type = Token::TokenType;

  explicit Lexer(std::string _source,
                 std::shared_ptr<ErrorHandler> _err_handler =
                     std::make_shared<CerrHandler>());

  std::vector<Token> lex();

  // clang-format off
  static const std::unordered_map<std::string, Type> keywords;
  // clang-format on

private:
  [[nodiscard]] bool is_at_end() const;
  char advance();
  void add_token(Type t, Token::Value value = NullType{});
  void scan_token();
  bool expect(char expected);
  char peek();
  void string();
  void number();
  [[nodiscard]] char peek_next() const;
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
