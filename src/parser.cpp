#include "parser.hpp"
#include "function.hpp"
#include "logging.hpp"
#include <algorithm>

using Type = Token::TokenType;

Parser::Parser(std::vector<Token> _tokens,
               std::shared_ptr<ErrorHandler> _err_handler)
    : err_handler(std::move(_err_handler)), tokens(std::move(_tokens)) {}

const char *Parser::ParseError::what() const noexcept { return message; }

//---------------Primitive parser function implementations-----------------

bool Parser::is_at_end() const { return peek().type == Type::_EOF; }

const Token &Parser::peek() const { return tokens[current]; }

bool Parser::check(Type type) const
{
  if (is_at_end())
  {
    return false;
  }
  return peek().type == type;
}

const Token &Parser::previous() const { return tokens[current - 1]; }

const Token &Parser::advance()
{
  if (!is_at_end())
  {
    ++current;
  }
  return previous();
}

bool Parser::match(const std::vector<Type> &matched_types)
{
  return std::any_of(matched_types.cbegin(), matched_types.cend(), [this](auto &&type)
                     { return this->match(type); });
}

bool Parser::match(Type matched_type)
{
  if (check(matched_type))
  {
    advance();
    return true;
  }
  return false;
}

//--------------------------Production implementations---------------------

stmt Parser::declaration()
{
  try
  {
    if (match(Type::FUN))
      return function("Function");
    if (match(Type::VAR))
      return var_declaration();

    return statement();
  }
  catch (const ParseError &err)
  {
    synchronize();
    return new_stmt<MalformedStmt>(true, err.what());
  }
}

stmt Parser::var_declaration()
{
  Token name = consume(Type::IDENTIFIER, "Expect variable identitifier");

  expr initializer = new_expr<Empty>();
  if (match(Type::EQUAL))
  {
    initializer = expression();
  }
  consume(Type::SEMICOLON, "Expect ';' after variable declaration");
  return new_stmt<Var>(std::move(name), std::move(initializer));
}

stmt Parser::statement()
{
  if (match(Type::IF))
    return if_statement();
  if (match(Type::FOR))
    return for_statement();
  if (match(Type::WHILE))
    return while_statement();
  if (match(Type::LEFT_BRACE))
    return new_stmt<Block>(block());
  if (match(Type::PRINT))
    return print_statement();
  if (match(Type::RETURN))
    return return_statement();

  return expression_statement();
}

stmt Parser::function(const std::string &kind)
{
  Token name = consume(Type::IDENTIFIER, "Expected valid identifier as " + kind + " name.");
  consume(Type::LEFT_PAREN, "Expect '(' after " + kind + " name.");

  auto params = check(Type::RIGHT_PAREN) ? std::vector<Token>{} : parameters();

  consume(Type::RIGHT_PAREN, "Expect ')' after parameter list.");
  consume(Type::LEFT_BRACE, "Expect '{' before " + kind + " body.");

  LOG_DEBUG("Fn param size at creation: ", params.size());

  return new_stmt<FunctionStmt>(std::move(name), std::move(params),
                                block());
}

std::vector<Token> Parser::parameters()
{
  std::vector<Token> params;
  do
  {
    if (params.size() >= 255)
    {
      error(peek(), "Cannot define more than 255 parameters.");
    }
    params.push_back(consume(Type::IDENTIFIER, "Expect parameter name."));
  } while (match(Type::COMMA));

  return params;
}

stmt Parser::for_statement()
{
  consume(Type::LEFT_PAREN, "Expect '(' after 'for'.");

  // First clause: initializer
  stmt initializer;
  if (match(Type::SEMICOLON))
  {
    ; // Do nothing
  }
  else if (match(Type::VAR))
  {
    initializer = var_declaration();
  }
  else
  {
    initializer = expression_statement();
  }
  // We don't need to match a ; here because all stmts have one

  // Second clause: condition
  expr condition;
  if (!check(Type::SEMICOLON))
  {
    condition = expression();
  }
  consume(Type::SEMICOLON, "Expect ';' after loop condition");

  // Third clause: increment
  expr increment;
  if (!check(Type::RIGHT_PAREN))
  {
    increment = expression();
  }
  consume(Type::RIGHT_PAREN, "Expect ')' after for loop clauses.");

  stmt body = statement();

  // Transform for statement into while statement for interpretation
  if (increment != nullptr)
  { // Add increment into while loop body
    std::vector<stmt> body_statements;
    body_statements.push_back(std::move(body));
    body_statements.push_back(new_stmt<StmtExpr>(std::move(increment)));

    body = new_stmt<Block>(std::move(body_statements));
  }

  if (condition == nullptr)
  { // Add condition, or true if none specified
    condition = new_expr<Literal>(true);
  }
  body = new_stmt<WhileStmt>(std::move(condition), std::move(body));

  if (initializer != nullptr)
  { // Add outer block with init if necessary
    std::vector<stmt> full_statements;
    full_statements.push_back(std::move(initializer));
    full_statements.push_back(std::move(body));
    body = new_stmt<Block>(std::move(full_statements));
  }

  return body;
}

