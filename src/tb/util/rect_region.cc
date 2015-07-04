/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include <cassert>

#include "tb/util/rect_region.h"

namespace tb {
namespace util {

RectRegion::RectRegion() = default;

RectRegion::~RectRegion() = default;

void RectRegion::RemoveRect(size_t index) {
  assert(index >= 0 && index < rects_.size());
  rects_.erase(rects_.begin() + index);
}

void RectRegion::RemoveRectFast(size_t index) {
  assert(index >= 0 && index < rects_.size());
  if (index != rects_.size() - 1) {
    rects_[index] = rects_.back();
  }
  rects_.pop_back();
}

void RectRegion::Clear() { rects_.clear(); }

bool RectRegion::Set(const Rect& rect) {
  rects_.clear();
  return AddRect(rect, false);
}

bool RectRegion::AddRect(const Rect& rect, bool coalesce) {
  if (!rects_.empty() && coalesce) {
    // If the rect can coalesce with any existing rect,
    // just replace it with the union of both, doing coalesce
    // check again recursively.
    // Searching backwards is most likely to give a hit quicker
    // in many usage scenarios.
    for (intptr_t i = rects_.size() - 1; i >= 0; i--) {
      if (  // Can coalesce vertically
          (rect.x == rects_[i].x && rect.w == rects_[i].w &&
           (rect.y == rects_[i].y + rects_[i].h ||
            rect.y + rect.h == rects_[i].y)) ||  // Can coalesce horizontally
          (rect.y == rects_[i].y && rect.h == rects_[i].h &&
           (rect.x == rects_[i].x + rects_[i].w ||
            rect.x + rect.w == rects_[i].x))) {
        Rect union_rect = rects_[i].Union(rect);
        RemoveRectFast(i);
        return AddRect(union_rect, true);
      }
    }
  }

  rects_.push_back(std::move(rect));
  return true;
}

bool RectRegion::IncludeRect(const Rect& include_rect) {
  for (size_t i = 0; i < rects_.size(); i++) {
    if (include_rect.intersects(rects_[i])) {
      // Make a region containing the non intersecting parts and then include
      // those recursively (they might still intersect some other part of the
      // region).
      RectRegion inclusion_region;
      if (!inclusion_region.AddExcludingRects(include_rect, rects_[i], false)) {
        return false;
      }
      for (int j = 0; j < inclusion_region.rects_.size(); j++) {
        if (!IncludeRect(inclusion_region.rects_[j])) {
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
  size_t num_rects_to_check = rects_.size();
  for (size_t i = 0; i < num_rects_to_check; i++) {
    if (rects_[i].intersects(exclude_rect)) {
      // Remove the existing rectangle we found we intersect
      // and add the pieces we don't intersect. New rects
      // will be added at the end of the list, so we can decrease
      // num_rects_to_check.
      Rect rect = rects_[i];
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
  assert(rect.intersects(exclude_rect));
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

const Rect& RectRegion::operator[](size_t index) const {
  assert(index >= 0 && index < rects_.size());
  return rects_[index];
}

}  // namespace util
}  // namespace tb
