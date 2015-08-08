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
  using namespace el::elements;

  Box::RegisterInflater();
  ImageBox::RegisterInflater();
  SpinBox::RegisterInflater();
  LayoutBox::RegisterInflater();
  ScrollContainer::RegisterInflater();
  DropDownButton::RegisterInflater();
  ListBox::RegisterInflater();
  TabContainer::RegisterInflater();
  TextBox::RegisterInflater();
  GroupBox::RegisterInflater();
  ToggleContainer::RegisterInflater();
  Element::RegisterInflater();
  Label::RegisterInflater();
  Button::RegisterInflater();
  LabelContainer::RegisterInflater();
  IconBox::RegisterInflater();
  Separator::RegisterInflater();
  ProgressSpinner::RegisterInflater();
  CheckBox::RegisterInflater();
  RadioButton::RegisterInflater();
  ScrollBar::RegisterInflater();
  Slider::RegisterInflater();
  Mover::RegisterInflater();
  Resizer::RegisterInflater();
  Dimmer::RegisterInflater();
  SplitContainer::RegisterInflater();
}

}  // namespace el
