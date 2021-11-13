#include "expr.hpp"

Expr::~Expr() = default;

std::ostream &operator<<(std::ostream &os, const std::vector<expr> &rhs) {
  for (const auto &expr : rhs) {
    os << expr;
  }

  return os;
}

std::ostream &operator<<(std::ostream &os, const expr &rhs) {
  rhs->print(os);
  return os;
}

std::ostream &operator<<(std::ostream &os, const Expr &rhs) {
  rhs.print(os);
  return os;
}
