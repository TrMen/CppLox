#include "interpreter.hpp"
#include "callable.hpp"
#include "function.hpp"
#include "buildin.hpp"
#include "logging.hpp"

using Type = Token::TokenType;

//------------------Interface implementations-----------------------------

Interpreter::Interpreter(std::ostream &_os, std::shared_ptr<ErrorHandler> _err_handler)
    : out_stream(_os), err_handler(std::move(_err_handler))
{
  for (const auto &buildin : Buildin::get_buildins())
  {
    environment.define(buildin);
  }
}

void Interpreter::interpret(const std::vector<stmt> &statements)
{
  try
  {
    for (const stmt &statement : statements)
    {
      execute(statement);
    }
  }
  catch (const RuntimeError &err)
  {
    err_handler->runtime_error(err.token, err.what());
  }
}

void Interpreter::execute_block(const std::vector<stmt> &body,
                                Environment block_env)
{
  Environment original_env = std::move(environment);
  block_env.enclosing = &original_env;
  environment = std::move(block_env);

  LOG_DEBUG("Executing block statements with env: ", environment, " enclosed by ", *environment.enclosing);

  try
  {
    for (const auto &statement : body)
    {
      LOG_DEBUG("Executing statement in block: ", statement);
      execute(statement);
    }
    environment = std::move(original_env);
  }
  catch (...)
  {
    LOG_INFO("Caught exception in execute_block()");
    environment = original_env;
    throw;
  }
  LOG_DEBUG("Env at and of block execution: ", environment);
}

//-------------Start of interpreter visitor implementation------------------

void Interpreter::execute(const stmt &statement)
{
  dynamic_cast<StmtVisitableBase &>(*statement).accept(*this);
}

//---------- Helper functions for dynamic typing ------------

/* All values except NullType and the bool false are truthy, including "", 0, functions, callables*/
static bool is_truthy(const Token::Value &value)
{
  bool truthy = false;
  std::visit(
      [&truthy](auto &&arg)
      {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, NullType>)
        {
          truthy = false;
        }
        else if constexpr (std::is_same_v<T, bool>)
        {
          truthy = arg;
        }
        else
        {
          truthy = true;
        }
      },
      value);

  return truthy;
}

/// Operands are variants. Returns true only of all variants hold value_type
/// No operands returns true
template <typename value_type, typename... Types>
static bool check_operand_types(const Types &...operands)
{
  return (std::holds_alternative<value_type>(operands) && ...);
}

/// Throw a RuntimeError if any operand is not of value_type.
template <typename value_type, typename... Operands>
static void assert_operand_types(const Token &op, const Operands &...operands)
{
  if (!check_operand_types<double>(operands...))
  {
    if (std::is_same_v<value_type, double>)
    {
      throw RuntimeError(op, "Operands must be numbers");
    }
    if (std::is_same_v<value_type, std::string>)
    {
      throw RuntimeError(op, "Operands must be strings");
    }
    throw RuntimeError(op, "Operands must all be the same");
  }
}

/// Throw a runtime error if the condition is false
static void assert_true(bool condition, const Token &op,
                        const std::string &message)
{
  if (!condition)
  {
    throw RuntimeError(op, message);
  }
}

//-------------Statement Visitor Methods------------------------------------

void Interpreter::visit(FunctionStmt &visitable)
{
  auto function = visitable.child<0>();
  function.value = std::make_shared<Function>(&visitable);
  environment.define(function);
}

void Interpreter::visit(IfStmt &visitable)
{
  Token::Value cond = get_evaluated(visitable.child<0>());
  if (is_truthy(cond))
  {
    execute(visitable.child<1>());
  }
  else
  { // This correctly evaluates nothing with EmtpyStmt as else stmt (no else)
    execute(visitable.child<2>());
  }
}

void Interpreter::visit(WhileStmt &visitable)
{
  while (is_truthy(get_evaluated(visitable.child<0>())))
  {
    execute(visitable.child<1>());
  }
}

void Interpreter::visit(EmptyStmt &)
{
  last_value = NullType();
}

