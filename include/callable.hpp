#pragma once

#include "token.hpp"
#include <vector>

struct Interpreter;

struct Callable
{
  virtual Token::Value
  call(Interpreter &interpreter,
       const std::vector<Token::Value> &arguments) = 0;
  virtual size_t arity() const = 0;
  virtual std::string to_string() const = 0;

  // Base class boilerplate
  Callable() = default;
  virtual ~Callable() = default;
  Callable(const Callable &) = delete;
  Callable &operator=(const Callable &) = delete;
  Callable(Callable &&) = delete;
  Callable &operator=(Callable &&) = delete;
};
