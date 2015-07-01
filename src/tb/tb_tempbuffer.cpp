/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_tempbuffer.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <memory>

namespace tb {

static char* p_realloc(char* buf, size_t size) {
  return (char*)realloc(buf, size);
}
static void p_free(char* buf) { free(buf); }

TBTempBuffer::TBTempBuffer() : m_data(0), m_data_size(0), m_append_pos(0) {}

TBTempBuffer::~TBTempBuffer() { p_free(m_data); }

void TBTempBuffer::SetAppendPos(size_t append_pos) {
  assert(append_pos >= 0 && append_pos <= m_data_size);
  m_append_pos = append_pos;
}

bool TBTempBuffer::Reserve(size_t size) {
  if (size > m_data_size) {
    char* new_data = p_realloc(m_data, size);
    if (!new_data) return false;
    m_data = new_data;
    m_data_size = size;
  }
  return true;
}

size_t TBTempBuffer::GetAppendReserveSize(size_t needed_size) const {
  // Reserve some extra memory to reduce the reserve calls.
  needed_size *= 2;
  return needed_size < 32 ? 32 : needed_size;
}

bool TBTempBuffer::Append(const char* data, size_t size) {
  if (m_append_pos + size > m_data_size &&
      !Reserve(GetAppendReserveSize(m_append_pos + size)))
    return false;
  memcpy(m_data + m_append_pos, data, size);
  m_append_pos += size;
  return true;
}

bool TBTempBuffer::AppendSpace(size_t size) {
  if (m_append_pos + size > m_data_size &&
      !Reserve(GetAppendReserveSize(m_append_pos + size)))
    return false;
  m_append_pos += size;
  return true;
}

bool TBTempBuffer::AppendString(const char* str) {
  // Add 1 to include the null termination in the data.
  if (Append(str, strlen(str) + 1)) {
    // Now remove the null termination from the append position
    // again, so another call will append to the same string (instead of
    // after the null termination of the first string)
    m_append_pos--;
    return true;
  }
  return false;
}

bool TBTempBuffer::AppendPath(const std::string& full_path_and_filename) {
  const char* path = full_path_and_filename.c_str();
  const char* str_start = path;
  while (const char* next = strpbrk(path, "\\/")) {
    path = next + 1;
  }

  if (str_start == path) {
    // Filename contained no path
    str_start = "./";
    path = str_start + 2;
  }

  size_t len = path - str_start;
  if (Reserve(len + 1)) {
    // Add the string, and nulltermination.
    Append(str_start, len);
    Append("", 1);
    // Remove null termination from append pos again (see details in
    // AppendString).
    m_append_pos--;
    return true;
  }
  return false;
}

}  // namespace tb
