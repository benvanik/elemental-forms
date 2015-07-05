/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_PARTS_CONTAINER_H_
#define TB_PARTS_CONTAINER_H_

#include "tb/element.h"

namespace tb {
namespace parts {

// Container is just a Element with border and padding (using skin
// "Container").
class Container : public Element {
 public:
  TBOBJECT_SUBCLASS(Container, Element);
  static void RegisterInflater();

  Container();
};

}  // namespace parts
}  // namespace tb

#endif  // TB_PARTS_CONTAINER_H_
