#include "interpreter.hpp"

#include <filesystem>

#include "callable.hpp"
#include "function.hpp"
#include "buildin.hpp"
#include "logging.hpp"
#include "class.hpp"
#include "instance.hpp"

using Type = Token::TokenType;

Interpreter::Interpreter(std::ostream &_os, std::shared_ptr<ErrorHandler> _err_handler)
    : out_stream(_os), globals(std::make_shared<Environment>()), environment(globals), err_handler(std::move(_err_handler)),
      interpreter_path{std::filesystem::current_path()}
{
  for (const auto &buildin : Buildin::get_buildins())
  {
    globals->define(buildin);
  }
}

//----------Top-level interpretation, evaluation and execution methods----------

void Interpreter::interpret(std::vector<stmt> &statements)
{
  ast = &statements;

  try
  {
    for (stmt &statement : statements)
    {
      execute(statement);

      LOG_INFO("Last value after stmt: ", last_value);
    }
  }
  catch (const RuntimeError &err)
  {
    err_handler->runtime_error(err.token, err.what());
  }
}

void Interpreter::execute_block(const std::vector<stmt> &body, std::shared_ptr<Environment> enclosing_env)
{
  auto original_env = environment;
  environment = std::make_shared<Environment>(std::move(enclosing_env));

  LOG_DEBUG("Executing block statements with env: ", *environment, " enclosed by ", *environment->enclosing);

  try
  {
    for (const auto &statement : body)
    {
      execute(statement);
    }
  }
  catch (...)
  {
    LOG_DEBUG("Caught exception in block. Restoring original env.");
    environment = std::move(original_env);
    throw;
  }
  environment = std::move(original_env);
  LOG_DEBUG("Env at and of block execution: ", *environment);
}

void Interpreter::execute(const stmt &statement)
{
  dynamic_cast<StmtVisitableBase &>(*statement).accept(*this);
}

/// For a node, get the value of its visit. This is required because we only
/// have visit functions returning void
Token::Value Interpreter::get_evaluated(const expr &expression)
{
  dynamic_cast<ExprVisitableBase &>(*expression).accept(*this);
  return last_value;
}

//---------- Helper functions ------------
namespace
{
  /* All values except NullType and the bool false are truthy, including "", 0, functions, callables*/
  bool is_truthy(const Token::Value &value)
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
  bool check_operand_types(const Types &...operands)
  {
    return (std::holds_alternative<value_type>(operands) && ...);
  }

  /// Throw a RuntimeError if any operand is not of value_type.
  template <typename value_type, typename... Operands>
  void assert_operand_types(const Token &op, const Operands &...operands)
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
  void assert_true(bool condition, const Token &op,
                   const std::string &message)
  {
    if (!condition)
    {
      throw RuntimeError(op, message);
    }
  }

}

//-------------Statement Visitor Methods------------------------------------

void Interpreter::visit(ReturnStmt &node)
{
  // If there is no value, the Empty expression will be evaluated to NullType
  throw Return{get_evaluated(node.child<1>())};
}

void Interpreter::visit(FunctionStmt &node)
{
  auto function = node.child<0>();
  LOG_DEBUG("Declaring func ", function.lexeme, " with env: ", *environment);
  function.value = std::make_shared<Function>(&node, environment);
  environment->define(std ::move(function));
}

void Interpreter::visit(ClassStmt &node)
{
  auto klass = node.child<0>();

  std::unordered_map<std::string, std::shared_ptr<Function>> methods;
  for (const auto &method : node.child<1>())
  {
    // Every AST node method becomes a runtime function that captures the envrionment
    // This allows methods to keep being associated with their original objects
    methods.emplace(method->child<0>().lexeme, std::make_shared<Function>(method.get(), environment));
  }

  klass.value = std::make_shared<Class>(node.child<0>().lexeme, std::move(methods));

  environment->define(std::move(klass));
}

void Interpreter::visit(IfStmt &node)
{
  auto cond = get_evaluated(node.child<0>());
  if (is_truthy(cond))
  {
    execute(node.child<1>());
  }
  else
  { // This correctly evaluates nothing with EmtpyStmt as else stmt (no else)
    execute(node.child<2>());
  }
}

void Interpreter::visit(WhileStmt &node)
{
  while (is_truthy(get_evaluated(node.child<0>())))
  {
    execute(node.child<1>());
  }
}

void Interpreter::visit(EmptyStmt &)
{
  last_value = NullType();
}

void Interpreter::visit(BlockStmt &node)
{
  execute_block(node.child<0>(), environment);
}

void Interpreter::visit(VarStmt &node)
{
  auto variable = node.child<0>();
  const expr &initializer = node.child<1>();
  // This will correctly return NullType when the initializer is Empty
  variable.value = get_evaluated(initializer);

  environment->define(variable);
}

void Interpreter::visit(ExprStmt &node)
{
  get_evaluated(node.child<0>());
}

void Interpreter::visit(PrintStmt &node)
{
  out_stream << get_evaluated(node.child<0>()) << std::endl;
}

