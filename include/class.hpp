#pragma once

#include "function.hpp"

class Class : public Callable, public std::enable_shared_from_this<Class>
{
public:
    using FunctionMap = std::unordered_map<std::string, std::shared_ptr<Function>>;

    Class(std::string _name, FunctionMap _methods, FunctionMap unbounds, FunctionMap getters);

    Token::Value call(Interpreter &, const std::vector<Token::Value> &arguments) override;

    std::string to_string() const override;

    size_t arity() const override;

    std::shared_ptr<Function> get_method(const std::string &name) const;

    std::shared_ptr<Function> get_unbound(const Token &name) const;

    std::shared_ptr<Function> get_getter(const std::string &name) const;

    const std::string name;

private:
    FunctionMap methods;
    FunctionMap unbounds;
    FunctionMap getters;
};