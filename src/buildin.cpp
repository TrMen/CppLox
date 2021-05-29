#include "buildin.hpp"
#include <chrono>
#include "logging.hpp"
#include "error.hpp"
#include "callable.hpp"
#include "interpreter.hpp"
#include <unordered_map>

namespace
{
    using namespace std::chrono;

    /// Build-in function with 0 parameters
    template <typename Closure>
    struct SimpleBuildin : public Callable
    {
        SimpleBuildin(std::string _name, Closure _action)
            : name(std::move(_name)), action(std::move(_action)) {}

        Token::Value call(Interpreter &interpreter, const std::vector<Token::Value> &) override
        {
            return Token::Value(action(interpreter));
        }

        size_t arity() const override { return 0; }

        std::string to_string() const override { return "<Native fn '" + name + "'>"; }

    private:
        const std::string name;
        Closure action;
    };

    struct SetLogLevel : public Callable
    {
        Token::Value call(Interpreter &,
                          const std::vector<Token::Value> &arguments) override
        {
            using Logging::LogLevel;
            const auto &log_level = arguments[0];

            static const std::unordered_map<std::string, LogLevel> str_to_log_level{
                {"error", LogLevel::error},
                {"warning", LogLevel::warning},
                {"info", LogLevel::info},
                {"debug", LogLevel::debug},
            };

            if (!std::holds_alternative<std::string>(log_level) || str_to_log_level.count(std::get<std::string>(log_level)) == 0)
            {
                const Token error_token{Token::TokenType::FUN, to_string(), NullType{}, 0};
                throw RuntimeError(error_token, "Must be called with one of: ['error', 'warning', 'info', 'debug']");
            }

            Logging::set_log_level(str_to_log_level.at(std::get<std ::string>(log_level)));
            return NullType{};
        }

        size_t arity() const override { return 1; }

        std::string to_string() const override { return "<Native fn 'setLogLevel'>"; }
    };
}

namespace Buildin
{

    std::vector<Token> get_buildins()
    {
        using Type = Token::TokenType;

        auto clock_closure =
            [](Interpreter &)
        {
            return static_cast<double>(duration_cast<seconds>(system_clock::now().time_since_epoch())
                                           .count());
        };
        auto clock_buildin = std::make_shared<SimpleBuildin<decltype(clock_closure)>>("clock", std::move(clock_closure));

        auto print_env_closure = [](Interpreter &interpreter)
        {
            interpreter.out_stream << interpreter.environment;
            return NullType{};
        };
        auto print_env_buildin = std::make_shared<SimpleBuildin<decltype(print_env_closure)>>("print_env", std::move(print_env_closure));

        auto exit_closure = [](Interpreter &)
        {
            throw Exit("Exit called by buildin exit()");
            return NullType{};
        };
        auto exit_buildin = std::make_shared<SimpleBuildin<decltype(exit_closure)>>("exit", std::move(exit_closure));

        return {
            {Type::FUN, "clock", std::move(clock_buildin), 0},
            {Type::FUN, "printEnv", std::move(print_env_buildin), 0},
            {Type::FUN, "exit", std::move(exit_buildin), 0},
            {Type::FUN, "setLogLevel", std::make_shared<SetLogLevel>(), 0},
        };
    }
}