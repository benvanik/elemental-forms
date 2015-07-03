/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_popup_window.h"

#include <algorithm>

#include "tb_widgets_listener.h"

namespace tb {

Rect PopupAlignment::GetAlignedRect(Element* popup, Element* target) const {
  Element* root = target->GetParentRoot();

  SizeConstraints sc(root->GetRect().w, root->GetRect().h);

  PreferredSize ps = popup->GetPreferredSize(sc);

  // Amount of pixels that should be avoided if the target rect needs to be
  // moved.
  int avoid_w = 0, avoid_h = 0;

  int x = 0, y = 0;
  int w = std::min(ps.pref_w, root->GetRect().w);
  int h = std::min(ps.pref_h, root->GetRect().h);

  if (pos_in_root.x != kUnspecified && pos_in_root.y != kUnspecified) {
    x = pos_in_root.x;
    y = pos_in_root.y;
    avoid_w = pos_offset.x;
    avoid_h = pos_offset.y;
    // Make sure it's moved into view horizontally.
    if (align == Align::kTop || align == Align::kBottom) {
      x = Clamp(x, 0, root->GetRect().w - w);
    }
  } else {
    target->ConvertToRoot(x, y);

    if (align == Align::kTop || align == Align::kBottom) {
      if (expand_to_target_width) {
        w = std::max(w, target->GetRect().w);
      }

      // If the menu is aligned top or bottom, limit its height to the worst
      // case available height.
      // Being in the center of the root, that is half the root height minus the
      // target rect.
      h = std::min(h, root->GetRect().h / 2 - target->GetRect().h);
    }
    avoid_w = target->GetRect().w;
    avoid_h = target->GetRect().h;
  }

  if (align == Align::kBottom) {
    y = y + avoid_h + h > root->GetRect().h ? y - h : y + avoid_h;
  } else if (align == Align::kTop) {
    y = y - h < 0 ? y + avoid_h : y - h;
  } else if (align == Align::kRight) {
    x = x + avoid_w + w > root->GetRect().w ? x - w : x + avoid_w;
    y = std::min(y, root->GetRect().h - h);
  } else {  // if (align == Align::kLeft)
    x = x - w < 0 ? x + avoid_w : x - w;
    y = std::min(y, root->GetRect().h - h);
  }
  return Rect(x, y, w, h);
}

PopupWindow::PopupWindow(Element* target) : m_target(target) {
  ElementListener::AddGlobalListener(this);
  SetSkinBg(TBIDC("PopupWindow"), InvokeInfo::kNoCallbacks);
  SetSettings(WindowSettings::kNone);
}

PopupWindow::~PopupWindow() { ElementListener::RemoveGlobalListener(this); }

bool PopupWindow::Show(const PopupAlignment& alignment) {
  // Calculate and set a good size for the popup window
  SetRect(alignment.GetAlignedRect(this, m_target.Get()));

  Element* root = m_target.Get()->GetParentRoot();
  root->AddChild(this);
  return true;
}

bool PopupWindow::OnEvent(const ElementEvent& ev) {
  if (ev.type == EventType::kKeyDown && ev.special_key == SpecialKey::kEsc) {
    Close();
    return true;
  }
  return Window::OnEvent(ev);
}

void PopupWindow::OnElementFocusChanged(Element* element, bool focused) {
  if (focused && !IsEventDestinationFor(element)) {
    Close();
  }
}

bool PopupWindow::OnElementInvokeEvent(Element* element,
                                       const ElementEvent& ev) {
  if ((ev.type == EventType::kPointerDown ||
       ev.type == EventType::kContextMenu) &&
      !IsEventDestinationFor(ev.target)) {
    Close();
  }
  return false;
}

void PopupWindow::OnElementDelete(Element* element) {
  // If the target element is deleted, close!
  if (!m_target.Get()) {
    Close();
  }
}

bool PopupWindow::OnElementDying(Element* element) {
  // If the target element or an ancestor of it is dying, close!
  if (element == m_target.Get() || element->IsAncestorOf(m_target.Get())) {
    Close();
  }
  return false;
}

}  // namespace tb
