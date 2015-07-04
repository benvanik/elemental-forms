/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include <cassert>

#include "tb_message_window.h"
#include "tb_text_box.h"
#include "tb_widgets_reader.h"

#include "tb/util/string_table.h"

namespace tb {

MessageWindow::MessageWindow(Element* target, TBID id) : m_target(target) {
  ElementListener::AddGlobalListener(this);
  SetID(id);
}

MessageWindow::~MessageWindow() {
  ElementListener::RemoveGlobalListener(this);
  if (Element* dimmer = m_dimmer.Get()) {
    dimmer->GetParent()->RemoveChild(dimmer);
    delete dimmer;
  }
}

bool MessageWindow::Show(const std::string& title, const std::string& message,
                         MessageWindowSettings* settings) {
  Element* target = m_target.Get();
  if (!target) return false;

  MessageWindowSettings default_settings;
  if (!settings) {
    settings = &default_settings;
  }

  Element* root = target->GetParentRoot();

  const char* source =
      "Layout: axis: y, distribution: available\n"
      "	Layout: distribution: available, size: available\n"
      "		SkinImage: id: 2\n"
      "		TextBox: multiline: 1, readonly: 1, id: 1\n"
      "	Layout: distribution-position: right bottom, id: 3\n";
  if (!ElementReader::get()->LoadData(GetContentRoot(), source)) {
    return false;
  }

  SetText(title);

  GetElementByIDAndType<SkinImage>(2)->SetSkinBg(settings->icon_skin);

  TextBox* text_box = GetElementByIDAndType<TextBox>(1);
  text_box->SetStyling(settings->styling);
  text_box->SetText(message);
  text_box->SetSkinBg("");

  // Create buttons.
  if (settings->msg == MessageWindowButtons::kOk) {
    AddButton("MessageWindow.ok", true);
  } else if (settings->msg == MessageWindowButtons::kOkCancel) {
    AddButton("MessageWindow.ok", true);
    AddButton("MessageWindow.cancel", false);
  } else if (settings->msg == MessageWindowButtons::kYesNo) {
    AddButton("MessageWindow.yes", true);
    AddButton("MessageWindow.no", false);
  }

  // Size to fit content. This will use the default size of the textfield.
  ResizeToFitContent();
  Rect rect = this->rect();

  // Get how much we overflow the textfield has given the current width, and
  // grow our height to show all we can.
  // FIX: It would be better to use adapt-to-content on the text_box to achieve
  // the most optimal size.
  // At least when we do full blown multi pass size checking.
  rect.h += text_box->GetStyleEdit()->GetOverflowY();

  // Create background dimmer.
  if (settings->dimmer) {
    Dimmer* dimmer = new Dimmer();
    root->AddChild(dimmer);
    m_dimmer.Set(dimmer);
  }

  // Center and size to the new height.
  Rect bounds(0, 0, root->rect().w, root->rect().h);
  set_rect(rect.CenterIn(bounds).MoveIn(bounds).Clip(bounds));
  root->AddChild(this);
  return true;
}

void MessageWindow::AddButton(TBID id, bool focused) {
  Layout* layout = GetElementByIDAndType<Layout>(3);
  if (!layout) return;
  Button* btn = new Button();
  btn->SetID(id);
  btn->SetText(util::GetString(btn->GetID()));
  layout->AddChild(btn);
  if (focused) {
    btn->SetFocus(FocusReason::kUnknown);
  }
}

bool MessageWindow::OnEvent(const ElementEvent& ev) {
  if (ev.type == EventType::kClick && ev.target->IsOfType<Button>()) {
    WeakElementPointer this_element(this);

    // Invoke the click on the target.
    ElementEvent target_ev(EventType::kClick);
    target_ev.ref_id = ev.target->GetID();
    InvokeEvent(target_ev);

    // If target got deleted, close.
    if (this_element.Get()) {
      Close();
    }
    return true;
  } else if (ev.type == EventType::kKeyDown &&
             ev.special_key == SpecialKey::kEsc) {
    ElementEvent click_ev(EventType::kClick);
    m_close_button.InvokeEvent(click_ev);
    return true;
  }
  return Window::OnEvent(ev);
}

void MessageWindow::OnDie() {
  if (Element* dimmer = m_dimmer.Get()) {
    dimmer->Die();
  }
}

void MessageWindow::OnElementDelete(Element* element) {
  // If the target element is deleted, close!
  if (!m_target.Get()) {
    Close();
  }
}

bool MessageWindow::OnElementDying(Element* element) {
  // If the target element or an ancestor of it is dying, close!
  if (element == m_target.Get() || element->IsAncestorOf(m_target.Get())) {
    Close();
  }
  return false;
}

}  // namespace tb
