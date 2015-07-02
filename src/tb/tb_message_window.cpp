/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_message_window.h"

#include <cassert>

#include "tb_language.h"
#include "tb_text_box.h"
#include "tb_widgets_reader.h"

namespace tb {

MessageWindow::MessageWindow(TBWidget* target, TBID id) : m_target(target) {
  WidgetListener::AddGlobalListener(this);
  SetID(id);
}

MessageWindow::~MessageWindow() {
  WidgetListener::RemoveGlobalListener(this);
  if (TBWidget* dimmer = m_dimmer.Get()) {
    dimmer->GetParent()->RemoveChild(dimmer);
    delete dimmer;
  }
}

bool MessageWindow::Show(const std::string& title, const std::string& message,
                         MessageWindowSettings* settings) {
  TBWidget* target = m_target.Get();
  if (!target) return false;

  MessageWindowSettings default_settings;
  if (!settings) {
    settings = &default_settings;
  }

  TBWidget* root = target->GetParentRoot();

  const char* source =
      "Layout: axis: y, distribution: available\n"
      "	Layout: distribution: available, size: available\n"
      "		SkinImage: id: 2\n"
      "		TextBox: multiline: 1, readonly: 1, id: 1\n"
      "	Layout: distribution-position: right bottom, id: 3\n";
  if (!g_widgets_reader->LoadData(GetContentRoot(), source)) {
    return false;
  }

  SetText(title);

  GetWidgetByIDAndType<SkinImage>(2)->SetSkinBg(settings->icon_skin);

  TextBox* text_box = GetWidgetByIDAndType<TextBox>(1);
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
  Rect rect = GetRect();

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
  Rect bounds(0, 0, root->GetRect().w, root->GetRect().h);
  SetRect(rect.CenterIn(bounds).MoveIn(bounds).Clip(bounds));
  root->AddChild(this);
  return true;
}

void MessageWindow::AddButton(TBID id, bool focused) {
  Layout* layout = GetWidgetByIDAndType<Layout>(3);
  if (!layout) return;
  Button* btn = new Button();
  btn->SetID(id);
  btn->SetText(g_tb_lng->GetString(btn->GetID()));
  layout->AddChild(btn);
  if (focused) {
    btn->SetFocus(FocusReason::kUnknown);
  }
}

bool MessageWindow::OnEvent(const TBWidgetEvent& ev) {
  if (ev.type == EventType::kClick && ev.target->IsOfType<Button>()) {
    WeakWidgetPointer this_widget(this);

    // Invoke the click on the target.
    TBWidgetEvent target_ev(EventType::kClick);
    target_ev.ref_id = ev.target->GetID();
    InvokeEvent(target_ev);

    // If target got deleted, close.
    if (this_widget.Get()) {
      Close();
    }
    return true;
  } else if (ev.type == EventType::kKeyDown &&
             ev.special_key == SpecialKey::kEsc) {
    TBWidgetEvent click_ev(EventType::kClick);
    m_close_button.InvokeEvent(click_ev);
    return true;
  }
  return Window::OnEvent(ev);
}

void MessageWindow::OnDie() {
  if (TBWidget* dimmer = m_dimmer.Get()) {
    dimmer->Die();
  }
}

void MessageWindow::OnWidgetDelete(TBWidget* widget) {
  // If the target widget is deleted, close!
  if (!m_target.Get()) {
    Close();
  }
}

bool MessageWindow::OnWidgetDying(TBWidget* widget) {
  // If the target widget or an ancestor of it is dying, close!
  if (widget == m_target.Get() || widget->IsAncestorOf(m_target.Get())) {
    Close();
  }
  return false;
}

}  // namespace tb
