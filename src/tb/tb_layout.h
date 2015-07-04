/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_LAYOUT_H
#define TB_LAYOUT_H

#include "tb_widgets.h"
#include "tb/types.h"

namespace tb {

// Specifies which height elements in a Axis::kX layout should have, or which
// width elements in a Axis::kY layout should have.
// No matter what, it will still prioritize minimum and maximum for each
// element.
enum class LayoutSize {
  kGravity,    // Sizes depend on the gravity for each element. (If the element
               // pulls towards both directions, it should grow to all available
               // space).
  kPreferred,  // Size will be the preferred so each element may be sized
               // differently.
  kAvailable   // Size should grow to all available space.
};
MAKE_ORDERED_ENUM_STRING_UTILS(LayoutSize, "gravity", "preferred", "available");

// Specifies which y position elements in a Axis::kX layout should have, or
// which x position elements in a Axis::kY layout should have.
enum class LayoutPosition {
  kCenter,   // Position is centered.
  kLeftTop,  // Position is to the left for Axis::kY layout and top for Axis::kX
             // layout.
  kRightBottom,  // Position is to the right for Axis::kY layout and bottom for
                 // Axis::kX layout.
  kGravity,  // Position depend on the gravity for each element. (If the element
             // pulls towards both directions, it will be centered).
};
MAKE_ORDERED_ENUM_STRING_UTILS(LayoutPosition, "center", "left top",
                               "right bottom", "gravity");

// Specifies which width elements in a Axis::kX layout should have, or which
// height elements in a Axis::kY layout should have.
enum class LayoutDistribution {
  kPreferred,  // Size will be the preferred so each element may be sized
               // differently.
  kAvailable,  // Size should grow to all available space.
  kGravity,    // Sizes depend on the gravity for each element. (If the element
               // pulls towards both directions, it should grow to all available
               // space).
};
MAKE_ORDERED_ENUM_STRING_UTILS(LayoutDistribution, "preferred", "available",
                               "gravity");

// Specifies how elements should be moved horizontally in a Axis::kX layout (or
// vertically in a Axis::kY layout) if there is extra space available.
enum class LayoutDistributionPosition {
  kCenter,
  kLeftTop,
  kRightBottom,
};
MAKE_ORDERED_ENUM_STRING_UTILS(LayoutDistributionPosition, "center", "left top",
                               "right bottom");

// Layout order parameter for Layout::SetLayoutOrder.
enum class LayoutOrder {
  kBottomToTop,  // From bottom to top element (default creation order).
  kTopToBottom,  // From top to bottom element.
};

// Specifies what happens when there is not enough room for the layout, even
// when all the children have been shrunk to their minimum size.
enum class LayoutOverflow {
  kClip,
  kScroll,
};
MAKE_ORDERED_ENUM_STRING_UTILS(LayoutOverflow, "clip", "scroll");

// Layout lays out its children along the given axis.
// Each elements size depend on its preferred size (See
// Element::GetPreferredSize), gravity, and the specified layout settings (See
// SetLayoutSize, SetLayoutPosition SetLayoutOverflow, SetLayoutDistribution,
// SetLayoutDistributionPosition), and the available size.
// Each element is also separated by the specified spacing (See SetSpacing).
class Layout : public Element {
 public:
  TBOBJECT_SUBCLASS(Layout, Element);
  static void RegisterInflater();

  // This means the spacing should be the default, read from the skin.
  static const int kSpacingFromSkin = util::kInvalidDimension;

  Layout(Axis axis = Axis::kX);

  // Sets along which axis the content should layout.
  void SetAxis(Axis axis) override;
  Axis GetAxis() const override { return m_axis; }

  // Sets the spacing between elements in this layout. Setting the default
  // (kSpacingFromSkin) will make it use the spacing specified in the skin.
  void SetSpacing(int spacing);
  int GetSpacing() const { return m_spacing; }

  // Sets the overflow scroll. If there is not enough room for all children in
  // this layout, it can scroll in the axis it's laid out. It does so
  // automatically by wheel or panning also for other LayoutOverflow than
  // LayoutOverflow::kScroll.
  void SetOverflowScroll(int overflow_scroll);
  int GetOverflowScroll() const { return m_overflow_scroll; }

  // Sets if a fadeout should be painter where the layout overflows or not.
  void SetPaintOverflowFadeout(bool paint_fadeout) {
    m_packed.paint_overflow_fadeout = paint_fadeout;
  }

  void SetLayoutSize(LayoutSize size);
  void SetLayoutPosition(LayoutPosition pos);
  void SetLayoutOverflow(LayoutOverflow overflow);
  void SetLayoutDistribution(LayoutDistribution distribution);
  void SetLayoutDistributionPosition(
      LayoutDistributionPosition distribution_pos);

  // Sets the layout order. The default is LayoutOrder::kBottomToTop, which
  // begins from bottom to top (default creation order).
  void SetLayoutOrder(LayoutOrder order);

  void InvalidateLayout(InvalidationMode il) override;

  PreferredSize OnCalculatePreferredContentSize(
      const SizeConstraints& constraints) override;

  void OnInflate(const resources::InflateInfo& info) override;
  bool OnEvent(const ElementEvent& ev) override;
  void OnPaintChildren(const PaintProps& paint_props) override;
  void OnProcess() override;
  void OnResized(int old_w, int old_h) override;
  void OnInflateChild(Element* child) override;
  void GetChildTranslation(int& x, int& y) const override;
  void ScrollTo(int x, int y) override;
  Element::ScrollInfo GetScrollInfo() override;

 protected:
  void ValidateLayout(const SizeConstraints& constraints,
                      PreferredSize* calculate_ps = nullptr);
  bool QualifyForExpansion(Gravity gravity) const;
  int GetWantedHeight(Gravity gravity, const PreferredSize& ps,
                      int available_height) const;
  Element* GetNextNonCollapsedElement(Element* child) const;
  int GetTrailingSpace(Element* child, int spacing) const;
  int CalculateSpacing();
  Element* GetFirstInLayoutOrder() const;
  Element* GetNextInLayoutOrder(Element* child) const;

  Axis m_axis = Axis::kX;
  int m_spacing = kSpacingFromSkin;
  int m_overflow = 0;
  int m_overflow_scroll = 0;
  union {
    struct {
      uint32_t layout_is_invalid : 1;
      uint32_t layout_mode_size : 4;
      uint32_t layout_mode_pos : 4;
      uint32_t layout_mode_overflow : 4;
      uint32_t layout_mode_dist : 4;
      uint32_t layout_mode_dist_pos : 4;
      uint32_t mode_reverse_order : 1;
      uint32_t paint_overflow_fadeout : 1;
    } m_packed;
    uint32_t m_packed_init = 0;
  };
};

}  // namespace tb

#endif  // TB_LAYOUT_H
