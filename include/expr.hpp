#pragma once

#include "token.hpp"
#include "visitor.hpp"
#include <memory>
#include <vector>

struct Expr
{
  virtual ~Expr();
  virtual void print(std::ostream &os) const = 0;
};
using expr = std::unique_ptr<Expr>;

template <int id, typename... Types>
struct Production;

// ---------------------Alias definitions for convenience---------------------

using Binary = Production<0, expr, Token, expr>; //	expr bin_op expr
using Grouping = Production<1, expr>;            // (expr)
using Literal = Production<2, Token::Value>;     //	value
using Unary = Production<3, Token, expr>;        //	unary_op expr
using Ternary =
    Production<4, expr, Token, expr, Token, expr>;  //	expr op expr op expr
using Malformed = Production<5, bool, std::string>; //	is_critical message
using Variable = Production<6, Token>;              //	name
using Empty = Production<7>;                        //	No data (for empty variable initializer)
using Assign = Production<8, Token, expr>;          //	name value
using Logical =
    Production<9, expr, Token,
               expr>; //	left op right	(where op is "and" or "or")
using Call =
    Production<20, expr, Token, std::vector<expr>>; //	callee paren arguments

using VisitableBase =
    Visitable<Literal, Grouping, Unary, Binary, Ternary, Malformed, Variable,
              Empty, Assign, Logical, Call>;
template <int id, typename... Types>
using VisitableImpl_t =
    VisitableImpl<Production<id, Types...>, Literal, Grouping, Unary, Binary,
                  Ternary, Malformed, Variable, Empty, Assign, Logical, Call>;
using Visitor_t = Visitor<Literal, Grouping, Unary, Binary, Ternary, Malformed,
                          Variable, Empty, Assign, Logical, Call>;

//--------------------End of alias definitions--------------------------------

/// A generic production for an expression. id is for disambiguation
template <int id, typename... Types>
struct Production : public Expr, public VisitableImpl_t<id, Types...>
{
public:
  explicit Production(Types &&...args)
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
    const auto elem = std::get<index>(derivatives);
    return elem;
  }

  std::tuple<Types...> derivatives;
};

std::ostream &operator<<(std::ostream &os, const std::vector<expr> &rhs);

std::ostream &operator<<(std::ostream &os, const Expr &rhs);
std::ostream &operator<<(std::ostream &os, const expr &rhs);

template <typename Type, typename... arg_types>
expr new_expr(arg_types &&...args)
{
  return std::make_unique<Type>(std::forward<arg_types>(args)...);
}