#pragma once

#include "callable.hpp"
#include "stmt.hpp"
#include <vector>

class Function : public Callable
{
public:
  explicit Function(const FunctionStmt *_declaration);

  Token::Value
  call(Interpreter &interpreter,
       const std::vector<Token::Value> &arguments) override;

  size_t arity() const override;
  std::string to_string() const override;

private:
  const FunctionStmt *declaration;
};
