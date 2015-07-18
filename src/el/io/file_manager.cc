/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/io/file_manager.h"

namespace el {
namespace io {

FileManager FileManager::file_manager_;

FileManager::FileManager() = default;

FileManager::~FileManager() = default;

void FileManager::RegisterFileSystem(std::unique_ptr<FileSystem> file_system) {
  file_manager_.file_systems_.emplace_back(std::move(file_system));
}

std::unique_ptr<File> FileManager::OpenRead(std::string filename) {
  for (auto it = file_manager_.file_systems_.rbegin();
       it != file_manager_.file_systems_.rend(); ++it) {
    auto file = it->get()->OpenRead(filename);
    if (file) {
      return file;
    }
  }
  return nullptr;
}

std::unique_ptr<std::vector<uint8_t>> FileManager::ReadContents(
    std::string filename) {
  auto file = OpenRead(filename);
  if (!file) {
    return nullptr;
  }
  auto buffer = std::make_unique<std::vector<uint8_t>>(file->size());
  file->Read(buffer->data(), buffer->size());
  return buffer;
}

}  // namespace io
}  // namespace el
