#pragma once
#include "expr.hpp"
#include "visitor.hpp"
#include <vector>

struct Statement
{
  virtual ~Statement();
  virtual void print(std::ostream &os) const = 0;
};
using stmt = std::unique_ptr<Statement>;

template <int id, typename... Types>
struct StmtProduction;

enum class FunctionKind
{
  FUNCTION,
  METHOD,
  UNBOUND,
  CONSTRUCTOR,
  LAMDBDA,
  GETTER,
};

std::string str(FunctionKind);

std::ostream &operator<<(std::ostream &, FunctionKind);

// clang-format off
using PrintStmt = StmtProduction<0, expr>;                                                                 // expression (for printing)
using ExprStmt = StmtProduction<1, expr>;                                                                  // expression
using VarStmt = StmtProduction<2, Token, expr>;                                                            // name initializer
using MalformedStmt = StmtProduction<3, bool, std::string>;                                                // is_critical message
using BlockStmt = StmtProduction<4, std::vector<stmt>>;                                                    // statements
using IfStmt = StmtProduction<5, expr, stmt, stmt>;                                                        //	condition then-stmt	else-stmt
using EmptyStmt = StmtProduction<6>;
using WhileStmt = StmtProduction<7, expr, stmt>;                                                           //	cond body
using FunctionStmt = StmtProduction<8, Token, std::vector<Token>, std::vector<stmt>, FunctionKind>;        // name params body kind
using ReturnStmt = StmtProduction<9, Token, expr>;                                                         // 'return' body
using FunctionStmtPtr = std::unique_ptr<FunctionStmt>;        
using ClassStmt = StmtProduction<10, Token, std::vector<FunctionStmtPtr>>;                                 // name methods
// clang-format on

#define STMT_TYPES PrintStmt, ExprStmt, VarStmt, MalformedStmt, BlockStmt, IfStmt, \
                   EmptyStmt, WhileStmt, FunctionStmt, ReturnStmt, ClassStmt

template <int id, typename... Types>
using StmtVisitable = VisitableImpl<StmtProduction<id, Types...>, STMT_TYPES>;
using StmtVisitor = Visitor<STMT_TYPES>;
using StmtVisitableBase = Visitable<STMT_TYPES>;

/// A production for statements.
/// id is for disambiguation for identical template args
template <int id, typename... Types>
struct StmtProduction : public Statement, StmtVisitable<id, Types...>
{
  explicit StmtProduction(Types &&...args)
      : derivatives(std::forward<Types>(args)...) {}

  void print(std::ostream &os) const override
  {
    os << "Statement: \n\t";
    std::apply([&os](auto &&...args)
               { ((os << args << "\t"), ...); },
               derivatives);
    os << '\n';
  }

  template <size_t index>
  decltype(auto) child()
  {
    auto &elem = std::get<index>(derivatives);
    return elem;
  }

  template <size_t index>
  decltype(auto) child() const
  {
    const auto &elem = std::get<index>(derivatives);
    return elem;
  }

  std::tuple<std::remove_reference_t<Types>...> derivatives;
};

#define DECLARE_STMT_VISIT_METHODS      \
  void visit(VarStmt &) override;       \
  void visit(MalformedStmt &) override; \
  void visit(BlockStmt &) override;     \
  void visit(PrintStmt &) override;     \
  void visit(ExprStmt &) override;      \
  void visit(IfStmt &) override;        \
  void visit(WhileStmt &) override;     \
  void visit(EmptyStmt &) override;     \
  void visit(FunctionStmt &) override;  \
  void visit(ReturnStmt &) override;    \
  void visit(ClassStmt &) override;

std::ostream &operator<<(std::ostream &os, const Statement &rhs);

std::ostream &operator<<(std::ostream &os, const stmt &rhs);
std::ostream &operator<<(std::ostream &os, const std::vector<FunctionStmtPtr> &rhs);
std::ostream &operator<<(std::ostream &os, const std::vector<stmt> &rhs);

template <typename Type, typename... arg_types>
stmt new_stmt(arg_types &&...args)
{
  return std::make_unique<Type>(std::forward<arg_types>(args)...);
}
