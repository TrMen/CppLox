#pragma once

#include "function.hpp"

class Class : public Callable, public std::enable_shared_from_this<Class>
{
public:
    using FunctionMap = std::unordered_map<std::string, FunctionPtr>;
    /// (methods, unbounds, getters)
    using ClassFunctions = std::tuple<Class::FunctionMap, Class::FunctionMap, Class::FunctionMap>;

    Class(std::string _name, ClassPtr superclass, ClassFunctions);

    Token::Value call(Interpreter &, const std::vector<Token::Value> &arguments) override;

    std::string to_string() const override;

    size_t arity() const override;

    const FunctionPtr &get_method(const std::string &name) const;

    const FunctionPtr &get_unbound(const std::string &name) const;

    const FunctionPtr &get_getter(const std::string &name) const;

    const std::string name;

private:
    ClassPtr superclass;

    FunctionMap methods;
    FunctionMap unbounds;
    FunctionMap getters;

    const FunctionPtr nullRef = nullptr;
};