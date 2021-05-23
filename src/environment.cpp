#include "environment.hpp"
#include "error.hpp"
#include "logging.hpp"
#include <sstream>

void Environment::define(Token variable)
{
  if (variables.find(variable.lexeme) != variables.cend())
  {
    throw RuntimeError(variable, "Identifier '" + variable.lexeme + "' is already defined in this scope.");
  }
  variables.emplace(std::move(variable.lexeme), std::move(variable.literal));
}

Token::literal_t Environment::get(const Token &token) const
{
  auto elem = variables.find(token.lexeme);
  LOG_DEBUG("Getting variable ", token.lexeme);
  if (elem != variables.cend())
  {
    return elem->second;
  }
  if (enclosing != nullptr)
  {
    return enclosing->get(token);
  }
  throw RuntimeError(token, "Cannot access undefined identifier '" + token.lexeme + "'.");
}

void Environment::assign(const Token &token, const Token::literal_t &value)
{
  auto elem = variables.find(token.lexeme);
  if (elem == variables.end())
  {
    if (enclosing != nullptr)
    {
      return enclosing->assign(token, value);
    }
    throw RuntimeError(token, "Cannot assign to undefined identifier '" + token.lexeme + "'.");
  }
  elem->second = value;
}

std::string Environment::to_string() const
{
  std::stringstream env;

  env << "{";

  for (const auto &variable : variables)
  {
    env << variable.first << ": " << variable.second << ", ";
  }
  env << "}";

  return env.str();
}

std::ostream &operator<<(std::ostream &os, const Environment &env)
{
  return os << env.to_string();
}