#include "token.hpp"
#include <memory>
#include <unordered_map>

///Store variable bindings
class Environment
{
public:
  explicit Environment(Environment *_enclosing = nullptr) : enclosing(_enclosing) {}

  /// Define a new variable (or function) binding with name and value extracted from the Token.
  /// May throw RuntimeError if the name is already defined
  void define(Token variable);

  /// Get a variable value by the name of the supplied token.
  /// May throw RuntimeError on unknown variable access.
  Token::literal_t get(const Token &token) const;

  /// Assign a new value to an existing variable.
  /// May throw RuntimeError on unknown variable access.
  void assign(const Token &name, const Token::literal_t &value);

  Environment *enclosing = nullptr;

  std::string to_string() const;

private:
  std::unordered_map<std::string, Token::literal_t> variables;
};

std::ostream &operator<<(std::ostream &os, const Environment &env);