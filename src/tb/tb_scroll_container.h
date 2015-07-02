/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
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
  kOff          // X and Y never; scroll-mode: off
};
MAKE_ORDERED_ENUM_STRING_UTILS(ScrollMode, "xy", "y", "y-auto", "auto", "off");

/** TBScrollContainerRoot - Internal for TBScrollContainer */
class TBScrollContainerRoot : public TBWidget {
 private:  // May only be used by TBScrollContainer.
  friend class TBScrollContainer;
  TBScrollContainerRoot() {}

 public:
  virtual void OnPaintChildren(const PaintProps& paint_props);
  virtual void GetChildTranslation(int& x, int& y) const;
};

/** TBScrollBarVisibility - Helper for TBScrollContainer or any other scrollable
        container that needs to solve scrollbar visibility according to
   ScrollMode. */
class TBScrollBarVisibility {
 public:
  TBScrollBarVisibility()
      : x_on(false), y_on(false), visible_w(0), visible_h(0) {}

  static TBScrollBarVisibility Solve(ScrollMode mode, int content_w,
                                     int content_h, int available_w,
                                     int available_h, int scrollbar_x_h,
                                     int scrollbar_y_w);
  static bool IsAlwaysOnX(ScrollMode mode) { return mode == ScrollMode::kXY; }
  static bool IsAlwaysOnY(ScrollMode mode) {
    return mode == ScrollMode::kXY || mode == ScrollMode::kY;
  }

 public:
  bool x_on, y_on;
  int visible_w, visible_h;
};

/** TBScrollContainer - A container with scrollbars that can scroll its
 * children. */

class TBScrollContainer : public TBWidget {
  friend class TBScrollContainerRoot;

 public:
  TBOBJECT_SUBCLASS(TBScrollContainer, TBWidget);

  TBScrollContainer();
  ~TBScrollContainer();

  /** Set to true if the preferred size of this container should adapt to the
     preferred
          size of the content. This is disabled by default. */
  void SetAdaptToContentSize(bool adapt);
  bool GetAdaptToContentSize() { return m_adapt_to_content_size; }

  /** Set to true if the content should adapt to the available size of this
     container
          when it's larger than the preferred size. */
  void SetAdaptContentSize(bool adapt);
  bool GetAdaptContentSize() { return m_adapt_content_size; }

  void SetScrollMode(ScrollMode mode);
  ScrollMode GetScrollMode() { return m_mode; }

  virtual void ScrollTo(int x, int y);
  virtual TBWidget::ScrollInfo GetScrollInfo();
  virtual TBWidget* GetScrollRoot() { return &m_root; }

  virtual void InvalidateLayout(InvalidationMode il);

  virtual TBRect GetPaddingRect();
  virtual PreferredSize OnCalculatePreferredContentSize(
      const SizeConstraints& constraints);

  virtual void OnInflate(const INFLATE_INFO& info);
  virtual bool OnEvent(const TBWidgetEvent& ev);
  virtual void OnProcess();
  virtual void OnResized(int old_w, int old_h);

  virtual TBWidget* GetContentRoot() { return &m_root; }

 protected:
  TBScrollBar m_scrollbar_x;
  TBScrollBar m_scrollbar_y;
  TBScrollContainerRoot m_root;
  bool m_adapt_to_content_size;
  bool m_adapt_content_size;
  bool m_layout_is_invalid;
  ScrollMode m_mode;
  void ValidateLayout(const SizeConstraints& constraints);
};
};

#endif  // TB_SCROLL_CONTAINER_H