stmt Parser::while_statement()
{
  consume(Type::LEFT_PAREN, "Expect '(' after 'while'.");
  expr cond = expression();
  consume(Type::RIGHT_PAREN, "Expect ')' after while condition");
  stmt body = statement();

  return new_stmt<WhileStmt>(std::move(cond), std::move(body));
}

stmt Parser::if_statement()
{
  consume(Type::LEFT_PAREN, "Expect '(' after 'if'.");
  expr cond = expression();
  consume(Type::RIGHT_PAREN, "Expect ')' after condition of if statement.");

  stmt then_stmt = statement();
  stmt else_stmt = new_stmt<EmptyStmt>();
  if (match(Type::ELSE))
  {
    else_stmt = statement();
  }

  return new_stmt<IfStmt>(std::move(cond), std::move(then_stmt),
                          std::move(else_stmt));
}

std::vector<stmt> Parser::block()
{
  std::vector<stmt> statements;

  while (!check(Type::RIGHT_BRACE) && !is_at_end())
  {
    statements.push_back(declaration());
  }

  consume(Type::RIGHT_BRACE, "Expect '}' after block.");
  return statements;
}

stmt Parser::print_statement()
{
  expr value = expression();
  consume(Type::SEMICOLON, "Expect ';' after statement");
  return new_stmt<Print>(std::move(value));
}

stmt Parser::expression_statement()
{
  expr value = expression();
  consume(Type::SEMICOLON, "Expect ';' after expression");
  return new_stmt<StmtExpr>(std::move(value));
}

stmt Parser::return_statement()
{
  Token return_keyword = previous(); // Keep for error-reporting
  expr body = new_expr<Empty>();     // Returned value is optional.

  if (!check(Type::SEMICOLON))
    body = expression();

  consume(Type::SEMICOLON, "Expect ';' after 'return' statement's expression");

  return new_stmt<ReturnStmt>(std::move(return_keyword), std::move(body));
}

/** Binary left-associative productions of the form
 * prod: derived | derived [list_of_terminals] prod
 * The supplied function pointer implements the derived production
 * matched_types is a list of tokens to match as operators
 * prod is the binary production itself
 * expr_type is the resulting derived AST type of the returned expr
 */
template <expr (Parser::*production)(), const std::vector<Type> &matched_types,
          const std::vector<Type> &error_types, typename expr_type = Binary>
static expr left_associative_binary_production(Parser *owner)
{
  if (owner->match(error_types))
  { // Erronous use of binary operator as unary
    Token prev = owner->previous();
    (owner->*production)(); // Discard result
    owner->err_handler->error(prev, "Illegal use of unary operator " + prev.lexeme);
    return new_expr<Malformed>(true, "Illegal use of unary operator " + prev.lexeme);
  }
  expr result = (owner->*production)();

  while (owner->match(matched_types))
  {
    Token op = owner->previous();
    expr rhs = (owner->*production)();
    result = new_expr<expr_type>(std::move(result), std::move(op), std::move(rhs));
  }
  return result;
}

expr Parser::expression()
{
  static const std::vector<Type> expression_types{Type::COMMA};
  return left_associative_binary_production<
      &Parser::comma_expression, expression_types, expression_types>(this);
}

expr Parser::comma_expression() { return assignment(); }

expr Parser::assignment()
{
  expr x_value = ternary_conditional();

  if (match(Type::EQUAL))
  {
    const auto &equal = previous();
    expr value = assignment();

    // This checks whether we can assign to the x_value.
    auto *l_value = dynamic_cast<Variable *>(x_value.get());
    if (l_value != nullptr)
    {
      Token name = l_value->child<0>();
      return new_expr<Assign>(std::move(name), std::move(value));
    }

    error(equal, "Invalid assignment operator"); // Report but don't throw
  }
  return x_value;
}

expr Parser::ternary_conditional()
{
  expr result = or_expression();

  if (match(Type::QUESTION_MARK))
  {
    Token question_mark = previous();
    expr middle = expression();
    Token colon = consume(Type::COLON, "Expected ':' after '?' for ternary conditional operator");
    expr right = expression();
    result = new_expr<Ternary>(std::move(result), std::move(question_mark),
                               std::move(middle), std::move(colon),
                               std::move(right));
  }
  return result;
}

expr Parser::or_expression()
{
  static const std::vector<Type> or_expression_types{Type::OR};
  return left_associative_binary_production<
      &Parser::and_expression, or_expression_types, or_expression_types, Logical>(this);
}

