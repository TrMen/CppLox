#include "instance.hpp"

Instance::Instance(std::shared_ptr<Class> _klass)
    : klass(std::move(_klass)) {}

std::string Instance::to_string()
{
    return klass->name + " instance";
}