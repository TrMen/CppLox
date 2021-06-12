#include "instance.hpp"

#include "logging.hpp"
#include "error.hpp"

Instance::Instance(std::shared_ptr<Class> _klass)
    : klass(std::move(_klass)) {}

std::string Instance::to_string()
{
    return klass->name + " instance";
}

Token::Value Instance::get_field(const Token &name)
{
    if (fields.contains(name.lexeme))
    {
        return fields.at(name.lexeme);
    }

    LOG_WARNING("Undefined property on object with fields: ");
    for (const auto &field : fields)
    {
        LOG_WARNING(field.first, ": ", field.second);
    }

    if (auto method = klass->get_method(name.lexeme))
    {
        // Create new env
        // Bind assign to the name of the variable the method was called on
        // Create a copy of the method surrounded by that environment (called bound method)
        // Call that copy
        return method->bind(shared_from_this());
    }

    throw RuntimeError(name, "Property " + name.lexeme + " is not defined");
}

void Instance::set_field(const Token &name, Token::Value value)
{
    fields[name.lexeme] = value;
}