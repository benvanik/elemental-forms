/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_ELEMENTS_SPLIT_CONTAINER_H_
#define EL_ELEMENTS_SPLIT_CONTAINER_H_

#include "el/element.h"
#include "el/elements/box.h"

namespace el {
namespace elements {

// Specifies how resizing of the split container is handled.
enum class FixedPane {
  // Value controls the first pane's size and the first pane will remain fixed
  // during resizing.
  kFirst,
  // Value controls the second pane's size and the second pane will remain fixed
  // during resizing.
  kSecond,
};
MAKE_ORDERED_ENUM_STRING_UTILS(FixedPane, "first", "second");

// An element that provides two containers resizable with a divider.
class SplitContainer : public Element {
 public:
  TBOBJECT_SUBCLASS(SplitContainer, Element);
  static void RegisterInflater();

  SplitContainer();
  ~SplitContainer() override;

  Axis axis() const { return axis_; }
  // Sets the axis the divider splits across. X is horizontal, Y is vertical.
  void set_axis(Axis axis);

  FixedPane fixed_pane() const { return fixed_pane_; }
  // Sets how resizing of the split container is handled.
  void set_fixed_pane(FixedPane fixed_pane);

  int value() override { return value_; }
  // Sets the split % of this element.
  void set_value(int value) override;

  int min_value() const { return min_value_; }
  int max_value() const { return max_value_; }
  int computed_min_value() const;
  int computed_max_value() const;
  // Sets the min, max value for the splitter.
  void set_limits(int min_value, int max_value);

  void OnInflate(const parsing::InflateInfo& info) override;
  void OnProcess() override;
  void OnResized(int old_w, int old_h) override;
  void OnChildAdded(Element* child) override;
  PreferredSize OnCalculatePreferredContentSize(
      const SizeConstraints& constraints) override;

  Element* first_pane() { return &first_pane_; }
  Element* second_pane() { return &second_pane_; }

 private:
  class Divider : public Element {
   public:
    bool OnEvent(const Event& ev) override;
  };

  void ValidateLayout(const SizeConstraints& constraints);

  Axis axis_ = Axis::kX;
  FixedPane fixed_pane_ = FixedPane::kFirst;
  int initial_value_ = util::kInvalidDimension;
  int value_ = 0;
  int min_value_ = 0;
  int max_value_ = util::kInvalidDimension;
  Element first_pane_;
  Element second_pane_;
  Divider divider_;
};

}  // namespace elements
}  // namespace el

#endif  // EL_ELEMENTS_SPLIT_CONTAINER_H_
