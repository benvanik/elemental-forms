/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_STR_H
#define TB_STR_H

#include <cstring>
#include <string>

#include "tb_types.h"

namespace tb {

/** TBStr is a simple string class.
        It's a compact wrapper for a char array, and doesn't do any storage
   magic to
        avoid buffer copying or remember its length. It is intended as "final
   storage"
        of strings since its buffer is compact.

        Serious work on strings is better done using TBTempBuffer and then set
   on a TBStr for
        final storage (since TBTempBuffer is optimized for speed rather than
   being compact).

        It is guaranteed to have a valid pointer at all times. If uninitialized,
   emptied or on
        out of memory, its storage will be a empty ("") const string.
*/

class TBStr {
 protected:
  char* s;

 public:
  ~TBStr();
  TBStr();
  TBStr(const TBStr& str);
  TBStr(const char* str);
  TBStr(const char* str, size_t len);

  const TBStr& operator=(const TBStr& str) {
    assign(str, std::string::npos);
    return *this;
  }

  inline size_t size() const { return strlen(s); }
  inline bool empty() const { return s[0] == 0; }

  inline char operator[](size_t n) const { return s[n]; }
  const char* c_str() const { return s; }
  const char* data() const { return s; }

  void assign(const char* str, size_t len);
  void assign(const TBStr& str, size_t len) { assign(str.c_str(), len); }

  void clear();

  void erase(size_t ofs, size_t len = std::string::npos);
  void insert(size_t ofs, const char* ins, size_t ins_len = std::string::npos);
  void insert(size_t ofs, const TBStr& ins,
              size_t ins_len = std::string::npos) {
    insert(ofs, ins.c_str(), ins_len);
  }
  void append(const char* ins, size_t ins_len = std::string::npos) {
    insert(strlen(s), ins, std::string::npos);
  }
  void append(const TBStr& ins, size_t ins_len = std::string::npos) {
    append(ins.c_str(), ins_len);
  }

  inline int compare(const char* str) const { return strcmp(s, str); }
  inline int compare(const TBStr& str) const { return strcmp(s, str.c_str()); }
};

const char* stristr(const char* arg1, const char* arg2);

TBStr format_string(const char* format, ...);

}  // namespace tb

#endif  // TB_STR_H
