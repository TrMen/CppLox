#include "class.hpp"

#include "error.hpp"
#include "instance.hpp"
#include "logging.hpp"

Class::Class(std::string _name, ClassPtr _superclass, ClassFunctions _functions)
    : superclass(std::move(_superclass)), methods(std::get<0>(_functions)),
      unbounds(std::move(std::get<1>(_functions))),
      getters(std::move(std::get<2>(_functions))), m_name(std::move(_name)) {}

const std::string &Class::name() const { return m_name; }

std::string Class::to_string() const {
  std::string representation = "class " + name() + "\nMethods:";
  for (const auto &method : methods) {
    representation += "\n\t" + method.first;
  }
  representation += "\nUnbound functions:";
  for (const auto &unbound : unbounds) {
    representation += "\n\t" + unbound.first;
  }

  return representation + '\n';
}

size_t Class::arity() const {
  if (const auto &constructor = get_method("init")) {
    return constructor->arity();
  }
  return 0;
}

Token::Value Class::call(Interpreter &interpreter,
                         const std::vector<Token::Value> &arguments) {
  LOG_DEBUG("Creating instance");

  auto instance = std::make_shared<Instance>(shared_from_this());

  LOG_DEBUG("Created instance successfully");

  // Run constructor method when class is called. Class-call args become
  // constructor args
  if (const auto &constructor = get_method("init")) {
    constructor->bind(instance)->call(interpreter, arguments);
  }

  return instance;
}

const FunctionPtr &Class::get_method(const std::string &name) const {
  if (methods.contains(name)) {
    return methods.at(name);
  }

  if (superclass != nullptr) {
    return superclass->get_method(name);
  }

  return nullRef;
}

const FunctionPtr &Class::get_unbound(const std::string &name) const {
  if (unbounds.contains(name)) {
    return unbounds.at(name);
  }

  if (superclass != nullptr) {
    return superclass->get_unbound(name);
  }

  return nullRef;
}

const FunctionPtr &Class::get_getter(const std::string &name) const {
  if (getters.contains(name)) {
    return getters.at(name);
  }

  if (superclass != nullptr) {
    return superclass->get_getter(name);
  }

  return nullRef;
}