expr Parser::and_expression()
{
  static const std::vector<Type> and_expression_types{Type::AND};
  return left_associative_binary_production<
      &Parser::equality, and_expression_types, and_expression_types, Logical>(this);
}

expr Parser::equality()
{
  static const std::vector<Type> equality_types{Type::BANG_EQUAL, Type::EQUAL_EQUAL};
  return left_associative_binary_production<
      &Parser::comparison, equality_types, equality_types>(this);
}

expr Parser::comparison()
{
  static const std::vector<Type> comparison_types{Type::GREATER, Type::GREATER_EQUAL,
                                                  Type::LESS, Type::LESS_EQUAL};
  return left_associative_binary_production<
      &Parser::addition, comparison_types, comparison_types>(this);
}

expr Parser::addition()
{
  static const std::vector<Type> addition_types{Type::MINUS, Type::PLUS};
  static const std::vector<Type> addition_forbidden_unaries{Type::PLUS};
  return left_associative_binary_production<
      &Parser::multiplication, addition_types, addition_forbidden_unaries>(
      this);
}

static std::vector<Type> multiplication_types{Type::STAR, Type::SLASH};
expr Parser::multiplication()
{
  return left_associative_binary_production<
      &Parser::unary, multiplication_types, multiplication_types>(this);
}

expr Parser::unary()
{
  if (match({Type::BANG, Type::MINUS}))
  {
    Token prev = previous();
    return new_expr<Unary>(std::move(prev), unary());
  }
  return call();
}

expr Parser::call()
{
  expr result = primary();

  while (true)
  {
    if (match(Type::LEFT_PAREN))
    {
      result = finish_call(std::move(result));
    }
    else
    {
      break;
    }
  }

  return result;
}

expr Parser::finish_call(expr callee)
{
  std::vector<expr> arguments;
  if (!check(Type::RIGHT_PAREN))
  {
    do
    {
      if (arguments.size() >= 255)
      {
        error(peek(), "Cannot have more than 255 function arguments");
      }
      arguments.push_back(comma_expression());
    } while (match(Type::COMMA));
  }

  Token paren = consume(Type::RIGHT_PAREN, "Expect ')' after arguments");

  return new_expr<Call>(std::move(callee), std::move(paren),
                        std::move(arguments));
}

expr Parser::primary()
{
  if (match(Type::FALSE))
    return new_expr<Literal>(false);
  if (match(Type::TRUE))
    return new_expr<Literal>(true);

  if (match(Type::NIL))
    return new_expr<Literal>(NullType());

  if (match({Type::NUMBER, Type::STRING}))
  {
    Token::Value previous_val = previous().value;
    return new_expr<Literal>(std::move(previous_val));
  }

  if (match(Type::IDENTIFIER))
  {
    auto variable = previous();
    return new_expr<Variable>(std::move(variable));
  }

  if (match(Type::LEFT_PAREN))
  {
    expr middle = expression();
    consume(Type::RIGHT_PAREN, "Expected ')' after expression");
    return new_expr<Grouping>(std::move(middle));
  }

  if (match(Type::PIPE))
  {
    auto params = check(Type::PIPE) ? std::vector<Token>{} : parameters();
    consume(Type::PIPE, "Expect '|' to finish lambda parameter list");
    if (match(Type::LEFT_BRACE))
    {
      return new_expr<Lambda>(std::move(params), block());
    }
    else
    {                                    // Single-expression lambda implicitly returns its value
      Token return_keyword = previous(); // Keep for error-reporting. Copy required here
      stmt implicit_return = std::make_unique<ReturnStmt>(std::move(return_keyword), expression());
      std::vector<stmt> block; // Initialization in constructor not possible because of unique_ptr
      block.emplace_back(std::move(implicit_return));
      return new_expr<Lambda>(std::move(params), std::move(block));
    }
  }

  throw error(peek(), "Expect expression.");
}

const Token &Parser::consume(Type type, const std::string &message)
{
  if (check(type))
  {
    return advance();
  }
  throw error(peek(), message);
}

Parser::ParseError Parser::error(const Token &token,
                                 const std::string &message)
{
  err_handler->error(token, message.c_str());
  return ParseError(message);
}

void Parser::synchronize()
{
  advance();

  while (!is_at_end() && previous().type != Type::SEMICOLON)
  {
    switch (peek().type)
    {
    case Type::CLASS:
    case Type::FUN:
    case Type::VAR:
    case Type::FOR:
    case Type::IF:
    case Type::WHILE:
    case Type::PRINT:
    case Type::RETURN:
      return;
    default:
      advance();
      break;
    }
  }
}

std::vector<stmt> Parser::parse()
{
  std::vector<stmt> statements;
  while (!is_at_end())
  {
    statements.push_back(declaration());
  }

  return statements;
}
