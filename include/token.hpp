#pragma once
#include <iostream>
#include <memory>
#include <string>
#include <variant>
#include <vector>

struct NullType {};

bool operator==(const NullType &, const NullType &);
bool operator!=(const NullType &, const NullType &);

struct Callable;
struct Class;
struct Function;
struct Instance;

using InstancePtr = std::shared_ptr<Instance>;
using CallablePtr = std::shared_ptr<Callable>;
using FunctionPtr = std::shared_ptr<Function>;
using ClassPtr = std::shared_ptr<Class>;

std::ostream &operator<<(std::ostream &os, const NullType &rhs);

struct Token {
  enum class TokenType {
    // Single-character tokens.
    LEFT_PAREN,
    RIGHT_PAREN,
    LEFT_BRACE,
    RIGHT_BRACE,
    COMMA,
    DOT,
    MINUS,
    PLUS,
    SEMICOLON,
    SLASH,
    STAR,
    QUESTION_MARK,
    COLON,
    PIPE,

    // One or two character tokens.
    BANG,
    BANG_EQUAL,
    EQUAL,
    EQUAL_EQUAL,
    GREATER,
    GREATER_EQUAL,
    LESS,
    LESS_EQUAL,

    // Literals.
    IDENTIFIER,
    STRING,
    NUMBER,

    // Keywords.
    AND,
    CLASS,
    ELSE,
    FALSE,
    FUN,
    FOR,
    IF,
    NIL,
    OR,
    PRINT,
    RETURN,
    SUPER,
    THIS,
    TRUE,
    VAR,
    WHILE,
    UNBOUND, // For methods that aren't bound (static methods)

    EOF_
  };

  using Value = std::variant<double, std::string, NullType, bool, CallablePtr,
                             InstancePtr>;

  Token(TokenType _type, std::string _lexeme, Value _value, unsigned int _line);

  const TokenType type;
  const std::string lexeme;
  Value value;
  const unsigned int line;
};

std::ostream &operator<<(std::ostream &os, const Token &t);

std::ostream &operator<<(std::ostream &os, const Token::Value &value);

std::ostream &operator<<(std::ostream &os, const std::vector<Token> &value);

std::string stringify(const Token::Value &value);

template <typename T>
std::shared_ptr<T> get_callable_as(const Token::Value &value) {
  static_assert(std::is_same_v<T, Class> || std::is_same_v<T, Function>,
                "Callable must be a class or function");

  if (const auto *val = std::get_if<CallablePtr>(&value)) {
    if (auto casted = std::dynamic_pointer_cast<T>(*val)) {
      return casted;
    }
  }
  return nullptr;
}