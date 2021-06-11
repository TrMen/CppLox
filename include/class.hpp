#pragma once

#include "callable.hpp"

class Class : public Callable, public std::enable_shared_from_this<Class>
{
public:
    Class(std::string name);

    Token::Value call(Interpreter &, const std::vector<Token::Value> &arguments) override;

    std::string to_string() const override;

    size_t arity() const override;

    const std::string name;
};