/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include <unordered_map>

#include "el/id.h"
#include "el/util/debug.h"
#include "el/util/hash.h"

namespace el {

#ifdef EL_RUNTIME_DEBUG_INFO

#if CHECKED
// Hash table for checking if we get any collisions (same hash value for TBID's
// created from different strings).
std::unordered_map<uint32_t, std::unique_ptr<TBID>> all_observed_ids_;
#endif  // CHECKED

std::unique_ptr<TBID> TBID::UnsafeClone(TBID* source) {
  auto clone = std::make_unique<TBID>(0);
  clone->id_ = source->id_;
  clone->debug_string = source->debug_string;
  return clone;
}

void TBID::reset(uint32_t newid) {
  id_ = newid;
  debug_string.clear();

#ifdef CHECKED
  if (id_ && !all_observed_ids_.count(id_)) {
    all_observed_ids_.emplace(id_, UnsafeClone(this));
  }
#endif  // CHECKED
}

void TBID::reset(const TBID& newid) {
  id_ = newid;
  debug_string = newid.debug_string;

#ifdef CHECKED
  if (id_) {
    auto& other_id = all_observed_ids_.find(id_);
    if (other_id != all_observed_ids_.end()) {
      // If this happens, 2 different strings result in the same hash.
      // It might be a good idea to change one of them, but it might not matter.
      assert(other_id->second->debug_string.compare(debug_string) == 0);
    } else {
      all_observed_ids_.emplace(id_, UnsafeClone(this));
    }
  }
#endif  // CHECKED
}

void TBID::reset(const char* string) {
  id_ = util::hash(string);
  debug_string = string;

#ifdef CHECKED
  if (id_) {
    auto& other_id = all_observed_ids_.find(id_);
    if (other_id != all_observed_ids_.end()) {
      assert(other_id->second->debug_string.compare(debug_string) == 0);
    } else {
      all_observed_ids_.emplace(id_, UnsafeClone(this));
    }
  }
#endif  // CHECKED
}

#else

void TBID::reset(const char* string) { id_ = util::hash(string); }

#endif  // EL_RUNTIME_DEBUG_INFO

}  // namespace el
