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
  explicit Interpreter(std::shared_ptr<ErrorHandler> _err_handler =
                           std::make_shared<CerrHandler>(),
                       std::ostream &_os = std::cout);

  ~Interpreter() override = default;

  /// Interprets a list of statements, representing a program
  void interpret(const std::vector<stmt> &statements);

  void execute(const stmt &statement);
  void execute(const shared_stmt &statement);

  Environment environment;

  void execute_block(const std::vector<stmt> &body, Environment block_env);
  void execute_block(const std::vector<shared_stmt> &body, Environment block_env);

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
  void visit([[maybe_unused]] EmptyStmt &visitable) override;
  void visit(FunctionStmt &visitable) override;

  // Expressions
  void visit(Assign &visitable) override;
  void visit(Logical &visitable) override;
  void visit(Variable &visitable) override;
  void visit([[maybe_unused]] Empty &visitable) override;
  void visit(Literal &visitable) override;
  void visit(Unary &visitable) override;
  void visit(Binary &visitable) override;
  void visit(Ternary &visitable) override;
  void visit(Malformed &visitable) override;
  void visit(Call &visitable) override;
  void visit(Grouping &visitable) override;

  Token::literal_t get_evaluated_literal(const expr &node);

  Token::literal_t last_value;
  RuntimeError error(const Token &token, const std::string &message);
  std::shared_ptr<ErrorHandler> err_handler;

  /// RAII class that manages an environment to execute a block in.
  /// Creating it will execute the block of statements with correct scope
  class BlockExecutor
  {
  public:
    BlockExecutor(Interpreter &_interpreter,
                  Environment env);
    BlockExecutor(Interpreter &_interpreter,
                  const std::vector<shared_stmt> &statements, Environment env);
    void execute(const std::vector<stmt> &statements);
    void execute(const std::vector<shared_stmt> &statements);
    ~BlockExecutor();

  private:
    Interpreter &interpreter;
    Environment original_env;
  };
};
