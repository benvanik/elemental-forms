/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_SCROLL_CONTAINER_H
#define TB_SCROLL_CONTAINER_H

#include "tb_widgets_common.h"

namespace tb {

enum class ScrollMode {
  kXY,          // X and Y always; scroll-mode: xy
  kY,           // Y always (X never); scroll-mode: y
  kAutoY,       // Y auto (X never); scroll-mode: y-auto
  kAutoXAutoY,  // X auto, Y auto; scroll-mode: auto
  kOff,         // X and Y never; scroll-mode: off
};
MAKE_ORDERED_ENUM_STRING_UTILS(ScrollMode, "xy", "y", "y-auto", "auto", "off");

// Internal for ScrollContainer.
class ScrollContainerRoot : public Element {
 private:  // May only be used by ScrollContainer.
  friend class ScrollContainer;
  ScrollContainerRoot() = default;

 public:
  void OnPaintChildren(const PaintProps& paint_props) override;
  void GetChildTranslation(int& x, int& y) const override;
};

// Helper for ScrollContainer or any other scrollable container that needs to
// solve scrollbar visibility according to ScrollMode.
class ScrollBarVisibility {
 public:
  ScrollBarVisibility() = default;

  static ScrollBarVisibility Solve(ScrollMode mode, int content_w,
                                   int content_h, int available_w,
                                   int available_h, int scrollbar_x_h,
                                   int scrollbar_y_w);
  static bool IsAlwaysOnX(ScrollMode mode) { return mode == ScrollMode::kXY; }
  static bool IsAlwaysOnY(ScrollMode mode) {
    return mode == ScrollMode::kXY || mode == ScrollMode::kY;
  }

 public:
  bool x_on = false;
  bool y_on = false;
  int visible_w = 0;
  int visible_h = 0;
};

// A container with scrollbars that can scroll its children.
class ScrollContainer : public Element {
  friend class ScrollContainerRoot;

 public:
  TBOBJECT_SUBCLASS(ScrollContainer, Element);

  ScrollContainer();
  ~ScrollContainer() override;

  // Sets to true if the preferred size of this container should adapt to the
  // preferred size of the content. This is disabled by default.
  void SetAdaptToContentSize(bool adapt);
  bool GetAdaptToContentSize() { return m_adapt_to_content_size; }

  // Sets to true if the content should adapt to the available size of this
  // container when it's larger than the preferred size.
  void SetAdaptContentSize(bool adapt);
  bool GetAdaptContentSize() { return m_adapt_content_size; }

  void SetScrollMode(ScrollMode mode);
  ScrollMode GetScrollMode() { return m_mode; }

  void ScrollTo(int x, int y) override;
  Element::ScrollInfo GetScrollInfo() override;
  Element* GetScrollRoot() override { return &m_root; }

  void InvalidateLayout(InvalidationMode il) override;

  Rect GetPaddingRect() override;
  PreferredSize OnCalculatePreferredContentSize(
      const SizeConstraints& constraints) override;

  void OnInflate(const InflateInfo& info) override;
  bool OnEvent(const ElementEvent& ev) override;
  void OnProcess() override;
  void OnResized(int old_w, int old_h) override;

  Element* GetContentRoot() override { return &m_root; }

 protected:
  void ValidateLayout(const SizeConstraints& constraints);

  ScrollBar m_scrollbar_x;
  ScrollBar m_scrollbar_y;
  ScrollContainerRoot m_root;
  bool m_adapt_to_content_size = false;
  bool m_adapt_content_size = false;
  bool m_layout_is_invalid = false;
  ScrollMode m_mode = ScrollMode::kXY;
};
};

#endif  // TB_SCROLL_CONTAINER_H
