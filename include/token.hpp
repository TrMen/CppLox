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
class Function;
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

  using literal_t =
      std::variant<double, std::string, NullType, bool,
                   std::shared_ptr<Callable>, std::shared_ptr<Function>>;

  Token(TokenType _type, std::string _lexeme, literal_t _literal,
        unsigned int _line)
      : type(_type), lexeme(std::move(_lexeme)), literal(std::move(_literal)),
        line(_line) {}

  const TokenType type;
  const std::string lexeme;
  literal_t literal;
  const unsigned int line;
};

std::ostream &operator<<(std::ostream &os, const Token &t);

std::ostream &operator<<(std::ostream &os, const Token::literal_t &literal);

std::ostream &operator<<(std::ostream &os, const std::vector<Token> &literal);

std::string stringify(const Token::literal_t &literal);
