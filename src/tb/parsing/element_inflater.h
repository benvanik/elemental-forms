/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_PARSING_ELEMENT_INFLATER_H_
#define TB_PARSING_ELEMENT_INFLATER_H_

#include <memory>
#include <vector>

#include "tb/parsing/element_factory.h"
#include "tb/parsing/parse_node.h"
#include "tb/value.h"

namespace tb {
class Element;
}  // namespace tb

namespace tb {
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
//   TB_REGISTER_ELEMENT_INFLATER(MyElement, Value::Type::kInt, ElementZ::kTop);
// }
//
// On startup the inflator must be registered before elements of that type can
// be inflated.
#define TB_REGISTER_ELEMENT_INFLATER(classname, sync_type, add_child_z)    \
  class classname##ElementInflater : public tb::parsing::ElementInflater { \
   public:                                                                 \
    classname##ElementInflater()                                           \
        : tb::parsing::ElementInflater(#classname, sync_type) {}           \
    tb::Element* Create(tb::parsing::InflateInfo* info) override {         \
      auto element = new classname();                                      \
      element->content_root()->set_z_inflate(add_child_z);                 \
      return element;                                                      \
    }                                                                      \
  };                                                                       \
  tb::parsing::ElementFactory::get()->RegisterInflater(                    \
      std::make_unique<classname##ElementInflater>());

}  // namespace parsing
}  // namespace tb

#endif  // TB_PARSING_ELEMENT_INFLATER_H_
