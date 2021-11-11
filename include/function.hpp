#pragma once

#include <vector>

#include "callable.hpp"
#include "environment.hpp"
#include "stmt.hpp"

struct Function : public Callable
{
  Function(const std::variant<const FunctionStmt *, const Lambda *> &declaration,
           std::shared_ptr<Environment> closure, FunctionKind kind);

  Token::Value call(Interpreter &interpreter, const std::vector<Token::Value> &arguments) override;

  [[nodiscard]] size_t arity() const override;
  [[nodiscard]] std::string to_string() const override;

  [[nodiscard]] const std::vector<Token> &parameters() const;
  [[nodiscard]] const std::vector<stmt> &body() const;

  /* Create a bound method fron this function. A bound method is a method that is identical in AST
  * but has an implicit 'this' variable that is always accessible.
  * 'this' will be bound to the given instance
  * 
  * Note that the Instance will be kept alive due to shared_ptr usage, so returning a bound method
  * from a scope is fine, even though the object goes out of scope. It's value will be kept.
  */
  FunctionPtr bind(InstancePtr);

private:
  const std::variant<const FunctionStmt *, const Lambda *> declaration;
  std::shared_ptr<Environment> closure;
  const FunctionKind kind;
};
