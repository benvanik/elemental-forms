/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_UTIL_RECT_REGION_H_
#define TB_UTIL_RECT_REGION_H_

#include <vector>

#include "tb/rect.h"

namespace tb {
namespace util {

// Performs calculations on regions represented by a list of rectangles.
class RectRegion {
 public:
  RectRegion();
  ~RectRegion();

  // Removes the rect at the given index.
  void RemoveRect(size_t index);

  // Remove the rect at the given index.
  // This method will change the order of rectangles after index.
  void RemoveRectFast(size_t index);

  // Removes all rectangles so the region becomes empty.
  void Clear();

  // Sets the region to the given rect.
  bool Set(const Rect& rect);

  // Adds the rect without doing any overlap check.
  // If coalesce is true, it will coalesce the rectangle with existing
  // rectangles if possible (until there's nothing more to coalesce it with).
  bool AddRect(const Rect& rect, bool coalesce);

  // Includes the rect in the region.
  // This will add only the parts that's not already in the region so the
  // result doesn't contain overlap parts. This assumes there's no overlap in
  // the region already!
  bool IncludeRect(const Rect& include_rect);

  // Excludes the rect from the region.
  bool ExcludeRect(const Rect& exclude_rect);

  // Adds the rectangles that's left of rect after excluding exclude_rect.
  bool AddExcludingRects(const Rect& rect, const Rect& exclude_rect,
                         bool coalesce);

  bool empty() const { return rects_.empty(); }
  size_t size() const { return rects_.size(); }
  const Rect& operator[](size_t index) const;

 private:
  std::vector<Rect> rects_;
};

}  // namespace util
}  // namespace tb

#endif  // TB_UTIL_RECT_REGION_H_
