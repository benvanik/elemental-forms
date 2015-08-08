/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include <algorithm>
#include <cmath>

#include "el/elements/popup_form.h"
#include "el/elements/text_box.h"
#include "el/tooltip_manager.h"

namespace el {

std::unique_ptr<TooltipManager> TooltipManager::tooltip_manager_singleton_;

namespace {

const TBID messageShow = TBIDC("TooltipManager.show");
const TBID messageHide = TBIDC("TooltipManager.hide");

class TTMsgParam : public util::TypedObject {
 public:
  TTMsgParam(Element* hovered) : m_hovered(hovered) {}

  Element* m_hovered;
};

}  // namespace

// Implements functionality of tooltip popups, based on PopupForm and
// contains TextBox as content viewer.
class TooltipForm : public elements::PopupForm {
 public:
  TBOBJECT_SUBCLASS(TooltipForm, elements::PopupForm);

  TooltipForm(Element* target);
  ~TooltipForm() override;

  bool Show(int mouse_x, int mouse_y);

  Point offset_point() const { return Point(m_offset_x, m_offset_y); }

 private:
  Rect GetAlignedRect(int x, int y);

  elements::TextBox m_content;
  int m_offset_x = 0;
  int m_offset_y = 0;
};

TooltipForm::TooltipForm(Element* target) : PopupForm(target) {
  set_background_skin("", InvokeInfo::kNoCallbacks);
  set_settings(elements::FormSettings::kNone);
  m_content.set_background_skin(TBIDC("TBTooltip"), InvokeInfo::kNoCallbacks);
  m_content.set_focusable(false);
  m_content.set_styled(true);
  m_content.set_gravity(Gravity::kAll);
  m_content.set_read_only(true);
  m_content.set_multiline(true);
  m_content.set_text(target->tooltip());
  m_content.set_adapt_to_content_size(true);
  AddChild(&m_content);
}

TooltipForm::~TooltipForm() { RemoveChild(&m_content); }

bool TooltipForm::Show(int mouse_x, int mouse_y) {
  m_offset_x = mouse_x;
  m_offset_y = mouse_y;

  target_element().get()->parent_root()->AddChild(this);
  set_rect(GetAlignedRect(m_offset_x, mouse_y));
  return true;
}

Rect TooltipForm::GetAlignedRect(int x, int y) {
  Element* root = parent_root();

  SizeConstraints sc(root->rect().w, root->rect().h);

  PreferredSize ps = GetPreferredSize(sc);

  Point pos(x, y);
  int w = std::min(ps.pref_w, root->rect().w);
  int h = std::min(ps.pref_h, root->rect().h);

  x = pos.x + w > root->rect().w ? pos.x - w : pos.x;
  y = pos.y;
  if (pos.y + h > root->rect().h) {
    y = pos.y - TooltipManager::get()->tooltip_point_offset_y - h;
  }

  return Rect(x, y, w, h);
}

TooltipManager::TooltipManager() { ElementListener::AddGlobalListener(this); }

TooltipManager::~TooltipManager() {
  ElementListener::RemoveGlobalListener(this);
}

bool TooltipManager::OnElementInvokeEvent(Element* element, const Event& ev) {
  if (ev.type == EventType::kPointerMove && !Element::captured_element) {
    Element* tipped_element = GetTippedElement();
    if (m_last_tipped_element != tipped_element && tipped_element) {
      auto msg_data = std::make_unique<MessageData>();
      msg_data->v1.set_object(new TTMsgParam(tipped_element));
      PostMessageDelayed(messageShow, std::move(msg_data),
                         tooltip_show_delay_ms);
    } else if (m_last_tipped_element == tipped_element && tipped_element &&
               m_tooltip) {
      int x = Element::pointer_move_element_x;
      int y = Element::pointer_move_element_y;
      tipped_element->ConvertToRoot(x, y);
      y += tooltip_point_offset_y;
      Point tt_point = m_tooltip->offset_point();
      if (abs(tt_point.x - x) > (int)tooltip_hide_point_dist ||
          abs(tt_point.y - y) > (int)tooltip_hide_point_dist) {
        KillToolTip();
        DeleteShowMessages();
      }
    } else if (!tipped_element) {
      KillToolTip();
      DeleteShowMessages();
    }
    m_last_tipped_element = tipped_element;
  } else {
    KillToolTip();
    DeleteShowMessages();
  }

  return false;
}

void TooltipManager::KillToolTip() {
  if (m_tooltip) {
    m_tooltip->Close();
    m_tooltip = nullptr;
  }
}

void TooltipManager::DeleteShowMessages() {
  Message* msg;
  while ((msg = GetMessageById(messageShow)) != nullptr) {
    DeleteMessage(msg);
  }
}

Element* TooltipManager::GetTippedElement() {
  Element* current = Element::hovered_element;
  while (current && current->tooltip().empty()) {
    current = current->parent();
  }
  return current;
}

void TooltipManager::OnMessageReceived(Message* msg) {
  if (msg->message_id() == messageShow) {
    Element* tipped_element = GetTippedElement();
    TTMsgParam* param = static_cast<TTMsgParam*>(msg->data()->v1.as_object());
    if (tipped_element == param->m_hovered) {
      KillToolTip();

      m_tooltip = new TooltipForm(tipped_element);

      int x = Element::pointer_move_element_x;
      int y = Element::pointer_move_element_y;
      Element::hovered_element->ConvertToRoot(x, y);
      y += tooltip_point_offset_y;

      m_tooltip->Show(x, y);

      auto msg_data = std::make_unique<MessageData>();
      msg_data->v1.set_object(new TTMsgParam(m_tooltip));
      PostMessageDelayed(messageHide, std::move(msg_data),
                         tooltip_show_duration_ms);
    }
  } else if (msg->message_id() == messageHide) {
    TTMsgParam* param = static_cast<TTMsgParam*>(msg->data()->v1.as_object());
    if (m_tooltip == param->m_hovered) {
      KillToolTip();
    }
  }
}

}  // namespace el
