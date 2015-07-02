/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_LAYOUT_H
#define TB_LAYOUT_H

#include "tb_widgets.h"

namespace tb {

/** This means the spacing should be the default, read from the skin. */
#define SPACING_FROM_SKIN kInvalidDimension

// Specifies which height widgets in a Axis::kX layout should have, or which
// width widgets in a Axis::kY layout should have.
// No matter what, it will still prioritize minimum and maximum for each widget.
enum class LayoutSize {
  kGravity,    // Sizes depend on the gravity for each widget. (If the widget
               // pulls towards both directions, it should grow to all available
               // space).
  kPreferred,  // Size will be the preferred so each widget may be sized
               // differently.
  kAvailable   // Size should grow to all available space.
};
MAKE_ORDERED_ENUM_STRING_UTILS(LayoutSize, "gravity", "preferred", "available");

// Specifies which y position widgets in a Axis::kX layout should have, or which
// x position widgets in a Axis::kY layout should have.
enum class LayoutPosition {
  kCenter,   // Position is centered.
  kLeftTop,  // Position is to the left for Axis::kY layout and top for Axis::kX
             // layout.
  kRightBottom,  // Position is to the right for Axis::kY layout and bottom for
                 // Axis::kX layout.
  kGravity,  // Position depend on the gravity for each widget. (If the widget
             // pulls towards both directions, it will be centered).
};
MAKE_ORDERED_ENUM_STRING_UTILS(LayoutPosition, "center", "left top",
                               "right bottom", "gravity");

// Specifies which width widgets in a Axis::kX layout should have, or which
// height widgets in a Axis::kY layout should have.
enum class LayoutDistribution {
  kPreferred,  // Size will be the preferred so each widget may be sized
               // differently.
  kAvailable,  // Size should grow to all available space.
  kGravity,    // Sizes depend on the gravity for each widget. (If the widget
               // pulls towards both directions, it should grow to all available
               // space).
};
MAKE_ORDERED_ENUM_STRING_UTILS(LayoutDistribution, "preferred", "available",
                               "gravity");

// Specifies how widgets should be moved horizontally in a Axis::kX layout (or
// vertically in a Axis::kY layout) if there is extra space available.
enum class LayoutDistributionPosition {
  kCenter,
  kLeftTop,
  kRightBottom,
};
MAKE_ORDERED_ENUM_STRING_UTILS(LayoutDistributionPosition, "center", "left top",
                               "right bottom");

// Layout order parameter for TBLayout::SetLayoutOrder.
enum class LayoutOrder {
  kBottomToTop,  // From bottom to top widget (default creation order).
  kTopToBottom,  // From top to bottom widget.
};

// Specifies what happens when there is not enough room for the layout, even
// when all the children have been shrunk to their minimum size.
enum class LayoutOverflow {
  kClip,
  kScroll,
};
MAKE_ORDERED_ENUM_STRING_UTILS(LayoutOverflow, "clip", "scroll");

/** TBLayout layouts its children along the given axis.

        Each widgets size depend on its preferred size (See
   TBWidget::GetPreferredSize),
        gravity, and the specified layout settings (See SetLayoutSize,
   SetLayoutPosition
        SetLayoutOverflow, SetLayoutDistribution,
   SetLayoutDistributionPosition), and
        the available size.

        Each widget is also separated by the specified spacing (See SetSpacing).
*/

class TBLayout : public TBWidget {
 public:
  TBOBJECT_SUBCLASS(TBLayout, TBWidget);

  TBLayout(Axis axis = Axis::kX);

  /** Set along which axis the content should be layouted */
  virtual void SetAxis(Axis axis);
  virtual Axis GetAxis() const { return m_axis; }

  /** Set the spacing between widgets in this layout. Setting the default
     (SPACING_FROM_SKIN)
          will make it use the spacing specified in the skin. */
  void SetSpacing(int spacing);
  int GetSpacing() const { return m_spacing; }

  /** Set the overflow scroll. If there is not enough room for all children in
     this layout,
          it can scroll in the axis it's laid out. It does so automatically by
     wheel or panning also
          for other LayoutOverflow than LayoutOverflow::kScroll. */
  void SetOverflowScroll(int overflow_scroll);
  int GetOverflowScroll() const { return m_overflow_scroll; }

  /** Set if a fadeout should be painter where the layout overflows or not. */
  void SetPaintOverflowFadeout(bool paint_fadeout) {
    m_packed.paint_overflow_fadeout = paint_fadeout;
  }

  /** Set the layout size mode. See LayoutSize. */
  void SetLayoutSize(LayoutSize size);

  /** Set the layout position mode. See LayoutPosition. */
  void SetLayoutPosition(LayoutPosition pos);

  /** Set the layout size mode. See LayoutOverflow. */
  void SetLayoutOverflow(LayoutOverflow overflow);

  /** Set the layout distribution mode. See LayoutDistribution. */
  void SetLayoutDistribution(LayoutDistribution distribution);

  /** Set the layout distribution position mode. See
   * LayoutDistributionPosition. */
  void SetLayoutDistributionPosition(
      LayoutDistributionPosition distribution_pos);

  /** Set the layout order. The default is LayoutOrder::kBottomToTop, which
     begins
          from bottom to top (default creation order). */
  void SetLayoutOrder(LayoutOrder order);

  virtual void InvalidateLayout(InvalidationMode il);

  virtual PreferredSize OnCalculatePreferredContentSize(
      const SizeConstraints& constraints);

  virtual void OnInflate(const INFLATE_INFO& info);
  virtual bool OnEvent(const TBWidgetEvent& ev);
  virtual void OnPaintChildren(const PaintProps& paint_props);
  virtual void OnProcess();
  virtual void OnResized(int old_w, int old_h);
  virtual void OnInflateChild(TBWidget* child);
  virtual void GetChildTranslation(int& x, int& y) const;
  virtual void ScrollTo(int x, int y);
  virtual TBWidget::ScrollInfo GetScrollInfo();

 protected:
  Axis m_axis;
  int m_spacing;
  int m_overflow;
  int m_overflow_scroll;
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
    uint32_t m_packed_init;
  };
  void ValidateLayout(const SizeConstraints& constraints,
                      PreferredSize* calculate_ps = nullptr);
  bool QualifyForExpansion(Gravity gravity) const;
  int GetWantedHeight(Gravity gravity, const PreferredSize& ps,
                      int available_height) const;
  TBWidget* GetNextNonCollapsedWidget(TBWidget* child) const;
  int GetTrailingSpace(TBWidget* child, int spacing) const;
  int CalculateSpacing();
  TBWidget* GetFirstInLayoutOrder() const;
  TBWidget* GetNextInLayoutOrder(TBWidget* child) const;
};
};

#endif  // TB_LAYOUT_H