void Interpreter::visit(Block &visitable)
{
  Environment new_env{&environment};
  execute_block(visitable.child<0>(), std::move(new_env));
}

void Interpreter::visit(Var &visitable)
{
  auto variable = visitable.child<0>();
  const expr &initializer = visitable.child<1>();
  // This will correctly return NullType when the initializer is Empty
  variable.value = get_evaluated(initializer);

  environment.define(variable);
}

void Interpreter::visit(StmtExpr &visitable)
{
  get_evaluated(visitable.child<0>());
}

void Interpreter::visit(Print &visitable)
{
  out_stream << get_evaluated(visitable.child<0>()) << std::endl;
}

void Interpreter::visit(MalformedStmt &visitable)
{
  bool is_critical = visitable.child<0>();
  std::string lexer_message = visitable.child<1>();

  if (is_critical)
  {
    throw RuntimeError(Token(Type::_EOF, "MALFORMED", "MALFORMED", 0),
                       "Malformed statement node in AST. Syntax was not valid. Lexer "
                       "message:\t" +
                           lexer_message);
  }
  // Non-critical syntax errors just leave the last value untouched.
}

//-------------Expression Visitor methods------------------------------------

/// For a node, get the value of its visit. This is required because we only
/// have visit functions returning void
Token::Value Interpreter::get_evaluated(const expr &node)
{
  dynamic_cast<VisitableBase &>(*node).accept(*this);
  return last_value;
}

void Interpreter::visit(Call &visitable)
{
  Token::Value callee = get_evaluated(visitable.child<0>());

  const auto callable = [&visitable, &callee]()
  {
    if (std::holds_alternative<std::shared_ptr<Callable>>(callee) ||
        std::holds_alternative<std::shared_ptr<Function>>(callee))
    {
      return dynamic_cast<Callable *>(std::get<std::shared_ptr<Callable>>(callee).get());
    }
    throw RuntimeError(visitable.child<1>(),
                       "Can only call functions and classes.");
  }();

  // Check arity (number of arguments)
  if (visitable.child<2>().size() != callable->arity())
  {
    throw RuntimeError(visitable.child<1>(),
                       "Expected " + std::to_string(callable->arity()) +
                           " arguments but got " +
                           std::to_string(visitable.child<2>().size()) + ".");
  }

  // Evaluate arguments
  std::vector<Token::Value> arguments;
  for (const auto &argument : visitable.child<2>())
  {
    arguments.push_back(get_evaluated(argument));
  }

  last_value = callable->call(*this, arguments);
}

void Interpreter::visit(Assign &visitable)
{
  const Token &name = visitable.child<0>();
  Token::Value value = get_evaluated(visitable.child<1>());

  environment.assign(name, value);
  last_value = value;
}

void Interpreter::visit(Logical &visitable)
{
  Token::Value lhs = get_evaluated(visitable.child<0>());
  const Token &op = visitable.child<1>();
  if (op.type == Type::OR)
  {
    if (is_truthy(lhs))
      last_value = lhs;
    return;
  }
  else if (!is_truthy(lhs))
  {
    last_value = lhs;
    return;
  }
  last_value = get_evaluated(visitable.child<2>());
}

void Interpreter::visit(Variable &visitable)
{
  last_value = environment.get(visitable.child<0>());
}

void Interpreter::visit(Empty &)
{
  // Empty expressions just have a null value
  last_value = NullType();
}

void Interpreter::visit(Literal &visitable)
{
  last_value = visitable.child<0>();
}

void Interpreter::visit(Grouping &visitable)
{
  last_value = get_evaluated(visitable.child<0>());
}

void Interpreter::visit(Unary &visitable)
{
  Token::Value value = get_evaluated(visitable.child<1>());

  const Token &op = visitable.child<0>();

  switch (op.type)
  {
  case Type::MINUS:
    assert_operand_types<double>(op, value);
    last_value = -std::get<double>(value);
    break;
  case Type::BANG:
    last_value = !is_truthy(value);
    break;
  default:
    throw RuntimeError(op, "Unknown token type in unary operator eval");
  }
}

