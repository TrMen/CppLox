#include "stmt.hpp"

#include <cassert>

Statement::~Statement() = default;

std::string str(FunctionKind kind)
{
    switch (kind)
    {
    case FunctionKind::FUNCTION:
        return "function";
    case FunctionKind::METHOD:
        return "method";
    case FunctionKind::UNBOUND:
        return "unbound class function";
    case FunctionKind::CONSTRUCTOR:
        return "constructor";
    case FunctionKind::LAMDBDA:
        return "lambda";
    case FunctionKind::GETTER:
        return "getter";
    }
    return "";
}

std::ostream &operator<<(std::ostream &os, FunctionKind kind)
{
    return os << str(kind);
}

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

std::ostream &operator<<(std::ostream &os, const std::vector<FunctionStmtPtr> &rhs)
{
    for (const auto &stmt : rhs)
    {
        stmt->print(os);
    }

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