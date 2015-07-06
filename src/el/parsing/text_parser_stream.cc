/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/parsing/text_parser_stream.h"
#include "el/util/file.h"

namespace el {
namespace parsing {

bool FileTextParserStream::Read(const std::string& filename,
                                TextParserTarget* target) {
  file_ = util::File::Open(filename, util::File::Mode::kRead);
  if (!file_) {
    return false;
  }
  TextParser p;
  auto status = p.Read(this, target);
  return status == TextParser::Status::kOk ? true : false;
}

size_t FileTextParserStream::GetMoreData(char* buf, size_t buf_len) {
  return file_->Read(buf, 1, buf_len);
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
  memcpy(buf, m_data, consume);
  m_data += consume;
  m_data_len -= consume;
  return consume;
}

}  // namespace parsing
}  // namespace el
