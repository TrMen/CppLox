// From:
// https://stackoverflow.com/questions/11796121/implementing-the-visitor-pattern-using-c-templates
// The idea is that Visitor recursively unpacks the arguments to add an overload
// for visit for each template parameter type. Then Visitable names those same
// types, plus a derived type. The derived type is the class that inherits from
// VisitableImpl
//
// Usage:
// Subclasses of Visitable:
//	class Mesh : public Object, public VisitableImpl<Mesh, Mesh, Text> {};
//	class Text : public Object, public VisitableImpl<Text, Mesh, Text> {};
// Visitor subclass:
//	class Renderer : public Visitor<Mesh, Text>{};
//
// Then later on, use through a Visitor<Mesh, Text> pointer or reference.
//	some_mesh_ptr->accept(some_renderer);
// To optain this, you probably need to static_cast to it, but that's fine
// because this is all compile-time decided
#pragma once
template <typename... Types>
struct Visitor;

template <typename T>
struct Visitor<T>
{
  virtual ~Visitor() = default;
  virtual void visit(T &visitable) = 0;
};

template <typename T, typename... Types>
struct Visitor<T, Types...> : public Visitor<Types...>
{
  using Visitor<Types...>::visit;
  virtual void visit(T &visitable) = 0;
};

template <typename... Types>
struct Visitable
{
  virtual ~Visitable() = default;
  virtual void accept(Visitor<Types...> &visitor) = 0;
};

template <typename Derived, typename... Types>
struct VisitableImpl : public Visitable<Types...>
{
  virtual void accept(Visitor<Types...> &visitor)
  {
    visitor.visit(static_cast<Derived &>(*this));
  }
};
