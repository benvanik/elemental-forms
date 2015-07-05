/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb/elements.h"

namespace tb {

void RegisterBuiltinElementInflaters() {
  using namespace tb::elements;

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
}

}  // namespace tb
