#pragma once

#include "interpreter.hpp"

class Resolver : public ExprVisitor, public StmtVisitor
{
public:
    Resolver(Interpreter &);

    void resolve(const std::vector<stmt> &);
    void resolve(const stmt &);

private:
    DECLARE_STMT_VISIT_METHODS

    DECLARE_EXPR_VISIT_METHODS

    void resolve(const expr &);

    void declare(const Token &identifier);
    void define(const Token &identifier);

    enum class FunctionType
    {
        NONE,
        FUNCTION,
        METHOD
    };

    enum class ClassType
    {
        NONE,
        CLASS,
    };

    void resolve_local(Expr &node, const Token &identifier);
    void resolve_function(const std::vector<Token> &params, const std::vector<stmt> &body, FunctionType);

    Interpreter &interpreter;

    // The bool represents whether the variable is initialized
    std::vector<std::unordered_map<std::string, bool>> scopes;

    FunctionType function_type = FunctionType::NONE;
    ClassType class_type = ClassType::NONE;
};
