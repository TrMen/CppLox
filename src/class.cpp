#include "class.hpp"

#include "instance.hpp"
#include "logging.hpp"
#include "error.hpp"

Class::Class(std::string _name, FunctionMap _methods, FunctionMap _unbounds)
    : name(std::move(_name)), methods(std::move(_methods)), unbounds(std::move(_unbounds))
{
}

std::string Class::to_string() const
{
    std::string representation = "class " + name + "\nMethods:";
    for (const auto &method : methods)
    {
        representation += "\n\t" + method.first;
    }
    representation += "\nUnbound functions:";
    for (const auto &unbound : unbounds)
    {
        representation += "\n\t" + unbound.first;
    }

    return representation;
}

size_t Class::arity() const
{
    if (auto constructor = get_method("init"))
    {
        return constructor->arity();
    }
    return 0;
}

Token::Value Class::call(Interpreter &interpreter, const std::vector<Token::Value> &arguments)
{
    LOG_DEBUG("Creating instance");

    auto instance = std::make_shared<Instance>(shared_from_this());

    LOG_DEBUG("Created instance successfully");

    // Run constructor method when class is called. Class-call args become constructor args
    if (auto constructor = get_method("init"))
    {
        constructor->bind(instance)->call(interpreter, arguments);
    }

    return instance;
}

std::shared_ptr<Function> Class::get_method(const std::string &name) const
{
    if (methods.contains(name))
    {
        return methods.at(name);
    }

    LOG_WARNING("Unknown method ", name, ". Existing methods: ");
    for (const auto &method : methods)
    {
        LOG_WARNING(method.first, ": ", method.second->to_string());
    }

    return nullptr;
}

std::shared_ptr<Function> Class::get_unbound(const Token &name) const
{
    if (unbounds.contains(name.lexeme))
    {
        return unbounds.at(name.lexeme);
    }

    LOG_WARNING("Unknown unbound function ", name, " for class ", this->name, ". Existing unbound functions: ");
    for (const auto &function : unbounds)
    {
        LOG_WARNING(function.first, ": ", function.second->to_string());
    }

    throw RuntimeError(name, "Undefined unbound method for class " + this->name);
}