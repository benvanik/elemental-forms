/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_PARTS_RESIZER_H_
#define TB_PARTS_RESIZER_H_

#include "tb/element.h"

namespace tb {
namespace parts {

// A lower right corner resize grip.
// It will resize its parent element.
class Resizer : public Element {
 public:
  TBOBJECT_SUBCLASS(Resizer, Element);
  static void RegisterInflater();

  Resizer();

  HitStatus GetHitStatus(int x, int y) override;
  bool OnEvent(const ElementEvent& ev) override;
};

}  // namespace parts
}  // namespace tb

#endif  // TB_PARTS_RESIZER_H_
