#include "lexer.hpp"

// clang-format off
const std::unordered_map<std::string, Lexer::Type> Lexer::keywords{
    {"and", Type::AND}, {"class", Type::CLASS}, {"else", Type::ELSE}, 
    {"false", Type::FALSE}, {"for", Type::FOR}, {"fun", Type::FUN}, 
    {"fn", Type::FUN}, {"if", Type::IF}, {"nil", Type::NIL}, 
    {"or", Type::OR}, {"print", Type::PRINT}, {"return", Type::RETURN}, 
    {"super", Type::SUPER}, {"this", Type::THIS}, {"true", Type::TRUE}, 
    {"var", Type::VAR}, {"while", Type::WHILE}, {"let", Type::VAR},
    {"return", Type::RETURN}, {"unbound", Type::UNBOUND}};
// clang-format on

Lexer::Lexer(std::string _source, std::shared_ptr<ErrorHandler> _err_handler)
    : source(std::move(_source)),
      err_handler(std::move(_err_handler))
{
  tokens.reserve(source.size() / 3);
}

bool Lexer::is_at_end() { return current >= source.size(); }

char Lexer::advance() { return source[current++]; }

void Lexer::scan_token()
{
  const auto c = advance();

  switch (c)
  {
  case '(':
    add_token(Type::LEFT_PAREN);
    break;
  case ')':
    add_token(Type::RIGHT_PAREN);
    break;
  case '{':
    add_token(Type::LEFT_BRACE);
    break;
  case '}':
    add_token(Type::RIGHT_BRACE);
    break;
  case '|':
    add_token(Type::PIPE);
    break;
  case ',':
    add_token(Type::COMMA);
    break;
  case '.':
    add_token(Type::DOT);
    break;
  case '-':
    add_token(Type::MINUS);
    break;
  case '+':
    add_token(Type::PLUS);
    break;
  case ';':
    add_token(Type::SEMICOLON);
    break;
  case '*':
    add_token(Type::STAR);
    break;
  case '?':
    add_token(Type::QUESTION_MARK);
    break;
  case ':':
    add_token(Type::COLON);
    break;
  case '!':
    add_token(expect('=') ? Type::BANG_EQUAL : Type::BANG);
    break;
  case '=':
    add_token(expect('=') ? Type::EQUAL_EQUAL : Type::EQUAL);
    break;
  case '<':
    add_token(expect('=') ? Type::LESS_EQUAL : Type::LESS);
    break;
  case '>':
    add_token(expect('=') ? Type::GREATER_EQUAL : Type::GREATER);
    break;
  case '/':
    slash_or_comment();
    break;
  case '\t':
  case ' ':
  case '\r':
    break;
  case '\n':
    ++line;
    break;
  case '"':
    string();
    break;
  default:
    if (isdigit(c) != 0)
    {
      number();
    }
    else if (isalpha(c) != 0)
    {
      identifier();
    }
    else
    {
      last_character_expected = false;
      last_syntax_error += c;
      syntax_error_start_line = line;
    }
    break;
  }
}

void Lexer::slash_or_comment()
{
  if (expect('/'))
  {
    // Comment till end of line
    while (peek() != '\n' && !is_at_end())
    {
      advance();
    }
  }
  else if (expect('*'))
  {
    unsigned int start_line = line;
    // Comment till matching */
    while (!is_at_end())
    {
      while (peek() != '*' && !is_at_end())
      {
        if (peek() == '\n')
        {
          ++line;
        }
        advance();
      }
      if (is_at_end())
      {
        err_handler->error(line, "Unterminated comment starting at line " +
                                     std::to_string(start_line));
        return;
      }
      advance(); // Consume the *

      if (peek() == '/')
      { // Look for closing /
        advance();
        return;
      }
    }
  }
  else
  {
    add_token(Type::SLASH);
  }
}

void Lexer::number()
{
  while (isdigit(peek()) != 0)
  {
    advance();
  }

  if (peek() == '.' && isdigit(peek_next()) != 0)
  {
    advance(); // Consume the .

    while (isdigit(peek()) != 0)
    {
      advance();
    }
  }
  add_token(Type::NUMBER, std::stod(source.substr(start, current - start)));
}

char Lexer::peek_next()
{
  if (current + 1 >= source.size())
  {
    return '\0';
  }
  return source[current + 1];
}

void Lexer::string()
{
  unsigned int start_line = line;
  while (peek() != '"' && !is_at_end())
  {
    if (peek() == '\n')
    {
      ++line;
    }
    advance();
  }

  if (is_at_end())
  {
    err_handler->error(line, "Unterminated string starting at line " +
                                 std::to_string(start_line));
    return;
  }

  advance(); // Consume the closing "

  std::string str = source.substr(start + 1, current - start - 2);
  add_token(Type::STRING, str);
}

char Lexer::peek()
{
  if (is_at_end())
  {
    return '\0';
  }
  return source[current];
}

void Lexer::add_token(Type type, Token::Value value)
{
  if (!last_character_expected)
  {
    last_character_expected = true;
    report_last_syntax_error();
  }
  std::string text = source.substr(start, current - start);
  tokens.emplace_back(type, text, value, line);
}

bool Lexer::expect(char expected)
{
  if (is_at_end() || source[current] != expected)
  {
    return false;
  }
  ++current;
  return true;
}

std::vector<Token> Lexer::lex()
{
  while (!is_at_end())
  {
    start = current;
    scan_token();
  }

  if (!last_character_expected)
  {
    report_last_syntax_error();
  }

  tokens.emplace_back(Type::_EOF, "", NullType(), line);
  return tokens;
}

void Lexer::identifier()
{
  while (isalnum(peek()) != 0)
  {
    advance();
  }

  std::string str = source.substr(start, current - start);
  const auto keyword_it = keywords.find(str);
  if (keyword_it != keywords.cend())
  {
    add_token(keyword_it->second);
  }
  else
  {
    add_token(Type::IDENTIFIER);
  }
}

// Syntax Error Handling:

void Lexer::report_last_syntax_error()
{
  std::string error_str = "Syntax error";
  if (syntax_error_start_line != line)
  {
    error_str +=
        " starting at line: " + std::to_string(syntax_error_start_line);
  }
  error_str += " ending at line: " + std::to_string(line) += ": ";
  error_str += last_syntax_error.size() >= 50 ? " with more than 50 characters"
                                              : "'" + last_syntax_error + "'";

  err_handler->error(line, error_str);
  last_syntax_error.clear();
  syntax_error_start_line = line;
}
