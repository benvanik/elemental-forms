/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_tooltips.h"

#include <algorithm>
#include <cmath>

#include "tb_language.h"
#include "tb_widgets_listener.h"

namespace tb {

tb::TooltipManager* tb::g_tooltip_mng = nullptr;

namespace {

const TBID messageShow = TBIDC("TooltipManager.show");
const TBID messageHide = TBIDC("TooltipManager.hide");

class TTMsgParam : public TypedObject {
 public:
  TTMsgParam(Widget* hovered) : m_hovered(hovered) {}

  Widget* m_hovered;
};

}  // namespace

TooltipWindow::TooltipWindow(Widget* target) : PopupWindow(target) {
  SetSkinBg("", InvokeInfo::kNoCallbacks);
  SetSettings(WindowSettings::kNone);
  m_content.SetSkinBg(TBIDC("TBTooltip"), InvokeInfo::kNoCallbacks);
  m_content.SetIsFocusable(false);
  m_content.SetStyling(true);
  m_content.SetGravity(Gravity::kAll);
  m_content.SetReadOnly(true);
  m_content.SetMultiline(true);
  m_content.SetText(target->GetDescription());
  m_content.SetAdaptToContentSize(true);
  AddChild(&m_content);
}

TooltipWindow::~TooltipWindow() { RemoveChild(&m_content); }

bool TooltipWindow::Show(int mouse_x, int mouse_y) {
  m_offset_x = mouse_x;
  m_offset_y = mouse_y;

  GetTargetWidget().Get()->GetParentRoot()->AddChild(this);
  SetRect(GetAlignedRect(m_offset_x, mouse_y));
  return true;
}

Rect TooltipWindow::GetAlignedRect(int x, int y) {
  Widget* root = GetParentRoot();

  SizeConstraints sc(root->GetRect().w, root->GetRect().h);

  PreferredSize ps = GetPreferredSize(sc);

  Point pos(x, y);
  int w = std::min(ps.pref_w, root->GetRect().w);
  int h = std::min(ps.pref_h, root->GetRect().h);

  x = pos.x + w > root->GetRect().w ? pos.x - w : pos.x;
  y = pos.y;
  if (pos.y + h > root->GetRect().h) {
    y = pos.y - g_tooltip_mng->tooltip_point_offset_y - h;
  }

  return Rect(x, y, w, h);
}

TooltipManager::TooltipManager() { WidgetListener::AddGlobalListener(this); }

TooltipManager::~TooltipManager() {
  WidgetListener::RemoveGlobalListener(this);
}

bool TooltipManager::OnWidgetInvokeEvent(Widget* widget,
                                         const WidgetEvent& ev) {
  if (ev.type == EventType::kPointerMove && !Widget::captured_widget) {
    Widget* tipped_widget = GetTippedWidget();
    if (m_last_tipped_widget != tipped_widget && tipped_widget) {
      MessageData* msg_data = new MessageData();
      msg_data->v1.SetObject(new TTMsgParam(tipped_widget));
      PostMessageDelayed(messageShow, msg_data, tooltip_show_delay_ms);
    } else if (m_last_tipped_widget == tipped_widget && tipped_widget &&
               m_tooltip) {
      int x = Widget::pointer_move_widget_x;
      int y = Widget::pointer_move_widget_y;
      tipped_widget->ConvertToRoot(x, y);
      y += tooltip_point_offset_y;
      Point tt_point = m_tooltip->GetOffsetPoint();
      if (abs(tt_point.x - x) > (int)tooltip_hide_point_dist ||
          abs(tt_point.y - y) > (int)tooltip_hide_point_dist) {
        KillToolTip();
        DeleteShowMessages();
      }
    } else if (!tipped_widget) {
      KillToolTip();
      DeleteShowMessages();
    }
    m_last_tipped_widget = tipped_widget;
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
  while ((msg = GetMessageByID(messageShow)) != nullptr) {
    DeleteMessage(msg);
  }
}

Widget* TooltipManager::GetTippedWidget() {
  Widget* current = Widget::hovered_widget;
  while (current && current->GetDescription().empty()) {
    current = current->GetParent();
  }
  return current;
}

void TooltipManager::OnMessageReceived(Message* msg) {
  if (msg->message == messageShow) {
    Widget* tipped_widget = GetTippedWidget();
    TTMsgParam* param = static_cast<TTMsgParam*>(msg->data->v1.GetObject());
    if (tipped_widget == param->m_hovered) {
      KillToolTip();

      m_tooltip = new TooltipWindow(tipped_widget);

      int x = Widget::pointer_move_widget_x;
      int y = Widget::pointer_move_widget_y;
      Widget::hovered_widget->ConvertToRoot(x, y);
      y += tooltip_point_offset_y;

      m_tooltip->Show(x, y);

      MessageData* msg_data = new MessageData();
      msg_data->v1.SetObject(new TTMsgParam(m_tooltip));
      PostMessageDelayed(messageHide, msg_data, tooltip_show_duration_ms);
    }
  } else if (msg->message == messageHide) {
    TTMsgParam* param = static_cast<TTMsgParam*>(msg->data->v1.GetObject());
    if (m_tooltip == param->m_hovered) {
      KillToolTip();
    }
  }
}

}  // namespace tb
