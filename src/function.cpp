#include "function.hpp"
#include "interpreter.hpp"
#include "logging.hpp"
#include <cassert>

using FuncPtr = const FunctionStmt *;
using LambdaPtr = const Lambda *;

Function::Function(FuncPtr _declaration, std::shared_ptr<Environment> closure)
    : declaration(_declaration),
      closure(std::move(closure))
{
}

Function::Function(LambdaPtr _declaration, std::shared_ptr<Environment> closure)
    : declaration(_declaration),
      closure(std::move(closure))
{
}

const std::vector<Token> &Function::parameters() const
{
  if (std::holds_alternative<FuncPtr>(declaration))
    return std::get<FuncPtr>(declaration)->child<1>();
  else if (std::holds_alternative<LambdaPtr>(declaration))
    return std::get<LambdaPtr>(declaration)->child<0>();

  assert(false);
}

const std::vector<stmt> &Function::body() const
{
  if (std::holds_alternative<FuncPtr>(declaration))
    return std::get<FuncPtr>(declaration)->child<2>();
  else if (std::holds_alternative<LambdaPtr>(declaration))
    return std::get<LambdaPtr>(declaration)->child<1>();

  assert(false);
}

Token::Value Function::call(Interpreter &interpreter, const std::vector<Token::Value> &arguments)
{
  auto environment = std::make_shared<Environment>(closure);

  LOG_DEBUG("Calling func with closure: ", *environment, " enclosed by ", *environment->enclosing);

  const auto &params = parameters();

  for (size_t i = 0; i < params.size(); ++i)
  {
    auto parameter = params[i];
    parameter.value = arguments[i];
    environment->define(parameter);
  }

  LOG_DEBUG("Fn env after defining params: ", *environment);

  try
  {
    interpreter.execute_block(body(), environment);
  }
  catch (const Interpreter::Return &returned) // Early return
  {
    return returned.val;
  }

  return NullType{};
}

size_t Function::arity() const { return parameters().size(); }

std::string Function::to_string() const
{
  if (std::holds_alternative<FuncPtr>(declaration))
  {
    const auto &function_name = std::get<FuncPtr>(declaration)->child<0>().lexeme;
    return "<User fn " + function_name + ">";
  }
  else
    return "<User lambda>";
}
