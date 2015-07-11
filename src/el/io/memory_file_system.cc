/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include <algorithm>

#include "el/io/memory_file_system.h"

namespace el {
namespace io {

namespace {
class MemoryFile : public File {
 public:
  MemoryFile(std::string filename, const uint8_t* data, size_t length)
      : filename_(filename), data_(data), length_(length) {}

  size_t size() const override { return length_; }

  size_t Read(void* buffer, size_t length) override {
    size_t to_read = std::min(length_ - position_, length);
    if (!to_read) {
      return 0;
    }
    std::memcpy(buffer, data_ + position_, to_read);
    position_ += to_read;
    return to_read;
  }

 private:
  std::string filename_;
  const uint8_t* data_;
  size_t length_;
  size_t position_ = 0;
};
}  // namespace

MemoryFileSystem::MemoryFileSystem() = default;

void MemoryFileSystem::AddFile(std::string filename, const void* data,
                               size_t length) {
  file_entries_[filename] = {reinterpret_cast<const uint8_t*>(data), length};
}

std::unique_ptr<File> MemoryFileSystem::OpenRead(std::string filename) {
  auto it = file_entries_.find(filename);
  if (it == file_entries_.end()) {
    return nullptr;
  }
  return std::make_unique<MemoryFile>(it->first, it->second.first,
                                      it->second.second);
}

}  // namespace io
}  // namespace el
