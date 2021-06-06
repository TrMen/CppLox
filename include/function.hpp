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
  Function(const FunctionStmt *_declaration, std::shared_ptr<Environment> closure);

  Function(const Lambda *_declaration, std::shared_ptr<Environment> closure);

  Token::Value call(Interpreter &interpreter, const std::vector<Token::Value> &arguments) override;

  size_t arity() const override;
  std::string to_string() const override;

  const std::vector<Token> &parameters() const;
  const std::vector<stmt> &body() const;

private:
  const std::variant<const FunctionStmt *, const Lambda *> declaration;
  std::shared_ptr<Environment> closure;
};
