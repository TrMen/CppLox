#include "interpreter.hpp"

#include <filesystem>
#include <cassert>

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

Interpreter::CheckedRecursiveDepth::CheckedRecursiveDepth(Interpreter &_interpreter, const Token &location)
    : interpreter(_interpreter)
{
  interpreter.recursion_depth += 1;
  if (interpreter.recursion_depth > MAX_RECURSION_DEPTH)
  {
    interpreter.recursion_depth -= 1;
    throw RuntimeError(location, "Maximum recursion depth reached. Are you recursing without basecase?");
  }
}

Interpreter::CheckedRecursiveDepth::~CheckedRecursiveDepth()
{
  interpreter.recursion_depth -= 1;
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

Token::Value Interpreter::get_evaluated(Expr &expression)
{
  dynamic_cast<ExprVisitableBase &>(expression).accept(*this);
  return last_value;
}

//---------- Helper functions ------------
namespace
{
  template <class... Ts>
  struct overloaded : Ts...
  {
    // Used to combine the operator() from multiple lambdas
    using Ts::operator()...;
  };
  // Explicit deduction guide. Shouldn't be needed for C++20, but doesn't compile without
  template <class... Ts>
  overloaded(Ts...) -> overloaded<Ts...>;

  /* All values except NullType and the bool false are truthy, including "", 0, functions, callables*/
  bool is_truthy(const Token::Value &value)
  {
    // clang-format off
    return std::visit(
        overloaded{[](bool condition) { return condition; },
                   [](NullType) { return false; },
                   [](auto &&) { return true; }
        }, value);
    // clang-format on
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
  function.value = std::make_shared<Function>(&node, environment, node.child<3>());
  environment->define(std::move(function));
}

Class::ClassFunctions Interpreter::split_class_functions(const std::vector<FunctionStmtPtr> &class_functions) const
{
  Class::FunctionMap methods;
  Class::FunctionMap unbounds;
  Class::FunctionMap getters;
  for (const auto &function : class_functions)
  {
    const auto &kind = function->child<3>();
    switch (kind)
    {
    case FunctionKind::METHOD:
    case FunctionKind::CONSTRUCTOR:
    {
      // Every AST node method becomes a runtime function that captures the envrionment
      // This allows methods to keep being associated with their original objects
      methods.emplace(function->child<0>().lexeme,
                      std::make_shared<Function>(function.get(), environment, kind));
      break;
    }
    case FunctionKind::UNBOUND:
    {
      unbounds.emplace(function->child<0>().lexeme,
                       std::make_shared<Function>(function.get(), environment, kind));
      break;
    }
    case FunctionKind::GETTER:
    {
      getters.emplace(function->child<0>().lexeme,
                      std::make_shared<Function>(function.get(), environment, kind));
      break;
    }
    default:
    {
      LOG_ERROR("Invalid kind: ", kind);
      assert(false);
    }
    }
  }
  return {std::move(methods), std::move(unbounds), std::move(getters)};
}

void Interpreter::visit(ClassStmt &node)
{
  auto klass = node.child<0>();

  auto &superclass_expr = node.child<2>();
  ClassPtr superclass = nullptr;
  if (superclass_expr != nullptr)
  {
    superclass = get_callable_as<Class>(get_evaluated(*superclass_expr));
    if (superclass == nullptr)
      throw new RuntimeError(superclass_expr->child<0>(), "Superclass must be a class.");

    environment = std::make_shared<Environment>(environment);
    environment->define("super", superclass); // Unlike 'this', super is defined once per class
  }

  klass.value = std::make_shared<Class>(node.child<0>().lexeme, std::move(superclass),
                                        split_class_functions(node.child<1>()));

  if (superclass_expr != nullptr)
    environment = environment->enclosing; // Pop the 'super' environment

  environment->define(std::move(klass));
}

void Interpreter::visit(Super &node)
{
  const auto &name = node.child<1>().lexeme;

  if (node.child<2>()) // In unbound method
  {
    // This is a horrible hack. The environment with 'this' doesn't exist in unbound methods,
    // so we have to look one further up.
    const auto superclass = get_callable_as<Class>(environment->get_at(*node.depth - 1, "super"));

    if (auto unbound = superclass->get_unbound(name))
    {
      last_value = std::move(unbound);
      return;
    }
    else
      throw RuntimeError(node.child<1>(), "Undefined unbound method. You can only access unbound super methods in an unbound submethod.");
  }

  // 'this' needs to still be bound to the original object, even though we use a superclass method
  auto object = std::get<InstancePtr>(environment->get_at(*node.depth - 1, "this"));
  const auto superclass = get_callable_as<Class>(environment->get_at(*node.depth, "super"));

  if (const auto &method = superclass->get_method(name))
  {
    last_value = method->bind(std::move(object));
  }
  else if (auto unbound = superclass->get_unbound(name))
  {
    last_value = std::move(unbound);
  }
  else if (const auto &getter = superclass->get_getter(name))
  {
    last_value = getter->bind(std::move(object))->call(*this, {});
  }
  else
  {
    throw RuntimeError(node.child<1>(),
                       "Undefined method or unbound function '" + node.child<1>().lexeme +
                           "' on class '" + superclass->name + '.');
  }
}

void Interpreter::visit(IfStmt &node)
{
  if (is_truthy(get_evaluated(node.child<0>())))
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
  // This will correctly return NullType when the initializer is Empty
  variable.value = get_evaluated(node.child<1>());

  environment->define(std::move(variable));
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

  last_value = std::make_shared<Function>(&node, environment, FunctionKind::LAMDBDA);
}

void Interpreter::visit(Call &node)
{
  auto callee = get_evaluated(node.child<0>());

  if (!std::holds_alternative<CallablePtr>(callee))
    throw RuntimeError(node.child<1>(), "Can only call functions and classes.");

  const auto &callable = std::get<CallablePtr>(callee);

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

  Interpreter::CheckedRecursiveDepth recursionCheck{*this, node.child<1>()};

  LOG_DEBUG("Calling callable in visit(Call): ", callable->to_string());
  last_value = callable->call(*this, arguments);
}

void Interpreter::visit(Get &node)
{
  auto object = get_evaluated(node.child<0>());

  if (const auto *obj = std::get_if<InstancePtr>(&object))
  {
    last_value = (*obj)->get_field(node.child<1>(), *this);
  }
  else if (const auto klass = get_callable_as<Class>(object))
  {
    last_value = klass->get_unbound(node.child<1>().lexeme);
    if (get_callable_as<Function>(last_value) == nullptr)
    {
      throw RuntimeError(node.child<1>(), "Undefined unbound function.");
    }
  }
  else
  {
    throw RuntimeError(node.child<1>(), "Can only access fields of objects or classes. Called with: " + stringify(object));
  }
}

void Interpreter::visit(Set &node)
{
  auto object = get_evaluated(node.child<0>());

  if (!std::holds_alternative<InstancePtr>(object))
  {
    throw RuntimeError(node.child<1>(), "Can only set properties on objects");
  }

  auto value = get_evaluated(node.child<2>());

  std::get<InstancePtr>(object)->set_field(node.child<1>(), value);

  last_value = std::move(value);
}

void Interpreter::visit(This &node)
{
  last_value = lookup_variable(node.child<0>(), node);
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
  LOG_DEBUG("Getting variable: ", node.child<0>().lexeme, " at depth ", environment->depth());
  LOG_DEBUG(environment->to_string_recursive());
  last_value = lookup_variable(node.child<0>(), node);
}

const Token::Value &Interpreter::lookup_variable(const Token &name, const Expr &node) const
{
  if (node.depth.has_value())
  {
    return environment->get_at(*node.depth, name.lexeme);
  }
  else
  {
    return globals->get(name);
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
