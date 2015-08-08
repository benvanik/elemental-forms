/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include <cstring>

#include "el/io/file_manager.h"
#include "el/parsing/text_parser_stream.h"

namespace el {
namespace parsing {

bool FileTextParserStream::Read(const std::string& filename,
                                TextParserTarget* target) {
  file_ = io::FileManager::OpenRead(filename);
  if (!file_) {
    return false;
  }
  TextParser p;
  auto status = p.Read(this, target);
  return status == TextParser::Status::kOk ? true : false;
}

size_t FileTextParserStream::GetMoreData(char* buf, size_t buf_len) {
  return file_->Read(buf, buf_len);
}

bool DataTextParserStream::Read(const char* data, size_t data_length,
                                TextParserTarget* target) {
  m_data = data;
  m_data_len =
      data_length == std::string::npos ? std::strlen(data) : data_length;
  TextParser p;
  auto status = p.Read(this, target);
  return status == TextParser::Status::kOk ? true : false;
}
size_t DataTextParserStream::GetMoreData(char* buf, size_t buf_len) {
  size_t consume = std::min(buf_len, m_data_len);
  std::memcpy(buf, m_data, consume);
  m_data += consume;
  m_data_len -= consume;
  return consume;
}

}  // namespace parsing
}  // namespace el
