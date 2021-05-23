#include "error.hpp"
#include "expr.hpp"
#include "interpreter.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>
#include "logging.hpp"

static std::shared_ptr<ErrorHandler> err_handler;

static void run(const std::string &source)
{
  if (!err_handler)
  {
    err_handler = std::make_shared<CerrHandler>();
  }
  static Interpreter interpreter{err_handler};

  Lexer lexer{source, err_handler};
  std::vector<Token> tokens = lexer.lex();

  if (err_handler->has_error())
  {
    return;
  }

  Parser parser{tokens, err_handler};
  std::vector<stmt> statements = parser.parse();

  LOG_DEBUG("\nTokens after parse:");

  for (const auto &t : tokens)
  {
    LOG_DEBUG("\t", t);
  }

  LOG_DEBUG("\n");

  if (err_handler->has_error())
  {
    return;
  }

  try
  {
    interpreter.interpret(statements);
  }
  catch (const Exit &e)
  {
    LOG_INFO("Interpretation terminated: ", e.what());
    std::exit(0);
  }

  Logging::newline();
}

static int run_prompt()
{
  std::string line{};

  while (true)
  {
    std::cout << "> ";
    std::getline(std::cin, line);
    if (std::cin.eof())
    {
      return 0;
    }

    run(line);
    err_handler->reset_error();
  }
}

static int run_file(const char *filename)
{
  std::ifstream ifstr(filename);
  std::stringstream ss{};
  ss << ifstr.rdbuf();

  run(ss.str());
  if (err_handler->has_error())
  {
    return 65;
  }
  if (err_handler->has_runtime_error())
  {
    return 70;
  }
  return 0;
}

int main(int argc, char *argv[])
{
  std::setprecision(3);
  Logging::set_log_level(Logging::LogLevel::debug);
  if (argc > 2)
  {
    std::cout << "Usage: scip [script]";
    return 64;
  }
  if (argc == 2)
  {
    return run_file(argv[1]);
  }
  return run_prompt();
}
