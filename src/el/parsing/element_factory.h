/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_PARSING_ELEMENT_FACTORY_H_
#define EL_PARSING_ELEMENT_FACTORY_H_

#include <memory>
#include <vector>

#include "el/parsing/parse_node.h"
#include "el/value.h"

namespace el {
class Element;
}  // namespace el

namespace el {
namespace parsing {

class ElementInflater;

// Parses a resource file (or buffer) into a ParseNode tree and turn it into a
// hierarchy of elements.
// It can create all types of elements that have a registered factory
// (ElementInflater). All core elements have a factory by default, and you can
// also add your own.
//
// Values may be looked up from any existing ParseNodeTree using the syntax
// "@treename>noderequest". If treename is left out, the request will be looked
// up in the same node tree. In addition to this, strings will be looked up from
// the global Language by using the syntax "@stringid"
//
// Branches may be included or not depending on the value of a ParseNodeTree
// node, using "@if @treename>noderequest" and optionally a following "@else".
//
// Branches may be included from ParseNodeTree using "@include
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
// lp>width			    set_layout_params   dimension
// lp>min-width		  set_layout_params   dimension
// lp>max-width		  set_layout_params   dimension
// lp>pref-width		set_layout_params   dimension
// lp>height			  set_layout_params   dimension
// lp>min-height		set_layout_params   dimension
// lp>max-height		set_layout_params   dimension
// lp>pref-height		set_layout_params   dimension
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

  ElementFactory();
  ~ElementFactory();

  void RegisterInflater(std::unique_ptr<ElementInflater> inflater);

  bool LoadFile(Element* target, const char* filename);
  bool LoadData(Element* target, const char* data,
                size_t data_length = std::string::npos);
  void LoadNodeTree(Element* target, ParseNode* node);

 private:
  bool CreateElement(Element* target, ParseNode* node);

  static std::unique_ptr<ElementFactory> element_reader_singleton_;
  std::vector<std::unique_ptr<ElementInflater>> inflaters_;
};

}  // namespace parsing
}  // namespace el

#endif  // EL_PARSING_ELEMENT_FACTORY_H_
