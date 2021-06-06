#pragma once
#include <iostream>
#include <memory>
#include <string>
#include <variant>
#include <vector>

struct NullType
{
};
bool operator==([[maybe_unused]] const NullType &lhs,
                [[maybe_unused]] const NullType &rhs);
bool operator!=([[maybe_unused]] const NullType &lhs,
                [[maybe_unused]] const NullType &rhs);

struct Callable;
class Class;
class Instance;

std::ostream &operator<<(std::ostream &os, const NullType &rhs);

struct Token
{
  enum class TokenType
  {
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

    _EOF
  };

  using Value =
      std::variant<double, std::string, NullType, bool,
                   std::shared_ptr<Callable>>;

  Token(TokenType _type, std::string _lexeme, Value _value,
        unsigned int _line);

  const TokenType type;
  const std::string lexeme;
  Value value;
  const unsigned int line;
};

std::ostream &operator<<(std::ostream &os, const Token &t);

std::ostream &operator<<(std::ostream &os, const Token::Value &value);

std::ostream &operator<<(std::ostream &os, const std::vector<Token> &value);

std::string stringify(const Token::Value &value);
