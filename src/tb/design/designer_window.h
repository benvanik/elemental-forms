/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_DESIGN_DESIGNER_WINDOW_H_
#define TB_DESIGN_DESIGNER_WINDOW_H_

#include "tb/element.h"
#include "tb/element_listener.h"
#include "tb/elements/list_box.h"
#include "tb/elements/text_box.h"
#include "tb/event_handler.h"
#include "tb/message_handler.h"
#include "tb/window.h"

namespace tb {
namespace design {

class DesignerWindow : public Window,
                       public MessageHandler,
                       public ElementListener {
 public:
  TBOBJECT_SUBCLASS(DesignerWindow, Window);

  DesignerWindow();
  ~DesignerWindow() override;

  void Show(Element* root_element);

  void BindContent(Element* bind_element);
  void CloseContent();

 private:
  class ElementItem : public GenericStringItem {
   public:
    ElementItem(Element* element, const std::string& str)
        : GenericStringItem(str), element_(element) {}
    Element* element() const { return element_; }

   private:
    Element* element_;
  };

  void RefreshContent();
  void PopulateElementListBox();

  EventHandler event_handler_ = {this};
  elements::ListBox* element_list_box_ = nullptr;
  ListItemSourceList<ElementItem> element_list_source_;
  elements::TextBox* source_text_box_ = nullptr;
  Element* build_container_ = nullptr;
  Element* build_content_root_ = nullptr;

  WeakElementPointer bind_element_;
};

}  // namespace design
}  // namespace tb

#endif  // TB_DESIGN_DESIGNER_WINDOW_H_
