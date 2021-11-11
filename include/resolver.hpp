#pragma once

#include <optional>

#include "interpreter.hpp"

struct Resolver : public ExprVisitor, public StmtVisitor
{
    explicit Resolver(Interpreter &);

    void resolve(const std::vector<stmt> &);
    void resolve(const stmt &);

private:
    DECLARE_STMT_VISIT_METHODS

    DECLARE_EXPR_VISIT_METHODS

    void resolve(const expr &);
    void resolve(Expr *);

    void declare(const Token &identifier);
    void define(const Token &identifier);

    enum class ClassKind
    {
        NONE,
        CLASS,
        SUBCLASS
    };

    void resolve_local(Expr &node, const Token &identifier);
    void resolve_function(const std::vector<Token> &params, const std::vector<stmt> &body, FunctionKind);

    Interpreter &interpreter;

    // The bool represents whether the variable is initialized
    std::vector<std::unordered_map<std::string, bool>> scopes;

    std::optional<FunctionKind> function_kind = std::nullopt;
    ClassKind class_kind = ClassKind::NONE;

    bool function_needs_return = false;
};
