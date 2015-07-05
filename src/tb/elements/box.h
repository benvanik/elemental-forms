/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_ELEMENTS_BOX_H_
#define TB_ELEMENTS_BOX_H_

#include "tb/element.h"

namespace tb {
namespace elements {

// Box is just a Element with border and padding (using skin
// "Box").
class Box : public Element {
 public:
  TBOBJECT_SUBCLASS(Box, Element);
  static void RegisterInflater();

  Box();
};

}  // namespace elements
}  // namespace tb

#endif  // TB_ELEMENTS_BOX_H_
