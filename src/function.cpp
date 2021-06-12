#include "function.hpp"
#include "interpreter.hpp"
#include "logging.hpp"
#include <cassert>

using FuncPtr = const FunctionStmt *;
using LambdaPtr = const Lambda *;

Function::Function(const std::variant<const FunctionStmt *, const Lambda *> &_declaration,
                   std::shared_ptr<Environment> _closure, FunctionKind _kind)
    : declaration(_declaration),
      closure(std::move(_closure)), kind(_kind)
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

  try
  {
    interpreter.execute_block(body(), environment);
  }
  catch (const Interpreter::Return &returned) // Early return
  {
    if (kind == FunctionKind::CONSTRUCTOR) // Allow empty returns in constructors that implicitly return 'this'
      return closure->get_at(0, "this");   // Non-empty returns in constructors are caught by resolver
    return returned.val;
  }

  if (kind == FunctionKind::CONSTRUCTOR)
    return closure->get_at(0, "this");

  return NullType{};
}

size_t Function::arity() const { return parameters().size(); }

std::string Function::to_string() const
{
  switch (kind)
  {
  case FunctionKind::FUNCTION:
    return "<User fn " + std::get<FuncPtr>(declaration)->child<0>().lexeme + ">";
  case FunctionKind::LAMDBDA:
    return "<User lambda>";
  case FunctionKind::CONSTRUCTOR:
    return "<User constructor>";
  case FunctionKind::METHOD:
    return "<User method>";
  case FunctionKind::UNBOUND:
    return "<User unbound fn>";
  }

  return "";
}

std::shared_ptr<Function> Function::bind(std::shared_ptr<Instance> instance)
{
  auto env = std::make_shared<Environment>(closure);
  env->define("this", instance);
  return std::make_shared<Function>(declaration, env, kind);
}
