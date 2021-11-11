#pragma once

#include "function.hpp"

struct Class : public Callable, public std::enable_shared_from_this<Class>
{
    using FunctionMap = std::unordered_map<std::string, FunctionPtr>;
    /// (methods, unbounds, getters)
    using ClassFunctions = std::tuple<Class::FunctionMap, Class::FunctionMap, Class::FunctionMap>;

    Class(std::string _name, ClassPtr superclass, ClassFunctions);

    Token::Value call(Interpreter &, const std::vector<Token::Value> &arguments) override;

    [[nodiscard]] std::string to_string() const override;

    [[nodiscard]] size_t arity() const override;

    [[nodiscard]] const FunctionPtr &get_method(const std::string &name) const;

    [[nodiscard]] const FunctionPtr &get_unbound(const std::string &name) const;

    [[nodiscard]] const FunctionPtr &get_getter(const std::string &name) const;

    [[nodiscard]] const std::string &name() const;

private:
    ClassPtr superclass;

    FunctionMap methods;
    FunctionMap unbounds;
    FunctionMap getters;

    const std::string m_name;

    const FunctionPtr nullRef = nullptr;
};