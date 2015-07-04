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
  ImageElement::RegisterInflater();
  SelectInline::RegisterInflater();
  Layout::RegisterInflater();
  ScrollContainer::RegisterInflater();
  SelectDropdown::RegisterInflater();
  SelectList::RegisterInflater();
  TabContainer::RegisterInflater();
  TextBox::RegisterInflater();
  Section::RegisterInflater();
  ToggleContainer::RegisterInflater();
  Element::RegisterInflater();
  Label::RegisterInflater();
  Button::RegisterInflater();
  LabelContainer::RegisterInflater();
  SkinImage::RegisterInflater();
  Separator::RegisterInflater();
  ProgressSpinner::RegisterInflater();
  CheckBox::RegisterInflater();
  RadioButton::RegisterInflater();
  ScrollBar::RegisterInflater();
  Slider::RegisterInflater();
  Container::RegisterInflater();
  Mover::RegisterInflater();
  Resizer::RegisterInflater();
  Dimmer::RegisterInflater();
}

}  // namespace tb
