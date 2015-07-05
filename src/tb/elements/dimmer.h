/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_ELEMENTS_DIMMER_H_
#define TB_ELEMENTS_DIMMER_H_

#include "tb/element.h"

namespace tb {
namespace elements {

// Dims elements in the background and blocks input.
class Dimmer : public Element {
 public:
  TBOBJECT_SUBCLASS(Dimmer, Element);
  static void RegisterInflater();

  Dimmer();

  void OnAdded() override;
};

}  // namespace elements
}  // namespace tb

#endif  // TB_ELEMENTS_DIMMER_H_
