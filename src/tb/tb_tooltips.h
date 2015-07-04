/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_TOOLTIPS_H
#define TB_TOOLTIPS_H

#include <memory>

#include "tb_popup_window.h"
#include "tb_text_box.h"
#include "tb_widgets_listener.h"

namespace tb {

// Implements functionality of tooltip popups, based on PopupWindow and
// contains TextBox as content viewer.
class TooltipWindow : public PopupWindow {
 public:
  TBOBJECT_SUBCLASS(TooltipWindow, PopupWindow);

  TooltipWindow(Element* target);
  ~TooltipWindow() override;

  bool Show(int mouse_x, int mouse_y);

  Point GetOffsetPoint() const { return Point(m_offset_x, m_offset_y); }

 private:
  Rect GetAlignedRect(int x, int y);

  TextBox m_content;
  int m_offset_x = 0;
  int m_offset_y = 0;
};

// Implements logic for show/hide tooltips.
class TooltipManager : private ElementListener, public MessageHandler {
 public:
  static TooltipManager* get() { return tooltip_manager_singleton_.get(); }
  static void set(std::unique_ptr<TooltipManager> value) {
    tooltip_manager_singleton_ = std::move(value);
  }

  TooltipManager();
  ~TooltipManager() override;

  // Offset by Y of tooltip point.
  uint32_t tooltip_point_offset_y = 20;
  // Delay in ms before tooltip will be shown.
  uint32_t tooltip_show_delay_ms = 700;
  // Tooltip display duration.
  uint32_t tooltip_show_duration_ms = 5000;
  // Distance by X or Y which used to hide tooltip.
  uint32_t tooltip_hide_point_dist = 40;

 private:
  bool OnElementInvokeEvent(Element* element, const ElementEvent& ev) override;
  void OnMessageReceived(Message* msg) override;
  void KillToolTip();

  void DeleteShowMessages();
  Element* GetTippedElement();

  static std::unique_ptr<TooltipManager> tooltip_manager_singleton_;

  TooltipWindow* m_tooltip = nullptr;
  Element* m_last_tipped_element = nullptr;
};

}  // namespace tb

#endif  // TB_TOOLTIPS_H
