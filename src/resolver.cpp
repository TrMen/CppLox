#include "resolver.hpp"

#include "logging.hpp"

Resolver::Resolver(Interpreter &_interpreter)
    : interpreter(_interpreter)
{
}

void Resolver::resolve(const expr &expression)
{
    dynamic_cast<ExprVisitableBase &>(*expression).accept(*this);
}

void Resolver::resolve(const stmt &statement)
{
    try
    {
        dynamic_cast<StmtVisitableBase &>(*statement).accept(*this);
    }
    catch (const CompiletimeError &err)
    {
        interpreter.err_handler->error(err.token, err.what());
    }
}

void Resolver::resolve(const std::vector<stmt> &statements)
{
    for (const auto &statement : statements)
    {
        resolve(statement);
    }
}

void Resolver::declare(const Token &identifier)
{
    if (!scopes.empty())
    {
        if (not scopes.back().emplace(identifier.lexeme, false).second)
        {
            throw CompiletimeError(identifier, "Variable with this name is already declared in this scope");
        }
    }
}

void Resolver::define(const Token &identifier)
{
    if (!scopes.empty())
    {
        scopes.back().insert_or_assign(identifier.lexeme, true);
    }
}

void Resolver::visit(BlockStmt &node)
{
    scopes.emplace_back();
    resolve(node.child<0>());
    scopes.pop_back();
}

void Resolver::visit(VarStmt &node)
{
    declare(node.child<0>());

    resolve(node.child<1>());

    define(node.child<0>());
}

void Resolver::visit(Variable &node)
{
    LOG_DEBUG("Resolving variable usage");

    // Var exists in current scope and is uninitialized -> We are currently declaring this variable
    if (not scopes.empty() && scopes.back().contains(node.child<0>().lexeme) && not scopes.back().at(node.child<0>().lexeme))
    {
        throw CompiletimeError(node.child<0>(), "Can't read local variable in its own initializer.");
    }

    // Otherwise, it might exist somewhere in an outer scope (global if no scopes exist, or in an outer local scope)
    resolve_local(node, node.child<0>());
}

void Resolver::resolve_local(Expr &node, const Token &identifier)
{
    LOG_DEBUG("Resolving local for ", identifier.lexeme);

    for (const auto &scope : scopes)
    {
        for (const auto &pair : scope)
        {
            LOG_DEBUG(pair.first, ": ", pair.second);
        }
        Logging::newline(Logging::LogLevel::debug);
    }

    for (int i = scopes.size() - 1; i >= 0; --i)
    {
        if (scopes.at(i).contains(identifier.lexeme))
        {
            LOG_DEBUG("Setting depth up for ", identifier.lexeme, " at ", scopes.size() - 1 - i);
            // Save the depth information in the AST node for usage by the interpreter
            node.depth = scopes.size() - 1 - i;
            return;
        }
    }
    // In fall-through case, the variable is not local -> must be global or undefined.
    // Depth information is not saved in the AST
}

void Resolver::visit(Assign &node)
{
    resolve(node.child<1>());

    resolve_local(node, node.child<0>());
}

void Resolver::visit(FunctionStmt &node)
{
    // Declare the name eagerly, to allow functions to recurisvely
    // refer to their names in their bodies
    declare(node.child<0>());
    define(node.child<0>());

    resolve_function(node.child<1>(), node.child<2>(), FunctionType::FUNCTION);
}

void Resolver::resolve_function(const std::vector<Token> &params, const std::vector<stmt> &body, FunctionType type)
{
    auto enclosing_function = current_function;
    current_function = type;

    scopes.emplace_back();

    for (const auto &param : params)
    {
        declare(param);
        define(param);
    }

    // Needs two scopes: One for function (where parameters live) and one for the body block (where locals live)
    // This separation isn't quite necessary, but a consequence of my block always creating a new env to execute in
    // The parameters live in the first block, so to be correctly looked up at interpretation, we need to simulate that
    // inner block here as well.
    scopes.emplace_back();

    resolve(body);

    scopes.pop_back();
    scopes.pop_back();

    current_function = enclosing_function;
}

void Resolver::visit(Lambda &node)
{
    resolve_function(node.child<0>(), node.child<1>(), FunctionType::FUNCTION);
}

void Resolver::visit(ReturnStmt &node)
{
    if (current_function == FunctionType::NONE)
    {
        throw CompiletimeError(node.child<0>(), "Can't return from top-level code");
    }
    resolve(node.child<1>());
}

void Resolver::visit(ClassStmt &node)
{
    declare(node.child<0>());
    define(node.child<0>());

    for (const auto &method : node.child<1>())
    {
        resolve_function(method->child<1>(), method->child<2>(), FunctionType::METHOD);
    }
}

// ----------------------Remaining visit impls that do nothing interesting---------------------------------

void Resolver::visit(ExprStmt &node)
{
    resolve(node.child<0>());
}

void Resolver::visit(IfStmt &node)
{
    resolve(node.child<0>());
    resolve(node.child<1>());
    // Resolution is different from execution. Else must always be resolved (Empty correctly does nothing)
    resolve(node.child<2>());
}

void Resolver::visit(PrintStmt &node)
{
    resolve(node.child<0>());
}

void Resolver::visit(WhileStmt &node)
{
    resolve(node.child<0>());
    // Like if, always resolve body of while
    resolve(node.child<1>());
}

void Resolver::visit(Binary &node)
{
    resolve(node.child<0>());
    resolve(node.child<2>());
}

void Resolver::visit(Call &node)
{
    resolve(node.child<0>());
    // Like if, always resolve body of while
    for (const auto &argument : node.child<2>())
    {
        resolve(argument);
    }
}

void Resolver::visit(Grouping &node)
{
    resolve(node.child<0>());
}

void Resolver::visit(Literal &)
{
}

void Resolver::visit(Logical &node)
{
    resolve(node.child<0>());
    resolve(node.child<2>());
}

void Resolver::visit(Unary &node)
{
    resolve(node.child<1>());
}

void Resolver::visit(Ternary &node)
{
    resolve(node.child<0>());
    resolve(node.child<2>());
    resolve(node.child<4>());
}

void Resolver::visit(Malformed &)
{
}

void Resolver::visit(Empty &)
{
}

void Resolver::visit(MalformedStmt &)
{
}

void Resolver::visit(EmptyStmt &)
{
}

void Resolver::visit(Get &node)
{
    resolve(node.child<0>());
}

void Resolver::visit(Set &node)
{
    resolve(node.child<0>());

    // Note, the property is dynamically-evaluated, so no variable is introduced for name here

    resolve(node.child<2>());
}