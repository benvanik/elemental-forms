/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_RESOURCES_TEXT_PARSER_H_
#define TB_RESOURCES_TEXT_PARSER_H_

#include <string>

#include "tb/util/string_builder.h"
#include "tb_value.h"

namespace tb {
namespace resources {

class TextParserStream;

class TextParserTarget {
 public:
  virtual ~TextParserTarget() = default;
  virtual void OnError(int line_nr, const std::string& error) = 0;
  virtual void OnComment(int line_nr, const char* comment) = 0;
  virtual void OnToken(int line_nr, const char* name, Value& value) = 0;
  virtual void Enter() = 0;
  virtual void Leave() = 0;
};

class TextParser {
 public:
  enum class Status {
    kOk,
    kOutOfMemory,
    kParseError,
  };
  TextParser() = default;
  Status Read(TextParserStream* stream, TextParserTarget* target);

 private:
  void OnLine(char* line, TextParserTarget* target);
  void OnCompactLine(char* line, TextParserTarget* target);
  void OnMultiline(char* line, TextParserTarget* target);
  void ConsumeValue(Value& dst_value, char*& line);

  int current_indent = 0;
  int current_line_nr = 0;
  std::string multi_line_token;
  StringBuilder multi_line_value;
  int multi_line_sub_level = 0;
  bool pending_multiline = false;
};

}  // namespace resources
}  // namespace tb

#endif  // TB_RESOURCES_TEXT_PARSER_H_
