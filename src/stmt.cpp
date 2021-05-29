#include "stmt.hpp"

Statement::~Statement() = default;

std::ostream &operator<<(std::ostream &os, const Statement &rhs)
{
    rhs.print(os);
    return os;
}

std::ostream &operator<<(std::ostream &os, const stmt &rhs)
{
    rhs->print(os);
    return os;
}

std::ostream &operator<<(std::ostream &os, const std::vector<stmt> &rhs)
{
    for (const auto &stmt : rhs)
    {
        os << stmt;
    }

    return os;
}