void Interpreter::visit(Binary &visitable)
{
  // This implementation defines left-to-right evaluation of binary
  // expressions
  Token::Value left = get_evaluated(visitable.child<0>());
  const Token &op = visitable.child<1>();
  Token::Value right = get_evaluated(visitable.child<2>());

  switch (op.type)
  {
  case Type::MINUS:
    assert_operand_types<double>(op, left, right);
    last_value = std::get<double>(left) - std::get<double>(right);
    break;
  case Type::SLASH:
    assert_operand_types<double>(op, left, right);
    assert_true(std::get<double>(right) != 0, op,
                "Right operand of division must not be 0");
    last_value = std::get<double>(left) / std::get<double>(right);
    break;
  case Type::STAR:
    assert_operand_types<double>(op, left, right);
    last_value = std::get<double>(left) * std::get<double>(right);
    break;
  case Type::PLUS:
    if (check_operand_types<double>(left, right))
    {
      last_value = std::get<double>(left) + std::get<double>(right);
    }
    else if (check_operand_types<std::string>(left) ||
             check_operand_types<std::string>(right))
    {
      last_value = stringify(left) + stringify(right);
    }
    else
    {
      throw RuntimeError(op, "Operands must all be numbers or strings");
    }
    break;
  case Type::GREATER:
    if (check_operand_types<double>(left, right))
    {
      last_value = std::get<double>(left) > std::get<double>(right);
      break;
    }
    else if (check_operand_types<std::string>(left, right))
    {
      last_value =
          std::get<std::string>(left).compare(std::get<std::string>(right)) > 0;
      break;
    }
    throw RuntimeError(op, "Operands must all be numbers or strings");
  case Type::GREATER_EQUAL:
    if (check_operand_types<double>(left, right))
    {
      last_value = std::get<double>(left) >= std::get<double>(right);
      break;
    }
    else if (check_operand_types<std::string>(left, right))
    {
      last_value = std::get<std::string>(left).compare(
                       std::get<std::string>(right)) >= 0;
      break;
    }
    throw RuntimeError(op, "Operands must all be numbers or strings");
  case Type::LESS:
    if (check_operand_types<double>(left, right))
    {
      last_value = std::get<double>(left) < std::get<double>(right);
      break;
    }
    else if (check_operand_types<std::string>(left, right))
    {
      last_value =
          std::get<std::string>(left).compare(std::get<std::string>(right)) < 0;
      break;
    }
    throw RuntimeError(op, "Operands must all be numbers or strings");
  case Type::LESS_EQUAL:
    if (check_operand_types<double>(left, right))
    {
      last_value = std::get<double>(left) < std::get<double>(right);
      break;
    }
    else if (check_operand_types<std::string>(left, right))
    {
      last_value = std::get<std::string>(left).compare(
                       std::get<std::string>(right)) <= 0;
      break;
    }
    throw RuntimeError(op, "Operands must all be numbers or strings");
  case Type::BANG_EQUAL:
    last_value = left != right;
    break;
  case Type::EQUAL_EQUAL:
    last_value = left == right;
    break;
  default:
    throw RuntimeError(op, "Unexpected operator in binary expression eval");
  }
}
void Interpreter::visit(Malformed &visitable)
{
  bool is_critical = visitable.child<0>();
  std::string lexer_message = visitable.child<1>();

  if (is_critical)
  {
    throw RuntimeError(Token(Type::_EOF, "MALFORMED", "MALFORMED", 0),
                       "Malformed expression node in AST. Syntax was not valid. Lexer "
                       "message:\t" +
                           lexer_message);
  }
  // Non-critical syntax errors just leave the last value untouched.
}

void Interpreter::visit(Ternary &visitable)
{
  Token::Value condition = get_evaluated(visitable.child<0>());
  const Token &first_op = visitable.child<1>();
  const expr &first = visitable.child<2>();
  const expr &second = visitable.child<4>();

  if (first_op.type == Type::QUESTION_MARK)
    last_value = is_truthy(condition) ? get_evaluated(first)
                                      : get_evaluated(second);
  else
    throw RuntimeError(first_op, "Unknown token type in ternary operator.");
}
