/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_ID_H
#define TB_ID_H

#include <cstdint>

#include "tb_hash.h"
#include "tb_str.h"
#include "tb_types.h"

namespace tb {

/** TBID is a wrapper for a uint32 to be used as ID.
        The uint32 can be set directly to any uint32, or it can be
        set from a string which will be hashed into the uint32. */
class TBID {
 public:
  TBID(uint32_t id = 0) { Set(id); }
  TBID(const char* string) { Set(string); }
  TBID(const TBStr& string) { Set(string.c_str()); }
  TBID(const TBID& id) { Set(id); }

#ifdef TB_RUNTIME_DEBUG_INFO
  void Set(uint32_t newid);
  void Set(const TBID& newid);
  void Set(const char* string);
#else
  void Set(uint32_t newid) { id = newid; }
  void Set(const TBID& newid) { id = newid; }
  void Set(const char* string) { id = TBGetHash(string); }
#endif

  operator uint32_t() const { return id; }
  const TBID& operator=(const TBID& id) {
    Set(id);
    return *this;
  }

 private:
  uint32_t id;

 public:
/** This string is here to aid debugging (Only in debug builds!)
        It should not to be used in your code! */
#ifdef TB_RUNTIME_DEBUG_INFO
  friend class TBLanguage;
  TBStr debug_string;
#endif
};

}  // namespace tb

#endif  // TB_ID_H
