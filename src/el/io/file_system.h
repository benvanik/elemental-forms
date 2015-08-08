/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_IO_FILE_SYSTEM_H_
#define EL_IO_FILE_SYSTEM_H_

#include <memory>
#include <string>

#include "el/config.h"

namespace el {
namespace io {

class File {
 public:
  virtual ~File() = default;

  virtual size_t size() const = 0;
  virtual size_t Read(void* buffer, size_t length) = 0;

 protected:
  File() = default;
};

class FileSystem {
 public:
  virtual std::unique_ptr<File> OpenRead(std::string filename) = 0;
};

}  // namespace io
}  // namespace el

#endif  // EL_IO_FILE_SYSTEM_H_
