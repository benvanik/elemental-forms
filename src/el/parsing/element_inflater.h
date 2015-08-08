/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_PARSING_ELEMENT_INFLATER_H_
#define EL_PARSING_ELEMENT_INFLATER_H_

#include <memory>
#include <vector>

#include "el/parsing/element_factory.h"
#include "el/parsing/parse_node.h"
#include "el/value.h"

namespace el {
class Element;
}  // namespace el

namespace el {
namespace parsing {

class ElementInflater;
class ElementFactory;

// Contains info passed to Element::OnInflate during resource loading.
struct InflateInfo {
  InflateInfo(ElementFactory* reader, Element* target, ParseNode* node,
              Value::Type sync_type)
      : reader(reader), target(target), node(node), sync_type(sync_type) {}

  ElementFactory* reader;
  // The element that that will be parent to the inflated element.
  Element* target;
  // The node containing properties.
  ParseNode* node;
  // The data type that should be synchronized through ElementValue.
  Value::Type sync_type;
};

// Creates a element from a ParseNode.
class ElementInflater {
 public:
  const char* name() const { return name_; }
  Value::Type sync_type() const { return sync_type_; }

  // Creates and returns the new element.
  virtual Element* Create(InflateInfo* info) = 0;

 protected:
  ElementInflater(const char* name, Value::Type sync_type)
      : name_(name), sync_type_(sync_type) {}

 private:
  friend class ElementFactory;

  const char* name_;
  Value::Type sync_type_;
};

// Creates a new ElementInflater for the given class name so it can be created
// from resources (using ElementFactory).
//
// classname   - The name of the class.
// sync_type   - The data type that should be synchronized through ElementValue.
// add_child_z - The order in which children should be added to it by default.
//
// Reading custom properties from resources can be done by overriding
// Element::OnInflate.
//
// Example:
// void MyElement::RegisterInflater() {
//   EL_REGISTER_ELEMENT_INFLATER(MyElement, Value::Type::kInt, ElementZ::kTop);
// }
//
// On startup the inflator must be registered before elements of that type can
// be inflated.
#define EL_REGISTER_ELEMENT_INFLATER(classname, sync_type, add_child_z)    \
  class classname##ElementInflater : public el::parsing::ElementInflater { \
   public:                                                                 \
    classname##ElementInflater()                                           \
        : el::parsing::ElementInflater(#classname, sync_type) {}           \
    el::Element* Create(el::parsing::InflateInfo* info) override {         \
      auto element = new classname();                                      \
      element->content_root()->set_z_inflate(add_child_z);                 \
      return element;                                                      \
    }                                                                      \
  };                                                                       \
  el::parsing::ElementFactory::get()->RegisterInflater(                    \
      std::make_unique<classname##ElementInflater>());

}  // namespace parsing
}  // namespace el

#endif  // EL_PARSING_ELEMENT_INFLATER_H_
