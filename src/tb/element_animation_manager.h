/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_ELEMENT_ANIMATION_MANAGER_H_
#define TB_ELEMENT_ANIMATION_MANAGER_H_

#include "tb/element_listener.h"

namespace tb {

class ElementAnimationManager : public ElementListener {
 public:
  static void Init();
  static void Shutdown();

  // Aborts all animations that are running for the given element.
  static void AbortAnimations(Element* element);

  // Abort all animations matching the given type that are running for the given
  // element.
  // This example will abort all opacity animations:
  // AbortAnimations(element,
  //     TypedObject::GetTypeId<OpacityElementAnimation>())
  static void AbortAnimations(Element* element, util::tb_type_id_t type_id);

 private:
  void OnElementDelete(Element* element) override;
  bool OnElementDying(Element* element) override;
  void OnElementAdded(Element* parent, Element* child) override;
  void OnElementRemove(Element* parent, Element* child) override;
};

}  // namespace tb

#endif  // TB_ELEMENT_ANIMATION_MANAGER_H_
