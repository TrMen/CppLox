#pragma once

#include <memory>
#include <unordered_map>

#include "class.hpp"

struct Instance : public std::enable_shared_from_this<Instance> {
  explicit Instance(ClassPtr);

  [[nodiscard]] std::string to_string() const;

  [[nodiscard]] virtual Token::Value get_field(const Token &name,
                                               Interpreter &);

  virtual void set_field(const Token &name, Token::Value);

private:
  // Field are more general than properties. A field is anything defined on an
  // instance, like a method or property
  std::unordered_map<std::string, Token::Value> fields;

  ClassPtr klass;
};