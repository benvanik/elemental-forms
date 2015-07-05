/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb/elements.h"

#include "tb/parts/dimmer.h"
#include "tb/parts/mover.h"
#include "tb/parts/resizer.h"

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

  parts::Mover::RegisterInflater();
  parts::Resizer::RegisterInflater();
  parts::Dimmer::RegisterInflater();
}

}  // namespace tb
