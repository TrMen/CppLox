#include "expr.hpp"

Expr::~Expr() = default;

std::ostream &operator<<(std::ostream &os, const std::vector<expr> &rhs)
{
  for (const auto &expr : rhs)
  {
    os << expr;
  }

  return os;
}

std::ostream &operator<<(std::ostream &os, const expr &rhs)
{
  rhs->print(os);
  return os;
}

std::ostream &operator<<(std::ostream &os, const Expr &rhs)
{
  rhs.print(os);
  return os;
}

//-------------Start of Visitor implementations------------------
/*
static void cast_and_accept(const expr &visited, Visitor_t &visitor) {
  dynamic_cast<VisitableBase &>(*visited).accept(visitor);
}

std::string ASTPrinter::get_representation(const expr &expr) {
  cast_and_accept(expr, *this);
  return str_tree;
}

static std::string simple_literal_visit(const Literal &visitable) {
  const auto data = visitable.child<0>();
  const auto index = data.index();
  switch (index) {
  case 0: // double
    return std::to_string(std::get<0>(data));
  case 1: // std::string
    return std::get<1>(data);
  case 2: // NullType
    return "null";
  case 3: // bool;
    return std::get<3>(data) ? "true" : "false";
  }
  return "";
}

void ASTPrinter::visit(Literal &visitable) {
  str_tree += simple_literal_visit(visitable);
}
void ASTPrinter::visit(Unary &visitable) {
  parenthesize(visitable.child<0>().lexeme, visitable.child<1>());
}
void ASTPrinter::visit(Grouping &visitable) {
  parenthesize("group", visitable.child<0>());
}
void ASTPrinter::visit(Binary &visitable) {
  parenthesize(visitable.child<1>().lexeme, visitable.child<0>(),
               visitable.child<2>());
}
void ASTPrinter::visit(Ternary &visitable) {
  std::string prefix =
      visitable.child<1>().lexeme + visitable.child<3>().lexeme;
  parenthesize(prefix, visitable.child<0>(), visitable.child<2>(),
               visitable.child<4>());
}

void ASTPrinter::visit(Malformed &visitable) {
  std::string text = "MALFORMED NODE ";
  if (visitable.child<0>()) {
    text += "CRITICAL ";
  }
  if (!visitable.child<1>().empty()) {
    text += "MESSAGE: " + visitable.child<1>();
  }
  parenthesize(text);
}

template <typename T, typename... Types>
void ASTPrinter::write(const T &expr, const Types &... args) {
  str_tree += " ";

  cast_and_accept(expr, *this);

  if constexpr (sizeof...(args) > 0) {
    write(args...);
  }
}

template <typename... Types>
void ASTPrinter::parenthesize(const std::string &name, const Types &... args) {
  str_tree += "(" + name;
  if constexpr (sizeof...(args) > 0) {
    write(args...);
  }
  str_tree += ")";
}
*/
