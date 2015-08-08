/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <memory>

#include "el/util/string_builder.h"

namespace el {
namespace util {

StringBuilder::StringBuilder() = default;

StringBuilder::StringBuilder(size_t reserve_size) { Reserve(reserve_size); }

StringBuilder::~StringBuilder() { free(m_data); }

void StringBuilder::SetAppendPos(size_t append_pos) {
  assert(append_pos >= 0 && append_pos <= m_data_size);
  m_append_pos = append_pos;
}

void StringBuilder::Reserve(size_t size) {
  if (size > m_data_size) {
    char* new_data = (char*)realloc(m_data, size);
    assert(new_data);
    m_data = new_data;
    m_data_size = size;
  }
}

size_t StringBuilder::GetAppendReserveSize(size_t needed_size) const {
  // Reserve some extra memory to reduce the reserve calls.
  needed_size *= 2;
  return needed_size < 32 ? 32 : needed_size;
}

void StringBuilder::Append(const char* data, size_t size) {
  if (m_append_pos + size > m_data_size) {
    Reserve(GetAppendReserveSize(m_append_pos + size));
  }
  std::memcpy(m_data + m_append_pos, data, size);
  m_append_pos += size;
}

void StringBuilder::AppendSpace(size_t size) {
  if (m_append_pos + size > m_data_size) {
    Reserve(GetAppendReserveSize(m_append_pos + size));
  }
  m_append_pos += size;
}

void StringBuilder::AppendString(const char* str) {
  // Add 1 to include the null termination in the data.
  Append(str, strlen(str) + 1);

  // Now remove the null termination from the append position
  // again, so another call will append to the same string (instead of
  // after the null termination of the first string)
  m_append_pos--;
}

void StringBuilder::AppendPath(const std::string& full_path_and_filename) {
  const char* path = full_path_and_filename.c_str();
  const char* str_start = path;
  while (const char* next = strpbrk(path, "\\/")) {
    path = next + 1;
  }

  if (str_start == path) {
    // Filename contained no path.
    str_start = "./";
    path = str_start + 2;
  }

  size_t len = path - str_start;
  Reserve(len + 1);

  // Add the string, and nulltermination.
  Append(str_start, len);
  Append("", 1);

  // Remove null termination from append pos again (see details in
  // AppendString).
  m_append_pos--;
}

}  // namespace util
}  // namespace el
