/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_RECT_H_
#define EL_RECT_H_

#include <string>
#include <vector>

namespace el {

class Point {
 public:
  int x, y;

  Point() : x(0), y(0) {}
  Point(int x, int y) : x(x), y(y) {}
};
std::string to_string(const Point& value);

class Rect {
 public:
  int x, y, w, h;

  Rect() : x(0), y(0), w(0), h(0) {}
  Rect(int x, int y, int w, int h) : x(x), y(y), w(w), h(h) {}

  inline bool empty() const { return w <= 0 || h <= 0; }
  inline bool is_inside_out() const { return w < 0 || h < 0; }
  inline bool equals(const Rect& rect) const {
    return rect.x == x && rect.y == y && rect.w == w && rect.h == h;
  }
  bool intersects(const Rect& rect) const;
  bool contains(const Point& p) const {
    return p.x >= x && p.y >= y && p.x < x + w && p.y < y + h;
  }

  inline void reset() { x = y = w = h = 0; }
  inline void reset(int x, int y, int w, int h) {
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
std::string to_string(const Rect& value);

}  // namespace el

#endif  // EL_RECT_H_
