/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_ID_H_
#define EL_ID_H_

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>

#include "el/config.h"
#include "el/util/hash.h"

namespace el {

// TBID is a wrapper for a uint32_t to be used as ID.
// The uint32_t can be set directly to any uint32_t, or it can be set from a
// string which will be hashed into the uint32_t.
class TBID {
 public:
  TBID(uint32_t id = 0) { reset(id); }
  TBID(const char* string) { reset(string); }
  TBID(const std::string& string) { reset(string.c_str()); }
  TBID(const TBID& id) { reset(id); }

#ifdef EL_RUNTIME_DEBUG_INFO
  void reset(uint32_t newid);
  void reset(const TBID& newid);
  void reset(const char* string);
#else
  void reset(uint32_t newid) { id_ = newid; }
  void reset(const TBID& newid) { id_ = newid; }
  void reset(const char* string);
#endif  // EL_RUNTIME_DEBUG_INFO

  operator uint32_t() const { return id_; }
  const TBID& operator=(const TBID& id) {
    reset(id);
    return *this;
  }

 private:
  uint32_t id_;

  std::unique_ptr<TBID> UnsafeClone(TBID* source);

 public:
#ifdef EL_RUNTIME_DEBUG_INFO
  // This string is here to aid debugging (Only in debug builds!)
  // It should not to be used in your code!
  std::string debug_string;
#endif  // EL_RUNTIME_DEBUG_INFO
};
#ifndef EL_RUNTIME_DEBUG_INFO
static_assert(sizeof(TBID) == sizeof(uint32_t), "Treated as uint32_t");
#endif  // !EL_RUNTIME_DEBUG_INFO

}  // namespace el

#endif  // EL_ID_H_
