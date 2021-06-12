#include "class.hpp"

#include "instance.hpp"
#include "logging.hpp"

Class::Class(std::string _name, std::unordered_map<std::string, std::shared_ptr<Function>> _methods)
    : name(std::move(_name)), methods(std::move(_methods))
{
}

std::string Class::to_string() const
{
    return name;
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
    auto instance = std::make_shared<Instance>(shared_from_this());

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