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

#include "tb_linklist.h"
#include "tb_widgets.h"

namespace tb {

class Node;
class Widget;
class WidgetFactory;
class WidgetReader;

// Contains info passed to Widget::OnInflate during resource loading.
struct InflateInfo {
  InflateInfo(WidgetReader* reader, Widget* target, Node* node,
              Value::Type sync_type)
      : reader(reader), target(target), node(node), sync_type(sync_type) {}

  WidgetReader* reader;
  // The widget that that will be parent to the inflated widget.
  Widget* target;
  // The node containing properties.
  Node* node;
  // The data type that should be synchronized through WidgetValue.
  Value::Type sync_type;
};

// Creates a widget from a Node.
class WidgetFactory : public TBLinkOf<WidgetFactory> {
 public:
  WidgetFactory(const char* name, Value::Type sync_type);

  // Creates and returns the new widget.
  virtual Widget* Create(InflateInfo* info) = 0;

  void Register();

 public:
  const char* name;
  Value::Type sync_type;
  WidgetFactory* next_registered_wf = nullptr;
};

// Creates a new WidgetFactory for the given class name so it can be created
// from resources (using WidgetReader).
//
// classname   - The name of the class.
// sync_type   - The data type that should be synchronized through WidgetValue.
// add_child_z - The order in which children should be added to it by default.
//
// It should be followed by an empty block (may eventually be removed).
// Reading custom properties from resources can be done by overriding
// Widget::OnInflate.
//
// Example:
//   TB_WIDGET_FACTORY(MyWidget, Value::Type::kInt, WidgetZ::kTop) {}
#define TB_WIDGET_FACTORY(classname, sync_type, add_child_z)            \
  class classname##WidgetFactory : public WidgetFactory {               \
   public:                                                              \
    classname##WidgetFactory() : WidgetFactory(#classname, sync_type) { \
      Register();                                                       \
    }                                                                   \
    Widget* Create(InflateInfo* info) override {                        \
      classname* widget = new classname();                              \
      widget->GetContentRoot()->SetZInflate(add_child_z);               \
      ReadCustomProps(widget, info);                                    \
      return widget;                                                    \
    }                                                                   \
    void ReadCustomProps(classname* widget, InflateInfo* info);         \
  };                                                                    \
  static classname##WidgetFactory classname##_wf;                       \
  void classname##WidgetFactory::ReadCustomProps(classname* widget,     \
                                                 InflateInfo* info)

// Parses a resource file (or buffer) into a Node tree and turn it into a
// hierarchy of widgets.
// It can create all types of widgets that have a registered factory
// (WidgetFactory). All core widgets have a factory by default, and you can also
// add your own.
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
// properties is always supported on all widgets. Those are:
//
// Resource name:   Widget property:    Values:
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
// rect				      SetRect           4 integers (x, y, width,
//                                    height)
// lp>width			    SetLayoutParams   dimension
// lp>min-width		  SetLayoutParams   dimension
// lp>max-width		  SetLayoutParams   dimension
// lp>pref-width		SetLayoutParams   dimension
// lp>height			  SetLayoutParams   dimension
// lp>min-height		SetLayoutParams   dimension
// lp>max-height		SetLayoutParams   dimension
// lp>pref-height		SetLayoutParams   dimension
// autofocus			  The Widget will be focused automatically the
//                  first time its Window is activated.
// font>name			  Font name
// font>size			  Font size
class WidgetReader {
 public:
  static WidgetReader* Create();
  ~WidgetReader();

  // Adds a widget factory. Does not take ownership of the factory.
  // The easiest way to add factories for custom widget types, is using the
  // TB_WIDGET_FACTORY macro that automatically register it during startup.
  bool AddFactory(WidgetFactory* wf) {
    factories.AddLast(wf);
    return true;
  }
  void RemoveFactory(WidgetFactory* wf) { factories.Remove(wf); }

  // Sets the id from the given node.
  static void SetIDFromNode(TBID& id, Node* node);

  bool LoadFile(Widget* target, const char* filename);
  bool LoadData(Widget* target, const char* data);
  bool LoadData(Widget* target, const char* data, size_t data_len);
  void LoadNodeTree(Widget* target, Node* node);

 private:
  bool Init();
  bool CreateWidget(Widget* target, Node* node);

  TBLinkListOf<WidgetFactory> factories;
};

}  // namespace tb

#endif  // TB_WIDGETS_READER_H
