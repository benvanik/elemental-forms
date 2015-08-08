/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_IO_FILE_MANAGER_H_
#define EL_IO_FILE_MANAGER_H_

#include <memory>
#include <string>
#include <vector>

#include "el/config.h"
#include "el/io/file_system.h"

namespace el {
namespace io {

// Manages runtime-specified file systems to provide file IO.
//
// Hosts should register file systems at startup immediately after calling
// el::Initialize. Files are resolved by walking through the file systems in
// reverse of the order they were added, meaning that the last added file
// system will override the first.
class FileManager {
 public:
  static FileManager* get() { return &file_manager_; }

  static void RegisterFileSystem(std::unique_ptr<FileSystem> file_system);

  static std::unique_ptr<File> OpenRead(std::string filename);
  static std::unique_ptr<std::vector<uint8_t>> ReadContents(
      std::string filename);

  ~FileManager();

 private:
  FileManager();

  static FileManager file_manager_;

  std::vector<std::unique_ptr<FileSystem>> file_systems_;
};

}  // namespace io
}  // namespace el

#endif  // EL_IO_FILE_MANAGER_H_
