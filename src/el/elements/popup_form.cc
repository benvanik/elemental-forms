/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include <algorithm>

#include "el/elements/popup_form.h"
#include "el/util/math.h"

namespace el {
namespace elements {

Rect PopupAlignment::GetAlignedRect(Element* popup, Element* target) const {
  Element* root = target->parent_root();

  SizeConstraints sc(root->rect().w, root->rect().h);

  PreferredSize ps = popup->GetPreferredSize(sc);

  // Amount of pixels that should be avoided if the target rect needs to be
  // moved.
  int avoid_w = 0, avoid_h = 0;

  int x = 0, y = 0;
  int w = std::min(ps.pref_w, root->rect().w);
  int h = std::min(ps.pref_h, root->rect().h);

  if (pos_in_root.x != kUnspecified && pos_in_root.y != kUnspecified) {
    x = pos_in_root.x;
    y = pos_in_root.y;
    avoid_w = pos_offset.x;
    avoid_h = pos_offset.y;
    // Make sure it's moved into view horizontally.
    if (align == Align::kTop || align == Align::kBottom) {
      x = util::Clamp(x, 0, root->rect().w - w);
    }
  } else {
    target->ConvertToRoot(&x, &y);

    if (align == Align::kTop || align == Align::kBottom) {
      if (expand_to_target_width) {
        w = std::max(w, target->rect().w);
      }

      // If the menu is aligned top or bottom, limit its height to the worst
      // case available height.
      // Being in the center of the root, that is half the root height minus the
      // target rect.
      h = std::min(h, root->rect().h / 2 - target->rect().h);
    }
    avoid_w = target->rect().w;
    avoid_h = target->rect().h;
  }

  if (align == Align::kBottom) {
    y = y + avoid_h + h > root->rect().h ? y - h : y + avoid_h;
  } else if (align == Align::kTop) {
    y = y - h < 0 ? y + avoid_h : y - h;
  } else if (align == Align::kRight) {
    x = x + avoid_w + w > root->rect().w ? x - w : x + avoid_w;
    y = std::min(y, root->rect().h - h);
  } else {  // if (align == Align::kLeft)
    x = x - w < 0 ? x + avoid_w : x - w;
    y = std::min(y, root->rect().h - h);
  }
  return Rect(x, y, w, h);
}

PopupForm::PopupForm(Element* target) : m_target(target) {
  ElementListener::AddGlobalListener(this);
  set_background_skin(TBIDC("PopupForm"), InvokeInfo::kNoCallbacks);
  set_settings(FormSettings::kNone);
}

PopupForm::~PopupForm() { ElementListener::RemoveGlobalListener(this); }

void PopupForm::Show(const PopupAlignment& alignment) {
  // Calculate and set a good size for the popup form
  set_rect(alignment.GetAlignedRect(this, m_target.get()));

  Element* root = m_target.get()->parent_root();
  root->AddChild(this);
}

bool PopupForm::OnEvent(const Event& ev) {
  if (ev.type == EventType::kKeyDown && ev.special_key == SpecialKey::kEsc) {
    Close();
    return true;
  }
  return Form::OnEvent(ev);
}

void PopupForm::OnElementFocusChanged(Element* element, bool focused) {
  if (focused && !IsEventDestinationFor(element)) {
    Close();
  }
}

bool PopupForm::OnElementInvokeEvent(Element* element, const Event& ev) {
  if ((ev.type == EventType::kPointerDown ||
       ev.type == EventType::kContextMenu) &&
      !IsEventDestinationFor(ev.target)) {
    Close();
  }
  return false;
}

void PopupForm::OnElementDelete(Element* element) {
  // If the target element is deleted, close!
  if (!m_target.get()) {
    Close();
  }
}

bool PopupForm::OnElementDying(Element* element) {
  // If the target element or an ancestor of it is dying, close!
  if (element == m_target.get() || element->IsAncestorOf(m_target.get())) {
    Close();
  }
  return false;
}

}  // namespace elements
}  // namespace el
