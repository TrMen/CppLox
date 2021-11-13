#include "buildin.hpp"

#include <chrono>
#include <filesystem>
#include <sstream>
#include <unordered_map>

#include "callable.hpp"
#include "error.hpp"
#include "interpreter.hpp"
#include "lexer.hpp"
#include "logging.hpp"
#include "parser.hpp"
#include "resolver.hpp"

namespace {
/// Build-in function with 0 parameters
template <typename Closure> struct SimpleBuildin : public Callable {
public:
  SimpleBuildin(std::string _name, Closure _action)
      : name(std::move(_name)), action(std::move(_action)) {}

  Token::Value call(Interpreter &interpreter,
                    const std::vector<Token::Value> &) override {
    return Token::Value(action(interpreter));
  }

  [[nodiscard]] size_t arity() const override { return 0; }

  [[nodiscard]] std::string to_string() const override {
    return "<Native fn '" + name + "'>";
  }

private:
  const std::string name;
  Closure action;
};

struct SetLogLevel : public Callable {
public:
  Token::Value call(Interpreter &,
                    const std::vector<Token::Value> &arguments) override {
    using Logging::LogLevel;
    const auto &log_level = arguments[0];

    static const std::unordered_map<std::string, LogLevel> str_to_log_level{
        {"error", LogLevel::ERROR},
        {"warning", LogLevel::WARNING},
        {"info", LogLevel::INFO},
        {"debug", LogLevel::DEBUG},
    };

    if (!std::holds_alternative<std::string>(log_level) ||
        str_to_log_level.count(std::get<std::string>(log_level)) == 0) {
      const Token error_token{Token::TokenType::FUN, to_string(), NullType{},
                              0};
      throw RuntimeError(
          error_token,
          "Must be called with one of: ['error', 'warning', 'info', 'debug']");
    }

    Logging::set_log_level(
        str_to_log_level.at(std::get<std ::string>(log_level)));
    return NullType{};
  }

  [[nodiscard]] size_t arity() const override { return 1; }

  [[nodiscard]] std::string to_string() const override {
    return "<Native fn 'setLogLevel'>";
  }
};

struct Eval : public Callable {
public:
  Token::Value call(Interpreter &interpreter,
                    const std::vector<Token::Value> &arguments) override {
    using Logging::LogLevel;
    const auto &source = arguments[0];

    if (!std::holds_alternative<std::string>(source)) {
      throw RuntimeError(
          source,
          "eval()'s first argument must be a string containing the source code",
          0);
    }

    Lexer lexer{std::get<std::string>(source), interpreter.err_handler};
    auto tokens = lexer.lex();
    if (interpreter.err_handler->has_error()) {
      return NullType{}; // Error already reported, but eval needs to be stopped
    }

    Parser parser{tokens, interpreter.err_handler};
    auto statements = parser.parse();

    if (interpreter.err_handler->has_error()) {
      return NullType{};
    }

    Resolver resolver{interpreter};
    resolver.resolve(statements);

    if (interpreter.err_handler->has_error()) {
      return NullType{};
    }

    interpreter.interpret(statements);
    return interpreter.last_value;
  }

  [[nodiscard]] size_t arity() const override { return 1; }

  [[nodiscard]] std::string to_string() const override {
    return "<Native fn 'eval'>";
  }
};

struct IncludeStr : public Callable {
public:
  Token::Value call(Interpreter &interpreter,
                    const std::vector<Token::Value> &arguments) override {
    using Logging::LogLevel;
    const auto &filename = arguments[0];

    if (!std::holds_alternative<std::string>(filename)) {
      throw RuntimeError(
          filename,
          "must be a string that specifies the name of the file to include", 0);
    }

    LOG_DEBUG("Currently interpreted path: ", interpreter.interpreter_path);

    auto file = std::filesystem::path(interpreter.interpreter_path)
                    .append(std::get<std::string>(filename));

    LOG_DEBUG("Requested file for includeStr(): ", file);

    std::ifstream ifs{file};

    std::stringstream buffer;
    buffer << ifs.rdbuf();

    if (!ifs) {
      throw RuntimeError(
          filename, "There was an error reading the file for includeStr()", 0);
    }

    return buffer.str();
  }

  [[nodiscard]] size_t arity() const override { return 1; }

  [[nodiscard]] std::string to_string() const override {
    return "<Native fn 'includeStr'>";
  }
};

struct Assert : public Callable {
public:
  Token::Value call(Interpreter &,
                    const std::vector<Token::Value> &arguments) override {
    using Logging::LogLevel;
    const auto &condition = arguments[0];
    const auto &message = arguments[1];

    if (!std::holds_alternative<bool>(condition)) {
      throw RuntimeError(condition,
                         "must be a boolean expression that is asserted", 0);
    }

    if (!std::holds_alternative<std::string>(message)) {
      throw RuntimeError(message,
                         "must be a string that specifies what went wrong", 0);
    }

    if (!std::get<bool>(condition)) {
      throw RuntimeError(condition, std::get<std::string>(message), 0);
    }

    return NullType{};
  }

  [[nodiscard]] size_t arity() const override { return 2; }

  [[nodiscard]] std::string to_string() const override {
    return "<Native fn 'assert'>";
  }
};
} // namespace

namespace Buildin {

std::vector<Token> get_buildins() {
  using Type = Token::TokenType;

  auto clock_closure = [](Interpreter &) {
    return static_cast<double>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
  };
  auto clock_buildin = std::make_shared<SimpleBuildin<decltype(clock_closure)>>(
      "clock", std::move(clock_closure));

  auto print_env_closure = [](Interpreter &interpreter) {
    interpreter.out_stream << "Globals: \n"
                           << *interpreter.globals << std::endl;
    interpreter.out_stream << "Locals: \n"
                           << *interpreter.environment << std::endl;
    return NullType{};
  };
  auto print_env_buildin =
      std::make_shared<SimpleBuildin<decltype(print_env_closure)>>(
          "print_env", std::move(print_env_closure));

  auto exit_closure = [](Interpreter &) {
    throw Exit("Exit called by buildin exit()");
    return NullType{};
  };
  auto exit_buildin = std::make_shared<SimpleBuildin<decltype(exit_closure)>>(
      "exit", std::move(exit_closure));

  return {
      {Type::FUN, "clock", std::move(clock_buildin), 0},
      {Type::FUN, "printEnv", std::move(print_env_buildin), 0},
      {Type::FUN, "exit", std::move(exit_buildin), 0},
      {Type::FUN, "includeStr", std::make_shared<IncludeStr>(), 0},
      {Type::FUN, "setLogLevel", std::make_shared<SetLogLevel>(), 0},
      {Type::FUN, "assert", std::make_shared<Assert>(), 0},
      {Type::FUN, "eval", std::make_shared<Eval>(), 0},
  };
}
} // namespace Buildin
