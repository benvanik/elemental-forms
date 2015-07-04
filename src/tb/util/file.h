/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_UTIL_FILE_H_
#define TB_UTIL_FILE_H_

#include <memory>
#include <string>

#include "tb/config.h"

namespace tb {
namespace util {

// Porting interface for file access.
class File {
 public:
  enum class Mode {
    kRead,
  };

  virtual ~File() = default;

  static std::unique_ptr<File> Open(const std::string& filename, Mode mode);

  virtual size_t Size() = 0;
  virtual size_t Read(void* buf, size_t elemSize, size_t count) = 0;

 protected:
  File() = default;
};

}  // namespace util
}  // namespace tb

#endif  // TB_UTIL_FILE_H_
