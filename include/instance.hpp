#pragma once

#include <unordered_map>
#include <memory>

#include "class.hpp"

class Instance : public std::enable_shared_from_this<Instance>
{
public:
    Instance(std::shared_ptr<Class>);

    std::string to_string();

    Token::Value get_field(const Token &name);

    void set_field(const Token &name, Token::Value);

private:
    // Field are more general than properties. A field is anything defined on an instance, like a method or property
    std::unordered_map<std::string, Token::Value> fields;

    std::shared_ptr<Class> klass;
};