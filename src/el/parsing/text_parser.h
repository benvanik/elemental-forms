/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_PARSING_TEXT_PARSER_H_
#define EL_PARSING_TEXT_PARSER_H_

#include <string>

#include "el/util/string_builder.h"
#include "el/value.h"

namespace el {
namespace parsing {

class TextParserStream;

class TextParserTarget {
 public:
  virtual ~TextParserTarget() = default;
  virtual void OnError(int line_nr, const std::string& error) = 0;
  virtual void OnComment(int line_nr, const char* comment) = 0;
  virtual void OnToken(int line_nr, const char* name, Value* value) = 0;
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
  void ConsumeValue(Value* dst_value, char** line);

  int current_indent = 0;
  int current_line_nr = 0;
  std::string multi_line_token;
  util::StringBuilder multi_line_value;
  int multi_line_sub_level = 0;
  bool pending_multiline = false;
};

}  // namespace parsing
}  // namespace el

#endif  // EL_PARSING_TEXT_PARSER_H_
