/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_IO_MEMORY_FILE_SYSTEM_H_
#define EL_IO_MEMORY_FILE_SYSTEM_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "el/io/file_system.h"

namespace el {
namespace io {

class MemoryFileSystem : public FileSystem {
 public:
  MemoryFileSystem();

  void AddFile(std::string filename, const void* data, size_t length);

  std::unique_ptr<File> OpenRead(std::string filename) override;

 private:
  std::unordered_map<std::string, std::pair<const uint8_t*, size_t>>
      file_entries_;
};

}  // namespace io
}  // namespace el

#endif  // EL_IO_MEMORY_FILE_SYSTEM_H_
