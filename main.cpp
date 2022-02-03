#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

#include "error.hpp"
#include "expr.hpp"
#include "interpreter.hpp"
#include "lexer.hpp"
#include "logging.hpp"
#include "parser.hpp"
#include "resolver.hpp"

static void log_tokens(const std::vector<Token> &tokens) {
  LOG_DEBUG("\nTokens after parse:");

  for (const auto &t : tokens) {
    LOG_DEBUG("\t", t);
  }

  LOG_DEBUG("\n");
}

static std::vector<stmt>
run(const std::string &source, const std::shared_ptr<ErrorHandler> &err_handler,
    std::optional<std::string> filename = std::nullopt) {
  static Interpreter interpreter{std::cout, err_handler};

  if (filename.has_value()) {
    interpreter.interpreter_path =
        std::filesystem::path(*filename).remove_filename();
  }

  Lexer lexer{source, err_handler};
  std::vector<Token> tokens = lexer.lex();

  if (err_handler->has_error()) {
    return {};
  }

  Parser parser{tokens, err_handler};
  std::vector<stmt> statements = parser.parse();

  log_tokens(tokens);

  if (err_handler->has_error()) {
    return {};
  }

  Resolver resolver{interpreter};
  resolver.resolve(statements);

  if (err_handler->has_error()) {
    return {};
  }

  if (err_handler->has_error()) {
    return {};
  }

  try {
    interpreter.interpret(statements);

    if (err_handler->has_runtime_error()) {
      return {};
    }
  } catch (const Exit &e) {
    LOG_INFO("Interpretation terminated: ", e.what());
    std::exit(0);
  }

  Logging::newline(Logging::LogLevel::DEBUG);

  return statements;
}

static int run_prompt(const std::shared_ptr<ErrorHandler> &err_handler) {
  std::string line{};

  // Save statements so the AST of previous prompt inputs stays alive. Required
  // for proper handling of function declarations across input lines
  std::vector<stmt> run_statements;

  while (true) {
    std::cout << "> ";
    std::getline(std::cin, line);
    if (std::cin.eof()) {
      return 0;
    }

    auto newly_run_statements = run(line, err_handler);
    run_statements.insert(run_statements.end(),
                          std::make_move_iterator(newly_run_statements.begin()),
                          std::make_move_iterator(newly_run_statements.end()));

    err_handler->reset_error();
  }
}

static int run_file(const char *filename,
                    const std::shared_ptr<ErrorHandler> &err_handler) {
  std::ifstream ifstr(filename);
  std::stringstream ss{};
  ss << ifstr.rdbuf();

  if (!ifstr) {
    LOG_ERROR("File ", filename, " could not be opened");
    return 42;
  }

  run(ss.str(), err_handler, filename);
  if (err_handler->has_error()) {
    return 65;
  }
  if (err_handler->has_runtime_error()) {
    return 70;
  }
  return 0;
}

int main(int argc, char *argv[]) {
  std::setprecision(3);
  Logging::set_log_level(Logging::LogLevel::ERROR);
  if (argc > 2) {
    std::cout << "Usage: Lox [script]";
    return 64;
  }

  auto err_handler{std::make_shared<CerrHandler>()};

  if (argc == 2) {
    return run_file(argv[1], err_handler);
  }
  return run_prompt(err_handler);
}
