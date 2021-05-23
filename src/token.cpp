#include "token.hpp"
#include "callable.hpp"
#include "function.hpp"
#include <cmath>

bool operator==([[maybe_unused]] const NullType &lhs,
                [[maybe_unused]] const NullType &rhs)
{
  return true;
}
bool operator!=([[maybe_unused]] const NullType &lhs,
                [[maybe_unused]] const NullType &rhs)
{
  return false;
}

std::ostream &operator<<(std::ostream &os,
                         [[maybe_unused]] const NullType &rhs)
{
  return os;
}

std::ostream &operator<<(std::ostream &os, const Token &t)
{
  os << static_cast<std::underlying_type<Token::TokenType>::type>(t.type)
     << "\t" << t.lexeme << "\t" << t.literal;

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

std::string stringify(const Token::literal_t &arg)
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
  if (std::holds_alternative<std::shared_ptr<Function>>(arg))
  {
    return std::get<std::shared_ptr<Function>>(arg)->to_string();
  }
  return "INVALID STRINGIFY REPRESENTATION";
}

std::ostream &operator<<(std::ostream &os, const Token::literal_t &literal)
{
  return os << stringify(literal);
}
