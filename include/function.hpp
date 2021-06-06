#pragma once

#include "callable.hpp"
#include "stmt.hpp"
#include "environment.hpp"
#include <vector>

class Function : public Callable
{
public:
  // closure is deliberately taken by value because it should not be affected by
  // modifications after the function declaration
  explicit Function(const FunctionStmt *_declaration, Environment closure);

  Token::Value
  call(Interpreter &interpreter,
       const std::vector<Token::Value> &arguments) override;

  size_t arity() const override;
  std::string to_string() const override;

private:
  const FunctionStmt *declaration;
  Environment closure;
};
