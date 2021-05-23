#pragma once
#include "error.hpp"
#include "expr.hpp"
#include "stmt.hpp"
#include "token.hpp"
#include <exception>
#include <vector>

/// Parse an collection of Token to return an AST representation of it's syntax.
/// This is a recursive descent parser
class Parser
{
public:
  explicit Parser(std::vector<Token> _tokens,
                  std::shared_ptr<ErrorHandler> _err_handler =
                      std::make_shared<CerrHandler>())
      : err_handler(std::move(_err_handler)), tokens(std::move(_tokens)) {}
  bool match(const std::vector<Token::TokenType> &matched_values);
  Token previous();

  std::vector<stmt> parse();

  std::shared_ptr<ErrorHandler> err_handler;

private:
  // Statements
  stmt declaration();
  stmt var_declaration();
  stmt statement();
  stmt function(const std::string &kind);
  // This returns a vector so we can inspect the statements for functions and
  // classes, rather than just evaluating the value
  std::vector<stmt> block();
  stmt for_statement();
  stmt while_statement();
  stmt if_statement();
  stmt print_statement();
  stmt expression_statement();

  // Expressions
  expr expression();
  expr comma_expression();
  expr assignment();
  expr ternary_conditional();
  expr or_expression();
  expr and_expression();
  expr equality();
  expr comparison();
  expr addition();
  expr multiplication();
  expr unary();
  expr call();
  expr primary();

  expr finish_call(expr callee);

  /// Consume the next token if it matches type, else error with message
  Token consume(Token::TokenType type, const std::string &message);

  struct ParseError : std::exception
  {
    const char *what() const noexcept override;
    explicit ParseError(const char *_message) : message(_message) {}
    explicit ParseError(const std::string &_message)
        : message(_message.c_str()) {}

  private:
    const char *message;
  };
  /// Report a syntax error. Returns throwable ParseError for parser recursion
  /// stack unwinding. Error is returned so caller can decide whether to unwind
  ParseError error(const Token &token, const std::string &message);
  /// Consume tokens until the parser is in a synchonized state after an error.
  /// This means until the next statement begins
  void synchronize();

  Token peek();
  bool is_at_end();
  bool check(Token::TokenType type);
  Token advance();

  std::vector<Token> tokens;
  unsigned int current = 0;
};
