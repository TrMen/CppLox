#include "environment.hpp"

#include <sstream>
#include <cassert>

#include "error.hpp"
#include "logging.hpp"

Environment::Environment(std::shared_ptr<Environment> _enclosing) : enclosing(std::move(_enclosing)) {}

void Environment::define(Token variable)
{
  if (variables.find(variable.lexeme) != variables.cend())
  {
    throw RuntimeError(variable, "Identifier '" + variable.lexeme + "' is already defined in this scope.");
  }
  variables.emplace(std::move(variable.lexeme), std::move(variable.value));
}

void Environment::define(std::string identifier, Token::Value value)
{
  if (variables.find(identifier) != variables.cend())
  {
    throw RuntimeError(value, "Identifier '" + identifier + "' is already defined in this scope.", 0);
  }
  variables.emplace(std::move(identifier), std::move(value));
}

Token::Value Environment::get(const Token &token) const
{
  auto elem = variables.find(token.lexeme);
  LOG_DEBUG("Getting variable ", token.lexeme, " from : ", *this);
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

namespace
{
  const Environment *ancestor(const Environment *env, size_t depth)
  {
    while (depth > 0)
    {
      env = env->enclosing.get();
      assert(env != nullptr);
      --depth;
    }
    return env;
  }
}

Token::Value Environment::get_at(size_t depth, const std::string &name) const
{
  LOG_DEBUG("Get ", name, " at depth: ", depth, " with env:", depth, *ancestor(this, depth));
  // Existence must be ensured by resolver
  return ancestor(this, depth)->variables.at(name);
}

void Environment::assign(const Token &token, const Token::Value &value)
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

void Environment::assign_at(size_t depth, const std::string &name, Token::Value value)
{
  LOG_DEBUG("Assign at: ", *ancestor(this, depth));
  // const-cast here is fine since we know the original object was non-const. Reduces code duplication
  const_cast<Environment *>(ancestor(this, depth))->variables.at(name) = std::move(value);
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