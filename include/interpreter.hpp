#pragma once

#include "environment.hpp"
#include "error.hpp"
#include "expr.hpp"
#include "stmt.hpp"
#include <exception>
#include <vector>

class Parser;

struct Interpreter : public ExprVisitor, public StmtVisitor
{
  explicit Interpreter(std::ostream &_os, std::shared_ptr<ErrorHandler> _err_handler);

  ~Interpreter() override = default;

  /// Interprets a list of statements, representing a program
  void interpret(std::vector<stmt> &statements);

  void execute(const stmt &statement);

  void execute_block(const std::vector<stmt> &body, std::shared_ptr<Environment> enclosing_env);

  std::ostream &out_stream;

  const std::shared_ptr<Environment> globals;

  std::shared_ptr<Environment> environment;

  /// Used to unwind the interpreter execution when functions return
  struct Return
  {
    Token::Value val;
  };

  const std::shared_ptr<ErrorHandler> err_handler;

  Token::Value last_value;

  std::vector<stmt> *ast;

  std::string interpreter_path;

private:
  DECLARE_STMT_VISIT_METHODS

  DECLARE_EXPR_VISIT_METHODS

  Token::Value get_evaluated(const expr &node);
};
