/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include <algorithm>
#include <cassert>

#include "el/design/designer_form.h"
#include "el/elements/form.h"
#include "el/util/math.h"

namespace el {
namespace elements {

Form::Form() {
  set_background_skin(TBIDC("Form"), InvokeInfo::kNoCallbacks);
  AddChild(&title_mover_);
  AddChild(&title_resizer_);
  title_mover_.set_background_skin(TBIDC("Form.mover"));
  title_mover_.AddChild(&title_label_);
  title_label_.set_ignoring_input(true);
  title_mover_.AddChild(&title_design_button_);
  title_design_button_.set_background_skin(TBIDC("Form.close"));
  title_design_button_.set_focusable(false);
  title_design_button_.set_id(TBIDC("Form.design"));
  title_mover_.AddChild(&title_close_button_);
  title_close_button_.set_background_skin(TBIDC("Form.close"));
  title_close_button_.set_focusable(false);
  title_close_button_.set_id(TBIDC("Form.close"));
  set_group_root(true);
}

Form::~Form() {
  title_resizer_.RemoveFromParent();
  title_mover_.RemoveFromParent();
  title_design_button_.RemoveFromParent();
  title_close_button_.RemoveFromParent();
  title_mover_.RemoveChild(&title_label_);
}

Rect Form::GetResizeToFitContentRect(ResizeFit fit) {
  PreferredSize ps = GetPreferredSize();
  int new_w = ps.pref_w;
  int new_h = ps.pref_h;
  if (fit == ResizeFit::kMinimal) {
    new_w = ps.min_w;
    new_h = ps.min_h;
  } else if (fit == ResizeFit::kCurrentOrNeeded) {
    new_w = util::Clamp(rect().w, ps.min_w, ps.max_w);
    new_h = util::Clamp(rect().h, ps.min_h, ps.max_h);
  }
  if (parent()) {
    new_w = std::min(new_w, parent()->rect().w);
    new_h = std::min(new_h, parent()->rect().h);
  }
  return Rect(rect().x, rect().y, new_w, new_h);
}

void Form::ResizeToFitContent(ResizeFit fit) {
  set_rect(GetResizeToFitContentRect(fit));
}

void Form::Close() { Die(); }

bool Form::is_active() const { return has_state(Element::State::kSelected); }

Form* Form::GetTopMostOtherForm(bool only_activable_forms) {
  Form* other_form = nullptr;
  Element* sibling = parent()->last_child();
  while (sibling && !other_form) {
    if (sibling != this) {
      other_form = util::SafeCast<Form>(sibling);
    }
    if (only_activable_forms && other_form &&
        !any(other_form->form_settings_ & FormSettings::kCanActivate)) {
      other_form = nullptr;
    }
    sibling = sibling->GetPrev();
  }
  return other_form;
}

void Form::Activate() {
  if (!parent() || !any(form_settings_ & FormSettings::kCanActivate)) {
    return;
  }
  if (is_active()) {
    // Already active, but we may still have lost focus, so ensure it comes back
    // to us.
    EnsureFocus();
    return;
  }

  // Deactivate currently active form.
  Form* active_form = GetTopMostOtherForm(true);
  if (active_form) {
    active_form->Deactivate();
  }

  // Activate this form.
  set_z(ElementZ::kTop);
  SetFormActiveState(true);
  EnsureFocus();
}

bool Form::EnsureFocus() {
  // If we already have focus, we're done.
  if (focused_element && IsAncestorOf(focused_element)) {
    return true;
  }

  // Focus last focused element (if we have one)
  bool success = false;
  if (last_focus_element_.get()) {
    success = last_focus_element_.get()->set_focus(FocusReason::kUnknown);
  }
  // We didn't have one or failed, so try focus any child.
  if (!success) {
    success = set_focus_recursive(FocusReason::kUnknown);
  }
  return success;
}

void Form::Deactivate() {
  if (!is_active()) return;
  SetFormActiveState(false);
}

void Form::SetFormActiveState(bool active) {
  set_state(Element::State::kSelected, active);
  title_mover_.set_state(Element::State::kSelected, active);
}

void Form::set_settings(FormSettings settings) {
  if (settings == form_settings_) return;
  form_settings_ = settings;

  if (any(settings & FormSettings::kTitleBar)) {
    if (!title_mover_.parent()) {
      AddChild(&title_mover_);
    }
  } else {
    title_mover_.RemoveFromParent();
  }
  if (any(settings & FormSettings::kResizable)) {
    if (!title_resizer_.parent()) {
      AddChild(&title_resizer_);
    }
  } else {
    title_resizer_.RemoveFromParent();
  }
  if (any(settings & FormSettings::kDesignButton)) {
    if (!title_design_button_.parent()) {
      title_mover_.AddChild(&title_design_button_);
    }
  } else {
    title_design_button_.RemoveFromParent();
  }
  if (any(settings & FormSettings::kCloseButton)) {
    if (!title_close_button_.parent()) {
      title_mover_.AddChild(&title_close_button_);
    }
  } else {
    title_close_button_.RemoveFromParent();
  }

  if (any(settings & FormSettings::kFullScreen)) {
    set_background_skin(TBIDC("Form.fullscreen"));
    set_gravity(Gravity::kAll);
    FitToParent();
  } else {
    set_background_skin(TBIDC("Form"));
  }

  // FIX: invalidate layout / resize form!
  Invalidate();
}

void Form::CenterInParent() {
  if (!parent()) {
    return;
  }
  Rect bounds(0, 0, parent()->rect().w, parent()->rect().h);
  set_rect(rect().CenterIn(bounds).MoveIn(bounds).Clip(bounds));
}

int Form::title_bar_height() {
  if (any(form_settings_ & FormSettings::kTitleBar)) {
    return title_mover_.GetPreferredSize().pref_h;
  }
  return 0;
}

Rect Form::padding_rect() {
  Rect padding_rect = Element::padding_rect();
  int title_height = title_bar_height();
  padding_rect.y += title_height;
  padding_rect.h -= title_height;
  return padding_rect;
}

PreferredSize Form::OnCalculatePreferredSize(
    const SizeConstraints& constraints) {
  PreferredSize ps = OnCalculatePreferredContentSize(constraints);

  // Add form skin padding
  if (auto e = background_skin_element()) {
    ps.min_w += e->padding_left + e->padding_right;
    ps.pref_w += e->padding_left + e->padding_right;
    ps.min_h += e->padding_top + e->padding_bottom;
    ps.pref_h += e->padding_top + e->padding_bottom;
  }
  // Add form title bar height
  int title_height = title_bar_height();
  ps.min_h += title_height;
  ps.pref_h += title_height;
  return ps;
}

bool Form::OnEvent(const Event& ev) {
  if (ev.target == &title_close_button_) {
    if (ev.type == EventType::kClick) {
      Close();
      return true;
    }
  } else if (ev.target == &title_design_button_) {
    if (ev.type == EventType::kClick) {
      OpenDesigner();
      return true;
    }
  }
  return Element::OnEvent(ev);
}

void Form::OnAdded() {
  // If we was added last, call Activate to update status etc.
  if (parent()->last_child() == this) {
    Activate();
  }

  // Fit if needed.
  if (any(form_settings_ & FormSettings::kFullScreen)) {
    FitToParent();
  }
}

void Form::OnRemove() {
  Deactivate();

  // Active the top most other form
  if (Form* active_form = GetTopMostOtherForm(true)) active_form->Activate();
}

void Form::OnChildAdded(Element* child) {
  title_resizer_.set_z(ElementZ::kTop);
}

void Form::OnResized(int old_w, int old_h) {
  // Apply gravity on children.
  Element::OnResized(old_w, old_h);
  // Manually move our own decoration children.
  // FIX: Put a layout in the Mover so we can add things there nicely.
  int title_height = title_bar_height();
  title_mover_.set_rect({0, 0, rect().w, title_height});
  PreferredSize ps = title_resizer_.GetPreferredSize();
  title_resizer_.set_rect(
      {rect().w - ps.pref_w, rect().h - ps.pref_h, ps.pref_w, ps.pref_h});
  Rect title_mover_rect = title_mover_.padding_rect();
  int button_size = title_mover_rect.h;
  title_design_button_.set_rect(
      {title_mover_rect.x + title_mover_rect.w - button_size * 2,
       title_mover_rect.y, button_size, button_size});
  title_close_button_.set_rect(
      {title_mover_rect.x + title_mover_rect.w - button_size,
       title_mover_rect.y, button_size, button_size});
  if (any(form_settings_ & FormSettings::kDesignButton)) {
    title_mover_rect.w -= button_size;
  }
  if (any(form_settings_ & FormSettings::kCloseButton)) {
    title_mover_rect.w -= button_size;
  }
  title_label_.set_rect(title_mover_rect);
}

void Form::OpenDesigner() {
  auto designer_form = new design::DesignerForm();
  designer_form->BindContent(this);
  designer_form->Show(this);
}

}  // namespace elements
}  // namespace el
