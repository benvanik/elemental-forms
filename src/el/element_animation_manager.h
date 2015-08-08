/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_ELEMENT_ANIMATION_MANAGER_H_
#define EL_ELEMENT_ANIMATION_MANAGER_H_

#include "el/element_listener.h"

namespace el {

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

}  // namespace el

#endif  // EL_ELEMENT_ANIMATION_MANAGER_H_
