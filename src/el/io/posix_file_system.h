/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_IO_POSIX_FILE_SYSTEM_H_
#define EL_IO_POSIX_FILE_SYSTEM_H_

#include <memory>
#include <string>

#include "el/io/file_system.h"

namespace el {
namespace io {

class PosixFileSystem : public FileSystem {
 public:
  PosixFileSystem(std::string root_path);

  std::unique_ptr<File> OpenRead(std::string filename) override;

 private:
  std::string root_path_;
};

}  // namespace io
}  // namespace el

#endif  // EL_IO_POSIX_FILE_SYSTEM_H_
