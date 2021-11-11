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
  if (const auto *decl = std::get_if<FuncPtr>(&declaration))
  {
    return (*decl)->child<1>();
  }
  if (const auto *decl = std::get_if<LambdaPtr>(&declaration))
  {
    return (*decl)->child<0>();
  }

  assert(false && "Unhandled function type n Function::parameters()");
}

const std::vector<stmt> &Function::body() const
{
  if (const auto *decl = std::get_if<FuncPtr>(&declaration))
  {
    return (*decl)->child<2>();
  }
  if (const auto *decl = std::get_if<LambdaPtr>(&declaration))
  {
    return (*decl)->child<1>();
  }

  assert(false && "Unhandled function type in Function::body()");
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
    interpreter.execute_block(body(), std::move(environment));
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
  case FunctionKind::GETTER:
    return "<User getter>";
  }

  return "";
}

FunctionPtr Function::bind(InstancePtr instance)
{
  auto env = std::make_shared<Environment>(closure);
  env->define("this", std::move(instance));
  return std::make_shared<Function>(declaration, std::move(env), kind);
}
