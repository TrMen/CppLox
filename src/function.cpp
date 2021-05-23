#include "function.hpp"
#include "interpreter.hpp"

Token::literal_t
Function::call(Interpreter &interpreter,
               const std::vector<Token::literal_t> &arguments)
{
  Environment environment{&interpreter.environment};

  const auto &parameters = declaration->child<1>();

  for (size_t i = 0; i < parameters.size(); ++i)
  {
    auto parameter = parameters[i];
    parameter.literal = arguments[i];
    environment.define(parameter);
  }

  interpreter.execute_block(declaration->child<2>(), environment);
  return NullType{};
}

size_t Function::arity() const { return declaration->child<1>().size(); }

std::string Function::to_string() const
{
  return "<User fn " + declaration->child<0>().lexeme + ">";
}
