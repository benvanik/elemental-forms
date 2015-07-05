/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_PARTS_MOVER_H_
#define TB_PARTS_MOVER_H_

#include "tb/element.h"

namespace tb {
namespace parts {

// Moves its parent element when dragged.
class Mover : public Element {
 public:
  TBOBJECT_SUBCLASS(Mover, Element);
  static void RegisterInflater();

  Mover();

  bool OnEvent(const ElementEvent& ev) override;
};

}  // namespace parts
}  // namespace tb

#endif  // TB_PARTS_MOVER_H_
