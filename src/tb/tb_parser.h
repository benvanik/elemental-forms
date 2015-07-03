/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_PARSER_H
#define TB_PARSER_H

#include "tb_str.h"
#include "tb_string_builder.h"
#include "tb_value.h"

namespace tb {

// Unescapes backslash codes.
// This is done in place using the string both as source and destination.
void UnescapeString(char* str);

// Checks if buf is pointing at a end quote.
// It may need to iterate buf backwards toward buf_start to check if any
// preceding backslashes make it a escaped quote (which should not be the end
// quote).
bool IsEndQuote(const char* buf_start, const char* buf, const char quote_type);

class ParserTarget {
 public:
  virtual ~ParserTarget() = default;
  virtual void OnError(int line_nr, const std::string& error) = 0;
  virtual void OnComment(int line_nr, const char* comment) = 0;
  virtual void OnToken(int line_nr, const char* name, Value& value) = 0;
  virtual void Enter() = 0;
  virtual void Leave() = 0;
};

class ParserStream {
 public:
  virtual ~ParserStream() = default;
  virtual size_t GetMoreData(char* buf, size_t buf_len) = 0;
};

class Parser {
 public:
  enum class Status {
    kOk,
    kOutOfMemory,
    kParseError,
  };
  Parser() = default;
  Status Read(ParserStream* stream, ParserTarget* target);

 private:
  void OnLine(char* line, ParserTarget* target);
  void OnCompactLine(char* line, ParserTarget* target);
  void OnMultiline(char* line, ParserTarget* target);
  void ConsumeValue(Value& dst_value, char*& line);

  int current_indent = 0;
  int current_line_nr = 0;
  std::string multi_line_token;
  StringBuilder multi_line_value;
  int multi_line_sub_level = 0;
  bool pending_multiline = false;
};

}  // namespace tb

#endif  // TB_PARSER_H
