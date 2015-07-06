/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include <cassert>

#include "tb/elements/dimmer.h"
#include "tb/elements/icon_box.h"
#include "tb/elements/message_window.h"
#include "tb/elements/text_box.h"
#include "tb/util/string_table.h"

namespace tb {
namespace elements {

MessageWindow::MessageWindow(Element* target, TBID id) : m_target(target) {
  ElementListener::AddGlobalListener(this);
  set_id(id);
}

MessageWindow::~MessageWindow() {
  ElementListener::RemoveGlobalListener(this);
  if (Element* dimmer = m_dimmer.get()) {
    dimmer->parent()->RemoveChild(dimmer);
    delete dimmer;
  }
}

bool MessageWindow::Show(const std::string& title, const std::string& message,
                         MessageWindowSettings* settings) {
  Element* target = m_target.get();
  if (!target) return false;

  MessageWindowSettings default_settings;
  if (!settings) {
    settings = &default_settings;
  }

  Element* root = target->parent_root();

  const char* source =
      "LayoutBox: axis: y, distribution: available\n"
      "	LayoutBox: distribution: available, size: available\n"
      "		IconBox: id: 2\n"
      "		TextBox: multiline: 1, readonly: 1, id: 1\n"
      "	LayoutBox: distribution-position: right bottom, id: 3\n";
  if (!content_root()->LoadData(source)) {
    return false;
  }

  set_text(title);

  GetElementById<IconBox>(2)->set_background_skin(settings->icon_skin);

  TextBox* text_box = GetElementById<TextBox>(1);
  text_box->set_styled(settings->styling);
  text_box->set_text(message);
  text_box->set_background_skin("");

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
  rect.h += text_box->text_view()->GetOverflowY();

  // Create background dimmer.
  if (settings->dimmer) {
    auto dimmer = new Dimmer();
    root->AddChild(dimmer);
    m_dimmer.reset(dimmer);
  }

  // Center and size to the new height.
  Rect bounds(0, 0, root->rect().w, root->rect().h);
  set_rect(rect.CenterIn(bounds).MoveIn(bounds).Clip(bounds));
  root->AddChild(this);
  return true;
}

void MessageWindow::AddButton(TBID id, bool focused) {
  auto layout = GetElementById<LayoutBox>(3);
  if (!layout) return;
  Button* btn = new Button();
  btn->set_id(id);
  btn->set_text(util::GetString(btn->id()));
  layout->AddChild(btn);
  if (focused) {
    btn->set_focus(FocusReason::kUnknown);
  }
}

bool MessageWindow::OnEvent(const Event& ev) {
  if (ev.type == EventType::kClick && ev.target->IsOfType<Button>()) {
    WeakElementPointer this_element(this);

    // Invoke the click on the target.
    Event target_ev(EventType::kClick);
    target_ev.ref_id = ev.target->id();
    InvokeEvent(target_ev);

    // If target got deleted, close.
    if (this_element.get()) {
      Close();
    }
    return true;
  } else if (ev.type == EventType::kKeyDown &&
             ev.special_key == SpecialKey::kEsc) {
    Event click_ev(EventType::kClick);
    title_close_button_.InvokeEvent(click_ev);
    return true;
  }
  return Window::OnEvent(ev);
}

void MessageWindow::OnDie() {
  if (Element* dimmer = m_dimmer.get()) {
    dimmer->Die();
  }
}

void MessageWindow::OnElementDelete(Element* element) {
  // If the target element is deleted, close!
  if (!m_target.get()) {
    Close();
  }
}

bool MessageWindow::OnElementDying(Element* element) {
  // If the target element or an ancestor of it is dying, close!
  if (element == m_target.get() || element->IsAncestorOf(m_target.get())) {
    Close();
  }
  return false;
}

}  // namespace elements
}  // namespace tb
