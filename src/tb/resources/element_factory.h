/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_RESOURCES_ELEMENT_FACTORY_H_
#define TB_RESOURCES_ELEMENT_FACTORY_H_

#include <memory>
#include <vector>

#include "tb_node_tree.h"
#include "tb_value.h"

namespace tb {
class Element;
}  // namespace tb

namespace tb {
namespace resources {

class ElementInflater;
class ElementFactory;

// Contains info passed to Element::OnInflate during resource loading.
struct InflateInfo {
  InflateInfo(ElementFactory* reader, Element* target, Node* node,
              Value::Type sync_type)
      : reader(reader), target(target), node(node), sync_type(sync_type) {}

  ElementFactory* reader;
  // The element that that will be parent to the inflated element.
  Element* target;
  // The node containing properties.
  Node* node;
  // The data type that should be synchronized through ElementValue.
  Value::Type sync_type;
};

// Creates a element from a Node.
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
#define TB_REGISTER_ELEMENT_INFLATER(classname, sync_type, add_child_z)      \
  class classname##ElementInflater : public tb::resources::ElementInflater { \
   public:                                                                   \
    classname##ElementInflater()                                             \
        : tb::resources::ElementInflater(#classname, sync_type) {}           \
    tb::Element* Create(tb::resources::InflateInfo* info) override {         \
      auto element = new classname();                                        \
      element->GetContentRoot()->SetZInflate(add_child_z);                   \
      return element;                                                        \
    }                                                                        \
  };                                                                         \
  tb::resources::ElementFactory::get()->RegisterInflater(                    \
      std::make_unique<classname##ElementInflater>());

// Parses a resource file (or buffer) into a Node tree and turn it into a
// hierarchy of elements.
// It can create all types of elements that have a registered factory
// (ElementInflater). All core elements have a factory by default, and you can
// also add your own.
//
// Values may be looked up from any existing NodeRefTree using the syntax
// "@treename>noderequest". If treename is left out, the request will be looked
// up in the same node tree. In addition to this, strings will be looked up from
// the global Language by using the syntax "@stringid"
//
// Branches may be included or not depending on the value of a NodeRefTree
// node, using "@if @treename>noderequest" and optionally a following "@else".
//
// Branches may be included from NodeRefTree using "@include
// @treename>noderequest", or included from nodes specified previosly in the
// same tree using "@include noderequest".
//
// Files can be included by using the syntax "@file filename".
//
// Each factory may have its own set of properties, but a set of generic
// properties is always supported on all elements. Those are:
//
// Resource name:   Element property:    Values:
//
// id					      m_id              TBID (string or
// int)
// group-id			    m_group_id		    TBID (string or int)
// value				    SetValue          integer
// data				      m_data            integer
// is-group-root		SetIsGroupRoot	  boolean
// is-focusable		  SetIsFocusable	  boolean
// want-long-click	SetWantLongClick  boolean
// ignore-input		  SetIgnoreInput	  boolean
// opacity				  SetOpacity        float (0 - 1)
// text				      SetText           string
// connection			  Connect           string
// axis				      SetAxis           x or y
// gravity				  SetGravity        string
//     combination of left, top, right, bottom, or all
// visibility			  SetVisibility     string (visible, invisible,
//                                    gone)
// state				    SetState          string (disabled)
// skin				      SetSkinBg         TBID (string or int)
// rect				      set_rect          4 integers (x, y, width,
//                                    height)
// lp>width			    SetLayoutParams   dimension
// lp>min-width		  SetLayoutParams   dimension
// lp>max-width		  SetLayoutParams   dimension
// lp>pref-width		SetLayoutParams   dimension
// lp>height			  SetLayoutParams   dimension
// lp>min-height		SetLayoutParams   dimension
// lp>max-height		SetLayoutParams   dimension
// lp>pref-height		SetLayoutParams   dimension
// autofocus			  The Element will be focused automatically the
//                  first time its Window is activated.
// font>name			  Font name
// font>size			  Font size
class ElementFactory {
 public:
  static ElementFactory* get() { return element_reader_singleton_.get(); }
  static void set(std::unique_ptr<ElementFactory> value) {
    element_reader_singleton_ = std::move(value);
  }

  void RegisterInflater(std::unique_ptr<ElementInflater> inflater);

  bool LoadFile(Element* target, const char* filename);
  bool LoadData(Element* target, const char* data,
                size_t data_length = std::string::npos);
  void LoadNodeTree(Element* target, Node* node);

 private:
  bool CreateElement(Element* target, Node* node);

  static std::unique_ptr<ElementFactory> element_reader_singleton_;
  std::vector<std::unique_ptr<ElementInflater>> inflaters_;
};

}  // namespace resources
}  // namespace tb

#endif  // TB_RESOURCES_ELEMENT_FACTORY_H_
