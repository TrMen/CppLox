#include "token.hpp"

#include <cmath>
#include <cassert>

#include "callable.hpp"
#include "function.hpp"
#include "error.hpp"
#include "instance.hpp"

Token::Token(TokenType _type, std::string _lexeme, Value _value,
             unsigned int _line) : type(_type), lexeme(std::move(_lexeme)), value(std::move(_value)),
                                   line(_line) {}

bool operator==(const NullType &,
                const NullType &)
{
  return true;
}
bool operator!=(const NullType &,
                const NullType &)
{
  return false;
}

std::ostream &operator<<(std::ostream &os,
                         const NullType &)
{
  return os;
}

std::ostream &operator<<(std::ostream &os, const Token &t)
{
  os << static_cast<std::underlying_type<Token::TokenType>::type>(t.type)
     << "\t" << t.lexeme << "\t" << t.value;

  return os;
}

std::ostream &operator<<(std::ostream &os, const std::vector<Token> &tokens)
{
  os << "{ ";
  for (const auto &token : tokens)
  {
    os << token << ", ";
  }
  os << "} \t";

  return os;
}

std::string stringify(const Token::Value &arg)
{
  if (std::holds_alternative<bool>(arg))
  {
    return std::get<bool>(arg) ? "true" : "false";
  }
  if (std::holds_alternative<NullType>(arg))
  {
    return "nil";
  }
  if (std::holds_alternative<double>(arg))
  {
    double num = std::get<double>(arg);
    if (std::floor(num) == num)
    {
      return std::to_string(static_cast<int>(num));
    }
    {
      return std::to_string(num);
    }
  }
  if (std::holds_alternative<std::string>(arg))
  {
    return std::get<std::string>(arg);
  }
  if (std::holds_alternative<std::shared_ptr<Callable>>(arg))
  {
    return std::get<std::shared_ptr<Callable>>(arg)->to_string();
  }
  if (std::holds_alternative<std::shared_ptr<Instance>>(arg))
  {
    return std::get<std::shared_ptr<Instance>>(arg)->to_string();
  }

  assert(false);
}

std::ostream &operator<<(std::ostream &os, const Token::Value &value)
{
  return os << stringify(value);
}
