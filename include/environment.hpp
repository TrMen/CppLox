#pragma once

#include "token.hpp"
#include <memory>
#include <unordered_map>

///Store variable bindings
class Environment
{
public:
  explicit Environment(std::shared_ptr<Environment> _enclosing = nullptr);

  /// Define a new variable (or function) binding with name and value extracted from the Token.
  /// May throw RuntimeError if the name is already defined
  void define(Token variable);

  void define(std::string identifier, Token::Value value);

  /// Get a variable value by the name of the supplied token.
  /// @throws RuntimeError on unknown variable access.
  const Token::Value &get(const Token &token) const;

  /// Get a variable value by the name.
  /// This assumes the variable is found in the index'th nested environment
  /// Unlike for get(), this variable must be present
  const Token::Value &get_at(size_t depth, const std::string &name) const;

  /// Assign a new value to an existing variable.
  /// @throws RuntimeError on unknown variable access.
  void assign(const Token &name, const Token::Value &value);

  /// Assign a new value to an existing variable.
  /// This assumes the variable is found in the index'th nested environment
  /// Unlike for assign(), this variable must be present
  void assign_at(size_t depth, const std::string &name, Token::Value value);

  std::shared_ptr<Environment> enclosing = nullptr;

  std::string to_string() const;

  std::string to_string_recursive() const;

  size_t depth() const;

  std::unordered_map<std::string, Token::Value> variables;
};

std::ostream &operator<<(std::ostream &os, const Environment &env);