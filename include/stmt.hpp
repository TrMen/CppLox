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

using Print = StmtProduction<0, expr>;      //	expression (for printing)
using StmtExpr = StmtProduction<1, expr>;   //	expression
using Var = StmtProduction<2, Token, expr>; // id name initializer
using MalformedStmt =
    StmtProduction<3, bool, std::string>;           //	is_critical message
using Block = StmtProduction<4, std::vector<stmt>>; //	statements
using IfStmt = StmtProduction<5, expr, stmt,
                              stmt>; //	condition then-stmt	else-stmt
using EmptyStmt = StmtProduction<6>;
using WhileStmt = StmtProduction<7, expr, stmt>; //	cond body
using FunctionStmt =
    StmtProduction<8, Token, std::vector<Token>,
                   std::vector<stmt>>; // name params body

#define TYPES Print, StmtExpr, Var, MalformedStmt, Block, IfStmt, EmptyStmt, WhileStmt, FunctionStmt

template <int id, typename... Types>
using StmtVisitable = VisitableImpl<StmtProduction<id, Types...>, TYPES>;
using StmtVisitor = Visitor<TYPES>;
using StmtVisitableBase = Visitable<TYPES>;

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
    os << "\n";
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

std::ostream &operator<<(std::ostream &os, const Statement &rhs);

std::ostream &operator<<(std::ostream &os, const stmt &rhs);
std::ostream &operator<<(std::ostream &os, const std::vector<stmt> &rhs);

template <typename Type, typename... arg_types>
stmt new_stmt(arg_types &&...args)
{
  return std::make_unique<Type>(std::forward<arg_types>(args)...);
}