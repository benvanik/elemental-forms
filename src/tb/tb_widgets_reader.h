/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_WIDGETS_READER_H
#define TB_WIDGETS_READER_H

#include "tb_widgets.h"

#include "tb/util/link_list.h"

namespace tb {

class Node;
class Element;
class ElementFactory;
class ElementReader;

// Contains info passed to Element::OnInflate during resource loading.
struct InflateInfo {
  InflateInfo(ElementReader* reader, Element* target, Node* node,
              Value::Type sync_type)
      : reader(reader), target(target), node(node), sync_type(sync_type) {}

  ElementReader* reader;
  // The element that that will be parent to the inflated element.
  Element* target;
  // The node containing properties.
  Node* node;
  // The data type that should be synchronized through ElementValue.
  Value::Type sync_type;
};

// Creates a element from a Node.
class ElementFactory : public util::TBLinkOf<ElementFactory> {
 public:
  ElementFactory(const char* name, Value::Type sync_type);

  // Creates and returns the new element.
  virtual Element* Create(InflateInfo* info) = 0;

  void Register();

 public:
  const char* name;
  Value::Type sync_type;
  ElementFactory* next_registered_wf = nullptr;
};

// Creates a new ElementFactory for the given class name so it can be created
// from resources (using ElementReader).
//
// classname   - The name of the class.
// sync_type   - The data type that should be synchronized through ElementValue.
// add_child_z - The order in which children should be added to it by default.
//
// It should be followed by an empty block (may eventually be removed).
// Reading custom properties from resources can be done by overriding
// Element::OnInflate.
//
// Example:
//   TB_WIDGET_FACTORY(MyElement, Value::Type::kInt, ElementZ::kTop) {}
#define TB_WIDGET_FACTORY(classname, sync_type, add_child_z)              \
  class classname##ElementFactory : public ElementFactory {               \
   public:                                                                \
    classname##ElementFactory() : ElementFactory(#classname, sync_type) { \
      Register();                                                         \
    }                                                                     \
    Element* Create(InflateInfo* info) override {                         \
      classname* element = new classname();                               \
      element->GetContentRoot()->SetZInflate(add_child_z);                \
      ReadCustomProps(element, info);                                     \
      return element;                                                     \
    }                                                                     \
    void ReadCustomProps(classname* element, InflateInfo* info);          \
  };                                                                      \
  static classname##ElementFactory classname##_wf;                        \
  void classname##ElementFactory::ReadCustomProps(classname* element,     \
                                                  InflateInfo* info)

// Parses a resource file (or buffer) into a Node tree and turn it into a
// hierarchy of elements.
// It can create all types of elements that have a registered factory
// (ElementFactory). All core elements have a factory by default, and you can
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
class ElementReader {
 public:
  static ElementReader* Create();
  ~ElementReader();

  // Adds a element factory. Does not take ownership of the factory.
  // The easiest way to add factories for custom element types, is using the
  // TB_WIDGET_FACTORY macro that automatically register it during startup.
  bool AddFactory(ElementFactory* wf) {
    factories.AddLast(wf);
    return true;
  }
  void RemoveFactory(ElementFactory* wf) { factories.Remove(wf); }

  // Sets the id from the given node.
  static void SetIDFromNode(TBID& id, Node* node);

  bool LoadFile(Element* target, const char* filename);
  bool LoadData(Element* target, const char* data);
  bool LoadData(Element* target, const char* data, size_t data_len);
  void LoadNodeTree(Element* target, Node* node);

 private:
  bool Init();
  bool CreateElement(Element* target, Node* node);

  util::TBLinkListOf<ElementFactory> factories;
};

}  // namespace tb

#endif  // TB_WIDGETS_READER_H
