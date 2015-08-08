/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_PARSING_TEXT_PARSER_STREAM_H_
#define EL_PARSING_TEXT_PARSER_STREAM_H_

#include <algorithm>
#include <memory>
#include <string>

#include "el/io/file_system.h"
#include "el/parsing/text_parser.h"

namespace el {
namespace parsing {

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
  std::unique_ptr<io::File> file_;
};

class DataTextParserStream : public TextParserStream {
 public:
  bool Read(const char* data, size_t data_length, TextParserTarget* target);
  size_t GetMoreData(char* buf, size_t buf_len) override;

 private:
  const char* m_data = nullptr;
  size_t m_data_len = 0;
};

}  // namespace parsing
}  // namespace el

#endif  // EL_PARSING_TEXT_PARSER_STREAM_H_
