#pragma once

#include "environment.hpp"
#include "error.hpp"
#include "expr.hpp"
#include "stmt.hpp"
#include <exception>
#include <vector>

class Parser;

struct Interpreter : public Visitor_t, public StmtVisitor
{
  explicit Interpreter(std::ostream &_os, std::shared_ptr<ErrorHandler> _err_handler);

  ~Interpreter() override = default;

  /// Interprets a list of statements, representing a program
  void interpret(const std::vector<stmt> &statements);

  void execute(const stmt &statement);

  Environment environment;

  void execute_block(const std::vector<stmt> &body, Environment block_env);

  std::ostream &out_stream;

private:
  // Statements
  void visit(Var &visitable) override;
  void visit(MalformedStmt &visitable) override;
  void visit(Block &visitable) override;
  void visit(Print &visitable) override;
  void visit(StmtExpr &visitable) override;
  void visit(IfStmt &visitable) override;
  void visit(WhileStmt &visitable) override;
  void visit(EmptyStmt &visitable) override;
  void visit(FunctionStmt &visitable) override;

  // Expressions
  void visit(Assign &visitable) override;
  void visit(Logical &visitable) override;
  void visit(Variable &visitable) override;
  void visit(Empty &visitable) override;
  void visit(Literal &visitable) override;
  void visit(Unary &visitable) override;
  void visit(Binary &visitable) override;
  void visit(Ternary &visitable) override;
  void visit(Malformed &visitable) override;
  void visit(Call &visitable) override;
  void visit(Grouping &visitable) override;

  Token::Value get_evaluated(const expr &node);

  Token::Value last_value;
  const std::shared_ptr<ErrorHandler> err_handler;
};
