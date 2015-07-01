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

#include "tb_types.h"

namespace tb {

/** Simple string class that doesn't own or change the string pointer. */

class TBStrC {
 protected:
  char* s;

 public:
  TBStrC(const char* str) : s(const_cast<char*>(str)) {}

  inline size_t size() const { return strlen(s); }
  inline bool empty() const { return s[0] == 0; }

  inline int compare(const char* str) const { return strcmp(s, str); }

  inline char operator[](size_t n) const { return s[n]; }
  inline operator const char*() const { return s; }
};

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

class TBStr : public TBStrC {
 public:
  ~TBStr();
  TBStr();
  TBStr(const TBStr& str);
  TBStr(const char* str);
  TBStr(const char* str, size_t len);

  void assign(const char* str, size_t len);
  bool SetFormatted(const char* format, ...);

  void clear();

  void erase(size_t ofs, size_t len);
  void insert(size_t ofs, const char* ins, size_t ins_len = std::string::npos);
  void append(const char* ins, size_t ins_len = std::string::npos) {
    insert(strlen(s), ins, std::string::npos);
  }

  char* c_str() const { return s; }
  const TBStr& operator=(const TBStr& str) {
    assign(str, std::string::npos);
    return *this;
  }
};

}  // namespace tb

#endif  // TB_STR_H
