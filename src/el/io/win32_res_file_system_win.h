/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_IO_WIN32_RES_FILE_SYSTEM_WIN_H_
#define EL_IO_WIN32_RES_FILE_SYSTEM_WIN_H_

#include <memory>
#include <string>

#include "el/io/file_system.h"

namespace el {
namespace io {

class Win32ResFileSystem : public FileSystem {
 public:
  Win32ResFileSystem(std::string prefix);

  std::unique_ptr<File> OpenRead(std::string filename) override;

 private:
  std::string prefix_;
};

}  // namespace io
}  // namespace el

#endif  // EL_IO_WIN32_RES_FILE_SYSTEM_WIN_H_
