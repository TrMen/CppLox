#pragma once

#include <memory>
#include <vector>
#include <optional>

#include "token.hpp"
#include "visitor.hpp"

struct Expr
{
  virtual ~Expr();
  virtual void print(std::ostream &os) const = 0;

  // For resolving scope depth.
  // How many environments out from the current one the correct definition is.
  std::optional<int> depth = std::nullopt;
};
using expr = std::unique_ptr<Expr>;

template <int id, typename... Types>
struct ExprProduction;

class Statement;
using stmt = std::unique_ptr<Statement>;

// ---------------------Alias definitions for convenience---------------------

// clang-format off
using Binary = ExprProduction<0, expr, Token, expr>;                       // expr bin_op expr
using Grouping = ExprProduction<1, expr>;                                  // (expr)
using Literal = ExprProduction<2, Token::Value>;                           // value
using Unary = ExprProduction<3, Token, expr>;                              // unary_op expr
using Ternary = ExprProduction<4, expr, Token, expr, Token, expr>;         // expr op expr op expr
using Malformed = ExprProduction<5, bool, std::string>;                    // is_critical message
using Variable = ExprProduction<6, Token>;                                 // name
using Empty = ExprProduction<7>;                                           // No data (for empty variable initializer)
using Assign = ExprProduction<8, Token, expr>;                             // name value
using Logical = ExprProduction<9, expr, Token, expr>;                      // left op right	(where op is "and" or "or")
using Call = ExprProduction<10, expr, Token, std::vector<expr>>;           // callee paren arguments
using Lambda = ExprProduction<11, std::vector<Token>, std::vector<stmt>>;  // params body
using Get = ExprProduction<12, expr, Token>;                               // object name
using Set = ExprProduction<13, expr, Token, expr>;                         // object name value
using This = ExprProduction<14, Token>;                                    // 'this'
// clang-format on

#define EXPR_TYPES Literal, Grouping, Unary, Binary, Ternary, Malformed, Variable, \
                   Empty, Assign, Logical, Call, Lambda, Get, Set, This

using ExprVisitableBase = Visitable<EXPR_TYPES>;
template <int id, typename... Types>
using ProductionVisitableImpl = VisitableImpl<ExprProduction<id, Types...>, EXPR_TYPES>;
using ExprVisitor = Visitor<EXPR_TYPES>;

//--------------------End of alias definitions--------------------------------

/// A generic production for an expression. id is for disambiguation
template <int id, typename... Types>
struct ExprProduction : public Expr, public ProductionVisitableImpl<id, Types...>
{
public:
  explicit ExprProduction(Types &&...args)
      : derivatives(std::forward<Types>(args)...) {}

  void print(std::ostream &os) const override
  {
    os << "Expr: \n\t";
    std::apply([&os](auto &&...args)
               { ((os << "\t" << args << "\t"), ...); },
               derivatives);
    os << "\n";
  }

  template <size_t index>
  decltype(auto) child()
  {
    return std::get<index>(derivatives);
  }

  template <size_t index>
  decltype(auto) child() const
  {
    const auto &elem = std::get<index>(derivatives);
    return elem;
  }

  std::tuple<Types...> derivatives;
};

std::ostream &operator<<(std::ostream &os, const std::vector<expr> &rhs);

std::ostream &operator<<(std::ostream &os, const Expr &rhs);
std::ostream &operator<<(std::ostream &os, const expr &rhs);

#define DECLARE_EXPR_VISIT_METHODS  \
  void visit(Assign &) override;    \
  void visit(Logical &) override;   \
  void visit(Variable &) override;  \
  void visit(Empty &) override;     \
  void visit(Literal &) override;   \
  void visit(Unary &) override;     \
  void visit(Binary &) override;    \
  void visit(Ternary &) override;   \
  void visit(Malformed &) override; \
  void visit(Call &) override;      \
  void visit(Grouping &) override;  \
  void visit(Lambda &) override;    \
  void visit(Get &) override;       \
  void visit(Set &) override;       \
  void visit(This &) override;

template <typename Type, typename... arg_types>
expr new_expr(arg_types &&...args)
{
  return std::make_unique<Type>(std::forward<arg_types>(args)...);
}
