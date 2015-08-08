/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_ELEMENTS_H_
#define EL_ELEMENTS_H_

#include "el/element.h"

#include "el/elements/box.h"
#include "el/elements/button.h"
#include "el/elements/check_box.h"
#include "el/elements/dimmer.h"
#include "el/elements/drop_down_button.h"
#include "el/elements/form.h"
#include "el/elements/group_box.h"
#include "el/elements/icon_box.h"
#include "el/elements/image_box.h"
#include "el/elements/label.h"
#include "el/elements/label_container.h"
#include "el/elements/layout_box.h"
#include "el/elements/list_box.h"
#include "el/elements/menu_form.h"
#include "el/elements/message_form.h"
#include "el/elements/modal_form.h"
#include "el/elements/mover.h"
#include "el/elements/popup_form.h"
#include "el/elements/progress_spinner.h"
#include "el/elements/radio_button.h"
#include "el/elements/resizer.h"
#include "el/elements/scroll_bar.h"
#include "el/elements/scroll_container.h"
#include "el/elements/separator.h"
#include "el/elements/slider.h"
#include "el/elements/spin_box.h"
#include "el/elements/split_container.h"
#include "el/elements/tab_container.h"
#include "el/elements/text_box.h"
#include "el/elements/toggle_container.h"

namespace el {

// TODO(benvanik): using all the elements to bring them into tb?
using namespace el::elements;

using el::elements::to_string;

}  // namespace el

#endif  // EL_ELEMENTS_H_
