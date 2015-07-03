/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_GEOMETRY_H
#define TB_GEOMETRY_H

#include <string>

#include "tb_core.h"
#include "tb_str.h"

namespace tb {

class Point {
 public:
  int x, y;
  Point() : x(0), y(0) {}
  Point(int x, int y) : x(x), y(y) {}
};

class Rect {
 public:
  int x, y, w, h;
  Rect() : x(0), y(0), w(0), h(0) {}
  Rect(int x, int y, int w, int h) : x(x), y(y), w(w), h(h) {}

  inline bool IsEmpty() const { return w <= 0 || h <= 0; }
  inline bool IsInsideOut() const { return w < 0 || h < 0; }
  inline bool Equals(const Rect& rect) const {
    return rect.x == x && rect.y == y && rect.w == w && rect.h == h;
  }
  bool Intersects(const Rect& rect) const;
  bool Contains(const Point& p) const {
    return p.x >= x && p.y >= y && p.x < x + w && p.y < y + h;
  }

  inline void Reset() { x = y = w = h = 0; }
  inline void Set(int x, int y, int w, int h) {
    this->x = x;
    this->y = y;
    this->w = w;
    this->h = h;
  }

  inline Rect Shrink(int left, int top, int right, int bottom) const {
    return Rect(x + left, y + top, w - left - right, h - top - bottom);
  }
  inline Rect Expand(int left, int top, int right, int bottom) const {
    return Shrink(-left, -top, -right, -bottom);
  }
  inline Rect Shrink(int tx, int ty) const {
    return Rect(x + tx, y + ty, w - tx * 2, h - ty * 2);
  }
  inline Rect Expand(int tx, int ty) const { return Shrink(-tx, -ty); }
  inline Rect Offset(int dx, int dy) const {
    return Rect(x + dx, y + dy, w, h);
  }

  // Returns a rect moved inside bounding_rect. If the rect doesn't fit inside
  // bounding_rect, it will be placed so the x and/or y matches bounding_rect.
  Rect MoveIn(const Rect& bounding_rect) const;

  // Returns a rect centered in bounding_rect.
  Rect CenterIn(const Rect& bounding_rect) const;

  Rect Union(const Rect& rect) const;
  Rect Clip(const Rect& clip_rect) const;
};
inline std::string to_string(const Rect& value) {
  return format_string("%d %d %d %d", value.x, value.y, value.w, value.h);
}

// Performs calculations on regions represented by a list of rectangles.
class RectRegion {
 public:
  RectRegion();
  ~RectRegion();

  // Removes the rect at the given index.
  void RemoveRect(int index);

  // Remove the rect at the given index.
  // This method will change the order of rectangles after index.
  void RemoveRectFast(int index);

  // Removes all rectangles so the region becomes empty.
  // If free_memory is false, the internal buffers will be reused if more
  // rectangles are added again under its life time.
  void RemoveAll(bool free_memory = true);

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

  bool IsEmpty() const { return m_num_rects == 0; }
  int GetNumRects() const { return m_num_rects; }
  const Rect& GetRect(int index) const;

 private:
  Rect* m_rects = nullptr;
  int m_num_rects = 0;
  int m_capacity = 0;
  void GrowIfNeeded();
};

};  // namespace tb

#endif  // TB_GEOMETRY_H
