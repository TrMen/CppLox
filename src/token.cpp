#include "token.hpp"

#include <cmath>
#include <cassert>

#include "callable.hpp"
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

struct StringifyVisitor
{
  std::string operator()(bool arg)
  {
    return arg ? "true" : "false";
  }
  std::string operator()(NullType)
  {
    return "nil";
  }
  std::string operator()(const std::string &arg)
  {
    return arg;
  }
  std::string operator()(double num)
  {
    if (std::floor(num) == num)
    {
      return std::to_string(static_cast<int>(num));
    }
    return std::to_string(num);
  }
  std::string operator()(const CallablePtr &arg)
  {
    return arg->to_string();
  }
  std::string operator()(const InstancePtr &arg)
  {
    return arg->to_string();
  }
};

std::string stringify(const Token::Value &arg)
{
  return std::visit(StringifyVisitor{}, arg);
}

std::ostream &operator<<(std::ostream &os, const Token::Value &value)
{
  return os << stringify(value);
}
