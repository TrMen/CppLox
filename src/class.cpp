#include "class.hpp"

#include "instance.hpp"

Class::Class(std::string _name)
    : name(std::move(_name))
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