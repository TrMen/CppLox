#pragma once

#include "callable.hpp"
#include "stmt.hpp"
#include <vector>

class Function : public Callable
{
public:
  explicit Function(std::unique_ptr<const FunctionStmt> _declaration)
      : declaration(std::move(_declaration)) {}

  Token::literal_t
  call(Interpreter &interpreter,
       const std::vector<Token::literal_t> &arguments) override;

  size_t arity() const override;
  std::string to_string() const override;

private:
  std::unique_ptr<const FunctionStmt> declaration;
};
