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
    return 0;
}

Token::Value Class::call(Interpreter &, const std::vector<Token::Value> &)
{
    return std::make_shared<Instance>(shared_from_this());
}

std::shared_ptr<Function> Class::get_method(const std::string &name)
{
    if (methods.contains(name))
    {
        return methods.at(name);
    }

    LOG_WARNING("Unknown method ", name, ". Existing methods: ");
    for (const auto &method : methods)
    {
        LOG_WARNING(method.first, ": ", method.second);
    }

    return nullptr;
}