/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_RESOURCES_TEXT_PARSER_STREAM_H_
#define TB_RESOURCES_TEXT_PARSER_STREAM_H_

#include <algorithm>
#include <string>

#include "tb/resources/text_parser.h"

namespace tb {
namespace util {
class File;
}  // namespace util
}  // namespace tb

namespace tb {
namespace resources {

class TextParserTarget;

class TextParserStream {
 public:
  virtual ~TextParserStream() = default;
  virtual size_t GetMoreData(char* buf, size_t buf_len) = 0;
};

class FileTextParserStream : public TextParserStream {
 public:
  bool Read(const std::string& filename, TextParserTarget* target);
  size_t GetMoreData(char* buf, size_t buf_len) override;

 private:
  std::unique_ptr<util::File> file_;
};

class DataTextParserStream : public TextParserStream {
 public:
  bool Read(const char* data, size_t data_length, TextParserTarget* target);
  size_t GetMoreData(char* buf, size_t buf_len) override;

 private:
  const char* m_data = nullptr;
  size_t m_data_len = 0;
};

}  // namespace resources
}  // namespace tb

#endif  // TB_RESOURCES_TEXT_PARSER_STREAM_H_