void Interpreter::visit(MalformedStmt &node)
{
  bool is_critical = node.child<0>();
  std::string lexer_message = node.child<1>();

  if (is_critical)
  {
    throw RuntimeError(Token(Type::_EOF, "MALFORMED", "MALFORMED", 0),
                       "Malformed statement node in AST. Syntax was not valid. Lexer "
                       "message:\t" +
                           lexer_message);
  }
  // Non-critical syntax errors just leave the last value untouched.
}

//-------------Expression Visitor Methods------------------------------------

void Interpreter::visit(Lambda &node)
{
  LOG_DEBUG("Declaring lambda");

  last_value = std::make_shared<Function>(&node, environment);
}

void Interpreter::visit(Call &node)
{
  auto callee = get_evaluated(node.child<0>());

  const auto callable = [&node, &callee]()
  {
    if (std::holds_alternative<std::shared_ptr<Callable>>(callee))
    {
      return dynamic_cast<Callable *>(std::get<std::shared_ptr<Callable>>(callee).get());
    }
    else if (std::holds_alternative<std::shared_ptr<Function>>(callee))
    {
      return dynamic_cast<Callable *>(std::get<std::shared_ptr<Function>>(callee).get());
    }
    throw RuntimeError(node.child<1>(),
                       "Can only call functions and classes.");
  }();

  // Check arity (number of arguments)
  if (node.child<2>().size() != callable->arity())
  {
    throw RuntimeError(node.child<1>(),
                       "Expected " + std::to_string(callable->arity()) +
                           " arguments but got " +
                           std::to_string(node.child<2>().size()) + ".");
  }

  // Evaluate arguments
  std::vector<Token::Value> arguments;
  for (const auto &argument : node.child<2>())
  {
    arguments.push_back(get_evaluated(argument));
  }

  last_value = callable->call(*this, arguments);
}

void Interpreter::visit(Get &node)
{
  auto object = get_evaluated(node.child<0>());

  if (std::holds_alternative<std::shared_ptr<Instance>>(object))
  {
    last_value = std::get<std::shared_ptr<Instance>>(object)->get_field(node.child<1>());
    return;
  }

  throw RuntimeError(node.child<1>(), "Expression before '.' must evaluate to an object");
}

void Interpreter::visit(Set &node)
{
  auto object = get_evaluated(node.child<0>());

  if (!std::holds_alternative<std::shared_ptr<Instance>>(object))
  {
    throw RuntimeError(node.child<1>(), "Expression before '.' must evaluate to an object");
  }

  auto value = get_evaluated(node.child<2>());

  std::get<std::shared_ptr<Instance>>(object)->set_field(node.child<1>(), value);

  last_value = std::move(value);
}

void Interpreter::visit(Assign &node)
{
  Token::Value value = get_evaluated(node.child<1>());

  const auto &identifier = node.child<0>();
  if (node.depth.has_value())
  {
    environment->assign_at(*node.depth, identifier.lexeme, value);
  }
  else
  {
    globals->assign(identifier, value);
  }

  last_value = std::move(value);
}

void Interpreter::visit(Logical &node)
{
  Token::Value lhs = get_evaluated(node.child<0>());
  const Token &op = node.child<1>();
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
  last_value = get_evaluated(node.child<2>());
}

void Interpreter::visit(Variable &node)
{
  const auto &identifier = node.child<0>();

  if (node.depth.has_value())
  {
    LOG_DEBUG("Node ", node.child<0>(), " has depth: ", *node.depth);
    last_value = environment->get_at(*node.depth, identifier.lexeme);
  }
  else
  {
    last_value = globals->get(identifier);
  }
}

void Interpreter::visit(Empty &)
{
  // Empty expressions just have a null value
  last_value = NullType();
}

void Interpreter::visit(Literal &node)
{
  last_value = node.child<0>();
}

void Interpreter::visit(Grouping &node)
{
  last_value = get_evaluated(node.child<0>());
}

void Interpreter::visit(Unary &node)
{
  Token::Value value = get_evaluated(node.child<1>());

  const Token &op = node.child<0>();

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

void Interpreter::visit(Binary &node)
{
  // This implementation defines left-to-right evaluation of binary
  // expressions
  Token::Value left = get_evaluated(node.child<0>());
  const Token &op = node.child<1>();
  Token::Value right = get_evaluated(node.child<2>());

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
      last_value = std::get<double>(left) <= std::get<double>(right);
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
void Interpreter::visit(Malformed &node)
{
  bool is_critical = node.child<0>();
  std::string lexer_message = node.child<1>();

  if (is_critical)
  {
    throw RuntimeError(Token(Type::_EOF, "MALFORMED", "MALFORMED", 0),
                       "Malformed expression node in AST. Syntax was not valid. Lexer message:\t" + lexer_message);
  }
  // Non-critical syntax errors just leave the last value untouched.
}

void Interpreter::visit(Ternary &node)
{
  Token::Value condition = get_evaluated(node.child<0>());
  const Token &first_op = node.child<1>();
  const expr &first = node.child<2>();
  const expr &second = node.child<4>();

  if (first_op.type == Type::QUESTION_MARK)
    last_value = is_truthy(condition) ? get_evaluated(first)
                                      : get_evaluated(second);
  else
    throw RuntimeError(first_op, "Unknown token type in ternary operator.");
}
