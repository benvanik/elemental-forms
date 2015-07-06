/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_TOOLTIP_MANAGER_H_
#define TB_TOOLTIP_MANAGER_H_

#include <memory>

#include "tb/element_listener.h"
#include "tb/message_handler.h"

namespace tb {

class TooltipWindow;

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
  bool OnElementInvokeEvent(Element* element, const Event& ev) override;
  void OnMessageReceived(Message* msg) override;
  void KillToolTip();

  void DeleteShowMessages();
  Element* GetTippedElement();

  static std::unique_ptr<TooltipManager> tooltip_manager_singleton_;

  TooltipWindow* m_tooltip = nullptr;
  Element* m_last_tipped_element = nullptr;
};

}  // namespace tb

#endif  // TB_TOOLTIP_MANAGER_H_
