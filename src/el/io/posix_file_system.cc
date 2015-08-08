/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/io/posix_file_system.h"

namespace el {
namespace io {

namespace {
class PosixFile : public File {
 public:
  PosixFile(FILE* file_handle_) : file_handle_(file_handle_) {}
  ~PosixFile() override { fclose(file_handle_); }

  size_t size() const override {
    long oldpos = ftell(file_handle_);
    fseek(file_handle_, 0, SEEK_END);
    long num_bytes = ftell(file_handle_);
    fseek(file_handle_, oldpos, SEEK_SET);
    return num_bytes;
  }

  size_t Read(void* buffer, size_t length) override {
    return fread(buffer, 1, length, file_handle_);
  }

 private:
  FILE* file_handle_;
};
}  // namespace

PosixFileSystem::PosixFileSystem(std::string root_path)
    : root_path_(std::move(root_path)) {
  auto last_char = root_path_[root_path_.size() - 1];
  if (last_char != '/' && last_char != '\\') {
    root_path_ += '/';
  }
}

std::unique_ptr<File> PosixFileSystem::OpenRead(std::string filename) {
  auto full_path = root_path_ + filename;
  FILE* file_handle = fopen(full_path.c_str(), "rb");
  if (!file_handle) {
    return nullptr;
  }
  return std::make_unique<PosixFile>(file_handle);
}

}  // namespace io
}  // namespace el
