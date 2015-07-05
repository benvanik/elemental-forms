/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_ELEMENTS_SEPARATOR_H_
#define TB_ELEMENTS_SEPARATOR_H_

#include "tb/element.h"

namespace tb {
namespace elements {

// A element only showing a skin.
// It is disabled by default.
class Separator : public Element {
 public:
  TBOBJECT_SUBCLASS(Separator, Element);
  static void RegisterInflater();

  Separator();
};

}  // namespace elements
}  // namespace tb

#endif  // TB_ELEMENTS_SEPARATOR_H_
