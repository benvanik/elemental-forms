/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/util/file.h"

#include <cstdio>

namespace el {
namespace util {

class PosixFile : public File {
 public:
  PosixFile(FILE* f) : file_(f) {}
  ~PosixFile() override { fclose(file_); }

  size_t Size() override {
    long oldpos = ftell(file_);
    fseek(file_, 0, SEEK_END);
    long num_bytes = ftell(file_);
    fseek(file_, oldpos, SEEK_SET);
    return num_bytes;
  }

  size_t Read(void* buf, size_t elemSize, size_t count) override {
    return fread(buf, elemSize, count, file_);
  }

 private:
  FILE* file_;
};

// static
std::unique_ptr<File> File::Open(const std::string& filename, Mode mode) {
  FILE* f = nullptr;
  switch (mode) {
    case Mode::kRead:
      f = fopen(filename.c_str(), "rb");
      break;
    default:
      break;
  }
  if (!f) {
    return nullptr;
  }
  return std::make_unique<PosixFile>(f);
}

}  // namespace util
}  // namespace el
