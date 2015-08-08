/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/elements.h"

namespace el {

void RegisterBuiltinElementInflaters() {
  Element::RegisterInflater();
  elements::Box::RegisterInflater();
  elements::Button::RegisterInflater();
  elements::CheckBox::RegisterInflater();
  elements::Dimmer::RegisterInflater();
  elements::DropDownButton::RegisterInflater();
  elements::GroupBox::RegisterInflater();
  elements::IconBox::RegisterInflater();
  elements::ImageBox::RegisterInflater();
  elements::Label::RegisterInflater();
  elements::LabelContainer::RegisterInflater();
  elements::LayoutBox::RegisterInflater();
  elements::ListBox::RegisterInflater();
  elements::Mover::RegisterInflater();
  elements::ProgressSpinner::RegisterInflater();
  elements::RadioButton::RegisterInflater();
  elements::Resizer::RegisterInflater();
  elements::ScrollBar::RegisterInflater();
  elements::ScrollContainer::RegisterInflater();
  elements::Separator::RegisterInflater();
  elements::Slider::RegisterInflater();
  elements::SpinBox::RegisterInflater();
  elements::SplitContainer::RegisterInflater();
  elements::TabContainer::RegisterInflater();
  elements::TextBox::RegisterInflater();
  elements::ToggleContainer::RegisterInflater();
}

}  // namespace el
