/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_id.h"

#include "tb/util/debug.h"
#include "tb/util/hash.h"
#include "tb/util/hash_table.h"

namespace tb {

#ifdef TB_RUNTIME_DEBUG_INFO

// Hash table for checking if we get any collisions (same hash value for TBID's
// created from different strings).
static util::HashTableAutoDeleteOf<TBID> all_id_hash;
static bool is_adding = false;

void TBID::Set(uint32_t newid) {
  id_ = newid;
  debug_string.clear();
  if (!is_adding && tb_core_is_initialized()) {
    if (!all_id_hash.Get(id_)) {
      is_adding = true;
      all_id_hash.Add(id_, new TBID(*this));
      is_adding = false;
    }
  }
}

void TBID::Set(const TBID& newid) {
  id_ = newid;
  TB_IF_DEBUG(debug_string = newid.debug_string);
  if (!is_adding && tb_core_is_initialized()) {
    if (TBID* other_id = all_id_hash.Get(id_)) {
      // If this happens, 2 different strings result in the same hash.
      // It might be a good idea to change one of them, but it might not matter.
      assert(other_id->debug_string.compare(debug_string) == 0);
    } else {
      is_adding = true;
      all_id_hash.Add(id_, new TBID(*this));
      is_adding = false;
    }
  }
}

void TBID::Set(const char* string) {
  id_ = util::hash(string);
  TB_IF_DEBUG(debug_string = string);
  if (!is_adding && tb_core_is_initialized()) {
    if (TBID* other_id = all_id_hash.Get(id_)) {
      assert(other_id->debug_string.compare(debug_string) == 0);
    } else {
      is_adding = true;
      all_id_hash.Add(id_, new TBID(*this));
      is_adding = false;
    }
  }
}

#else

void TBID::Set(const char* string) { id_ = util::hash(string); }

#endif  // TB_RUNTIME_DEBUG_INFO

}  // namespace tb
