#pragma once

#include <unordered_map>
#include <memory>

#include "class.hpp"

class Instance : public std::enable_shared_from_this<Instance>
{
public:
    Instance(ClassPtr);

    std::string to_string();

    virtual Token::Value get_field(const Token &name, Interpreter &);

    virtual void set_field(const Token &name, Token::Value);

private:
    // Field are more general than properties. A field is anything defined on an instance, like a method or property
    std::unordered_map<std::string, Token::Value> fields;

    ClassPtr klass;
};