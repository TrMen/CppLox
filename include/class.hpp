#pragma once

#include "function.hpp"

class Class : public Callable, public std::enable_shared_from_this<Class>
{
public:
    Class(std::string _name, std::unordered_map<std::string, std::shared_ptr<Function>> _methods);

    Token::Value call(Interpreter &, const std::vector<Token::Value> &arguments) override;

    std::string to_string() const override;

    size_t arity() const override;

    std::shared_ptr<Function> get_method(const std::string &name);

    const std::string name;

private:
    std::unordered_map<std::string, std::shared_ptr<Function>> methods;
};