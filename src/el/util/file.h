/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_UTIL_FILE_H_
#define EL_UTIL_FILE_H_

#include <memory>
#include <string>

#include "el/config.h"

namespace el {
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
}  // namespace el

#endif  // EL_UTIL_FILE_H_
