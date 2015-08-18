/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_DESIGN_DESIGNER_FORM_H_
#define EL_DESIGN_DESIGNER_FORM_H_

#include <string>

#include "el/element.h"
#include "el/element_listener.h"
#include "el/elements/form.h"
#include "el/elements/list_box.h"
#include "el/elements/text_box.h"
#include "el/event_handler.h"
#include "el/message_handler.h"

namespace el {
namespace design {

class DesignerForm : public elements::Form,
                     public MessageHandler,
                     public ElementListener {
 public:
  TBOBJECT_SUBCLASS(DesignerForm, Form);

  DesignerForm();
  ~DesignerForm() override;

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

  EventHandler event_handler_;
  elements::ListBox* element_list_box_ = nullptr;
  ListItemSourceList<ElementItem> element_list_source_;
  elements::TextBox* source_text_box_ = nullptr;
  Element* build_container_ = nullptr;
  Element* build_content_root_ = nullptr;

  WeakElementPointer bind_element_;
};

}  // namespace design
}  // namespace el

#endif  // EL_DESIGN_DESIGNER_FORM_H_
