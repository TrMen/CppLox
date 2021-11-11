#include "instance.hpp"

#include "error.hpp"
#include "interpreter.hpp"
#include "logging.hpp"

Instance::Instance(ClassPtr _klass)
    : klass(std::move(_klass)) {}

std::string Instance::to_string() const
{
    return klass->name() + " instance";
}

Token::Value Instance::get_field(const Token &name, Interpreter &interpreter)
{
    if (const auto &getter = klass->get_getter(name.lexeme))
    {
        Interpreter::CheckedRecursiveDepth recursionCheck{interpreter, name};
        return getter->bind(shared_from_this())->call(interpreter, {});
    }

    if (fields.contains(name.lexeme))
    {
        return fields.at(name.lexeme);
    }

    LOG_WARNING("Undefined property on object with fields: ");
    for (const auto &field : fields)
    {
        LOG_WARNING(field.first, ": ", field.second);
    }

    if (const auto &method = klass->get_method(name.lexeme))
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
    if (klass->get_getter(name.lexeme) != nullptr)
        throw RuntimeError(name, "A getter by this name exists. A property of the same name would be inaccessible");

    fields[name.lexeme] = std::move(value);
}