#pragma once

#include "class.hpp"

class Instance
{
public:
    Instance(std::shared_ptr<Class>);

    std::string to_string();

private:
    std::shared_ptr<Class> klass;
};