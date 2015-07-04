/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_STRING_BUILDER_H
#define TB_STRING_BUILDER_H

#include <string>

namespace tb {

// Manages a buffer that will be deleted on destruction.
// The buffer size can grow by calling Reserve or Append, but it
// will never shrink during the lifetime of the object.
class StringBuilder {
 public:
  StringBuilder();
  explicit StringBuilder(size_t reserve_size);
  ~StringBuilder();

  // Makes sure the buffer has at least size bytes.
  void Reserve(size_t size);

  // Gets a pointer to the buffer data.
  char* GetData() const { return m_data; }

  // Returns the size of the buffer in bytes.
  size_t GetCapacity() const { return m_data_size; }

  // Appends data with size bytes at the end of the buffer and increase the
  // append position with the same amount.
  void Append(const char* data, size_t size);
  void Append(const std::string& data, size_t size) {
    Append(data.c_str(), size);
  }

  // Increases the append position with size bytes without writing any data.
  // This is useful if you want to write the data later and want to make sure
  // space is reserved.
  void AppendSpace(size_t size);

  // Appends a null terminated string (including the null termination) at the
  // end of the buffer. The append position will be increased with the length of
  // the text (excluding the null termination) so multiple calls will produce a
  // concatenated null terminated string.
  void AppendString(const char* str);
  void AppendString(const std::string& str) { AppendString(str.c_str()); }

  // Append a path without the ending filename.
  // The buffer will be null terminated and the append position will be
  // increased with the length of the path (excluding the null termination).
  void AppendPath(const std::string& full_path_and_filename);

  // Sets the position (in bytes) in the buffer where Append should write.
  void SetAppendPos(size_t append_pos);

  // Resets the append position to 0.
  void ResetAppendPos() { m_append_pos = 0; }

  // Returns the current append position in in bytes.
  size_t GetAppendPos() const { return m_append_pos; }

 private:
  size_t GetAppendReserveSize(size_t needed_size) const;

  char* m_data = nullptr;
  size_t m_data_size = 0;
  size_t m_append_pos = 0;
};

}  // namespace tb

#endif  // TB_STRING_BUILDER_H
