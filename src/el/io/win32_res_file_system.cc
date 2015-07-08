/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include <algorithm>
#include <codecvt>
#include <cstring>
#include <locale>

#include "el/io/win32_res_file_system.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define NOMINMAX
#include <SDKDDKVer.h>
#include <windows.h>

namespace el {
namespace io {

namespace {
class Win32ResFile : public File {
 public:
  Win32ResFile(std::string filename, const uint8_t* data, size_t length)
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

Win32ResFileSystem::Win32ResFileSystem(std::string prefix) : prefix_(prefix) {}

std::unique_ptr<File> Win32ResFileSystem::OpenRead(std::string filename) {
  std::string res_name = prefix_ + filename;
  std::transform(res_name.begin(), res_name.end(), res_name.begin(), ::toupper);
  for (size_t i = 0; i < res_name.size(); ++i) {
    char c = res_name[i];
    if (c == ' ' || c == '.' || c == '/' || c == '\\' || c == '@') {
      res_name[i] = '_';
    }
  }
  static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
  auto wide_res_name = converter.from_bytes(res_name);

  HRSRC res_info = ::FindResource(nullptr, wide_res_name.c_str(), RT_RCDATA);
  if (!res_info) {
    return nullptr;
  }

  HGLOBAL res = ::LoadResource(nullptr, res_info);
  if (!res) {
    return nullptr;
  }

  size_t data_length = ::SizeofResource(nullptr, res_info);
  void* data = ::LockResource(res);
  return std::make_unique<Win32ResFile>(
      filename, reinterpret_cast<const uint8_t*>(data), data_length);
}

}  // namespace io
}  // namespace el
