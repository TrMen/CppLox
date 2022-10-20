#pragma once

#include "environment.hpp"

#include <vector>

#include "class.hpp"
#include "error.hpp"
#include "expr.hpp"
#include "stmt.hpp"

struct Parser;

struct Interpreter : public ExprVisitor, public StmtVisitor {
  explicit Interpreter(std::ostream &_os,
                       std::shared_ptr<ErrorHandler> _err_handler);

  Interpreter(const Interpreter &) = delete;
  Interpreter(Interpreter &&) noexcept = delete;
  Interpreter &operator=(const Interpreter &) = delete;
  Interpreter &operator=(Interpreter &&) noexcept = delete;
  ~Interpreter() override = default;

  /// Interprets a list of statements, representing a program
  void interpret(std::vector<stmt> &statements);

  void execute(const stmt &statement);

  void execute_block(const std::vector<stmt> &body,
                     std::shared_ptr<Environment> enclosing_env);

  std::ostream &out_stream;

  const std::shared_ptr<Environment> globals;

  std::shared_ptr<Environment> environment;

  /// Used to unwind the interpreter execution when functions return
  struct Return : std::exception {
    explicit Return(Token::Value _val) : val(std::move(_val)) {}
    Token::Value val;
  };

  const std::shared_ptr<ErrorHandler> err_handler;

  Token::Value last_value;

  std::string interpreter_path;

  struct CheckedRecursiveDepth {
    CheckedRecursiveDepth(Interpreter &, const Token &location);
    ~CheckedRecursiveDepth();

    CheckedRecursiveDepth(const CheckedRecursiveDepth &) = delete;
    CheckedRecursiveDepth operator=(const CheckedRecursiveDepth &) = delete;
    CheckedRecursiveDepth(CheckedRecursiveDepth &&) = delete;
    CheckedRecursiveDepth operator=(CheckedRecursiveDepth &&) = delete;

    Interpreter &interpreter;

    static constexpr size_t MAX_RECURSION_DEPTH = 1000;
  };

private:
  DECLARE_STMT_VISIT_METHODS

  DECLARE_EXPR_VISIT_METHODS

  size_t recursion_depth = 0;

  Token::Value get_evaluated(const expr &expression);
  Token::Value get_evaluated(Expr &expression);

  [[nodiscard]] Class::ClassFunctions split_class_functions(
      const std::vector<FunctionStmtPtr> &class_functions) const;

  [[nodiscard]] const Token::Value &lookup_variable(const Token &name,
                                                    const Expr &) const;
};
