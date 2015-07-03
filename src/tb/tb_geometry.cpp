/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_geometry.h"

#include <algorithm>
#include <cassert>

namespace tb {

bool Rect::Intersects(const Rect& rect) const {
  if (IsEmpty() || rect.IsEmpty()) return false;
  if (x + w > rect.x && x < rect.x + rect.w && y + h > rect.y &&
      y < rect.y + rect.h)
    return true;
  return false;
}

Rect Rect::MoveIn(const Rect& bounding_rect) const {
  return Rect(
      ClampClipMax(x, bounding_rect.x, bounding_rect.x + bounding_rect.w - w),
      ClampClipMax(y, bounding_rect.y, bounding_rect.y + bounding_rect.h - h),
      w, h);
}

Rect Rect::CenterIn(const Rect& bounding_rect) const {
  return Rect((bounding_rect.w - w) / 2, (bounding_rect.h - h) / 2, w, h);
}

Rect Rect::Union(const Rect& rect) const {
  assert(!IsInsideOut());
  assert(!rect.IsInsideOut());

  if (IsEmpty()) return rect;
  if (rect.IsEmpty()) return *this;

  int minx = std::min(x, rect.x);
  int miny = std::min(y, rect.y);
  int maxx = x + w > rect.x + rect.w ? x + w : rect.x + rect.w;
  int maxy = y + h > rect.y + rect.h ? y + h : rect.y + rect.h;
  return Rect(minx, miny, maxx - minx, maxy - miny);
}

Rect Rect::Clip(const Rect& clip_rect) const {
  assert(!clip_rect.IsInsideOut());
  Rect tmp;
  if (!Intersects(clip_rect)) return tmp;
  tmp.x = std::max(x, clip_rect.x);
  tmp.y = std::max(y, clip_rect.y);
  tmp.w = std::min(x + w, clip_rect.x + clip_rect.w) - tmp.x;
  tmp.h = std::min(y + h, clip_rect.y + clip_rect.h) - tmp.y;
  return tmp;
}

RectRegion::RectRegion() = default;

RectRegion::~RectRegion() { RemoveAll(true); }

void RectRegion::RemoveRect(int index) {
  assert(index >= 0 && index < m_num_rects);
  for (int i = index; i < m_num_rects - 1; i++) m_rects[i] = m_rects[i + 1];
  m_num_rects--;
}

void RectRegion::RemoveRectFast(int index) {
  assert(index >= 0 && index < m_num_rects);
  m_rects[index] = m_rects[--m_num_rects];
}

void RectRegion::RemoveAll(bool free_memory) {
  m_num_rects = 0;
  if (free_memory) {
    delete[] m_rects;
    m_rects = nullptr;
    m_capacity = 0;
  }
}

bool RectRegion::Set(const Rect& rect) {
  RemoveAll();
  return AddRect(rect, false);
}

void RectRegion::GrowIfNeeded() {
  if (m_num_rects == m_capacity) {
    int new_m_capacity = Clamp(4, m_capacity * 2, 1024);
    Rect* new_rects = new Rect[new_m_capacity];
    std::memmove(new_rects, m_rects, sizeof(Rect) * m_capacity);
    delete[] m_rects;
    m_rects = new_rects;
    m_capacity = new_m_capacity;
  }
}

bool RectRegion::AddRect(const Rect& rect, bool coalesce) {
  if (coalesce) {
    // If the rect can coalesce with any existing rect,
    // just replace it with the union of both, doing coalesce
    // check again recursively.
    // Searching backwards is most likely to give a hit quicker
    // in many usage scenarios.
    for (int i = m_num_rects - 1; i >= 0; i--) {
      if (  // Can coalesce vertically
          (rect.x == m_rects[i].x && rect.w == m_rects[i].w &&
           (rect.y == m_rects[i].y + m_rects[i].h ||
            rect.y + rect.h == m_rects[i].y)) ||  // Can coalesce horizontally
          (rect.y == m_rects[i].y && rect.h == m_rects[i].h &&
           (rect.x == m_rects[i].x + m_rects[i].w ||
            rect.x + rect.w == m_rects[i].x))) {
        Rect union_rect = m_rects[i].Union(rect);
        RemoveRectFast(i);
        return AddRect(union_rect, true);
      }
    }
  }

  GrowIfNeeded();
  m_rects[m_num_rects++] = rect;
  return true;
}

bool RectRegion::IncludeRect(const Rect& include_rect) {
  for (int i = 0; i < m_num_rects; i++) {
    if (include_rect.Intersects(m_rects[i])) {
      // Make a region containing the non intersecting parts and then include
      // those recursively (they might still intersect some other part of the
      // region).
      RectRegion inclusion_region;
      if (!inclusion_region.AddExcludingRects(include_rect, m_rects[i],
                                              false)) {
        return false;
      }
      for (int j = 0; j < inclusion_region.m_num_rects; j++) {
        if (!IncludeRect(inclusion_region.m_rects[j])) {
          return false;
        }
      }
      return true;
    }
  }
  // Now we know that the rect can be added without overlap.
  // Add it with coalesce checking to keep the number of rects down.
  return AddRect(include_rect, true);
}

bool RectRegion::ExcludeRect(const Rect& exclude_rect) {
  int num_rects_to_check = m_num_rects;
  for (int i = 0; i < num_rects_to_check; i++) {
    if (m_rects[i].Intersects(exclude_rect)) {
      // Remove the existing rectangle we found we intersect
      // and add the pieces we don't intersect. New rects
      // will be added at the end of the list, so we can decrease
      // num_rects_to_check.
      Rect rect = m_rects[i];
      RemoveRect(i);
      num_rects_to_check--;
      i--;

      if (!AddExcludingRects(rect, exclude_rect, true)) {
        return false;
      }
    }
  }
  return true;
}

bool RectRegion::AddExcludingRects(const Rect& rect, const Rect& exclude_rect,
                                   bool coalesce) {
  assert(rect.Intersects(exclude_rect));
  Rect remove = exclude_rect.Clip(rect);

  if (remove.y > rect.y) {
    if (!AddRect(Rect(rect.x, rect.y, rect.w, remove.y - rect.y), coalesce)) {
      return false;
    }
  }
  if (remove.x > rect.x) {
    if (!AddRect(Rect(rect.x, remove.y, remove.x - rect.x, remove.h),
                 coalesce)) {
      return false;
    }
  }
  if (remove.x + remove.w < rect.x + rect.w) {
    if (!AddRect(Rect(remove.x + remove.w, remove.y,
                      rect.x + rect.w - (remove.x + remove.w), remove.h),
                 coalesce)) {
      return false;
    }
  }
  if (remove.y + remove.h < rect.y + rect.h) {
    if (!AddRect(Rect(rect.x, remove.y + remove.h, rect.w,
                      rect.y + rect.h - (remove.y + remove.h)),
                 coalesce)) {
      return false;
    }
  }
  return true;
}

const Rect& RectRegion::GetRect(int index) const {
  assert(index >= 0 && index < m_num_rects);
  return m_rects[index];
}

}  // namespace tb
