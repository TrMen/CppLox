#include "instance.hpp"

#include "logging.hpp"
#include "error.hpp"

Instance::Instance(std::shared_ptr<Class> _klass)
    : klass(std::move(_klass)) {}

std::string Instance::to_string()
{
    return klass->name + " instance";
}

Token::Value Instance::get_field(const Token &name) const
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

    auto method = klass->get_method(name.lexeme);
    if (method != nullptr)
    {
        return method;
    }

    throw RuntimeError(name, "Property " + name.lexeme + " is not defined");
}

void Instance::set_field(const Token &name, Token::Value value)
{
    fields[name.lexeme] = value;
}