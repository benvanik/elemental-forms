/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb_parser.h"

#include <cassert>
#include <cctype>

#include "tb/util/string.h"
#include "tb/util/utf8.h"

namespace tb {

bool is_hex(char c) {
  return ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
          (c >= 'A' && c <= 'F'));
}

uint32_t parse_hex(char*& src, int max_count) {
  uint32_t hex = 0;
  for (int i = 0; i < max_count; i++) {
    char c = *src;
    if (!is_hex(c)) break;
    hex <<= 4;
    hex |= isdigit(c) ? c - '0' : tolower(c) - 'a' + 10;
    src++;
  }
  return hex;
}

void UnescapeString(char* str) {
  // Fast forward to any escape sequence.
  while (*str && *str != '\\') str++;

  char* dst = str, * src = str;
  while (*src) {
    if (*src == '\\') {
      bool code_found = true;
      switch (src[1]) {
        case 'a':
          *dst = '\a';
          break;
        case 'b':
          *dst = '\b';
          break;
        case 'f':
          *dst = '\f';
          break;
        case 'n':
          *dst = '\n';
          break;
        case 'r':
          *dst = '\r';
          break;
        case 't':
          *dst = '\t';
          break;
        case 'v':
          *dst = '\v';
          break;
        case '0':
          *dst = '\0';
          break;
        case '\"':
          *dst = '\"';
          break;
        case '\'':
          *dst = '\'';
          break;
        case '\\':
          *dst = '\\';
          break;
        case 'x':  // \xXX
        case 'u':  // \uXXXX
        {
          // This should be safe. A utf-8 character can be at most 4 bytes,
          // and we have 4 bytes to use for \xXX and 6 for \uXXXX.
          src += 2;
          if (auto hex = parse_hex(src, src[1] == 'x' ? 2 : 4)) {
            dst += util::utf8::encode(hex, dst);
          }
          continue;
        }
        default:
          code_found = false;
      }
      if (code_found) {
        src += 2;
        dst++;
        continue;
      }
    }
    *dst = *src;
    dst++;
    src++;
  }
  *dst = 0;
}

bool is_white_space(const char* str) {
  switch (*str) {
    case ' ':
    case '\t':
      return true;
    default:
      return false;
  }
}

// Returns true if the given string starts with a color.
// Ex: #ffdd00, #fd0
bool is_start_of_color(const char* str) {
  if (*str++ != '#') return false;
  int digit_count = 0;
  while (is_hex(*str)) {
    str++;
    digit_count++;
  }
  return digit_count == 8 || digit_count == 6 || digit_count == 4 ||
         digit_count == 3;
}

// Returns true if the given string may be a node reference, such as language
// strings or NodeRefTree references.
bool is_start_of_reference(const char* str) {
  if (*str++ != '@') {
    return false;
  }
  while (*str && *str != ' ') {
    // If the token ends with colon, it's not a value but a key.
    if (*str == ':') {
      return false;
    }
    str++;
  }
  return true;
}

// Checks if the line is a comment or empty space. If it is, consume the leading
// whitespace from line.
bool is_space_or_comment(char*& line) {
  char* tmp = line;
  while (is_white_space(tmp)) {
    tmp++;
  }
  if (*tmp == '#' || *tmp == 0) {
    line = tmp;
    return true;
  }
  return false;
}

bool is_pending_multiline(const char* str) {
  while (is_white_space(str)) {
    str++;
  }
  return str[0] == '\\' && str[1] == 0;
}

bool IsEndQuote(const char* buf_start, const char* buf, const char quote_type) {
  if (*buf != quote_type) {
    return false;
  }
  int num_backslashes = 0;
  while (buf_start < buf && *(buf-- - 1) == '\\') {
    num_backslashes++;
  }
  return !(num_backslashes & 1);
}

Parser::Status Parser::Read(ParserStream* stream, ParserTarget* target) {
  StringBuilder line(1024);
  StringBuilder work(1024);

  current_indent = 0;
  current_line_nr = 1;
  pending_multiline = false;
  multi_line_sub_level = 0;

  while (size_t read_len =
             stream->GetMoreData((char*)work.GetData(), work.GetCapacity())) {
    char* buf = work.GetData();

    // Skip BOM (BYTE ORDER MARK) character, often in the beginning of UTF-8
    // documents.
    if (current_line_nr == 1 && read_len > 3 && (uint8_t)buf[0] == 239 &&
        (uint8_t)buf[1] == 187 && (uint8_t)buf[2] == 191) {
      read_len -= 3;
      buf += 3;
    }

    size_t line_pos = 0;
    while (true) {
      // Find line end.
      size_t line_start = line_pos;
      while (line_pos < read_len && buf[line_pos] != '\n') {
        line_pos++;
      }

      if (line_pos < read_len) {
        // We have a line.
        // Skip preceding \r (if we have one).
        size_t line_len = line_pos - line_start;
        line.Append(buf + line_start, line_len);

        // Strip away trailing '\r' if the line has it.
        char* linebuf = line.GetData();
        size_t linebuf_len = line.GetAppendPos();
        if (linebuf_len > 0 && linebuf[linebuf_len - 1] == '\r') {
          linebuf[linebuf_len - 1] = 0;
        }

        // Terminate the line string.
        line.Append("", 1);

        // Handle line.
        OnLine(line.GetData(), target);
        current_line_nr++;

        line.ResetAppendPos();
        line_pos++;  // Skip this \n
        // Find next line.
        continue;
      }
      // No more lines here so push the rest and break for more data.
      line.Append(buf + line_start, read_len - line_start);
      break;
    }
  }
  if (line.GetAppendPos()) {
    line.Append("", 1);
    OnLine(line.GetData(), target);
    current_line_nr++;
  }
  return Status::kOk;
}

void Parser::OnLine(char* line, ParserTarget* target) {
  if (is_space_or_comment(line)) {
    if (*line == '#') {
      target->OnComment(current_line_nr, line + 1);
    }
    return;
  }
  if (pending_multiline) {
    OnMultiline(line, target);
    return;
  }

  // Check indent.
  int indent = 0;
  while (line[indent] == '\t' && line[indent] != 0) {
    indent++;
  }
  line += indent;

  if (indent - current_indent > 1) {
    target->OnError(current_line_nr, "Indentation error. (Line skipped)");
    return;
  }

  if (indent > current_indent) {
    // FIX: Report indentation error if more than 1 higher!
    assert(indent - current_indent == 1);
    target->Enter();
    current_indent++;
  } else if (indent < current_indent) {
    while (indent < current_indent) {
      target->Leave();
      current_indent--;
    }
  }

  if (line[0] == 0) {
    return;
  } else {
    char* token = line;
    // Read line while consuming it and copy over to token buf.
    while (!is_white_space(line) && line[0] != 0) {
      line++;
    }
    size_t token_len = line - token;
    // Consume any white space after the token.
    while (is_white_space(line)) {
      line++;
    }

    bool is_compact_line = token_len && token[token_len - 1] == ':';

    Value value;
    if (is_compact_line) {
      token_len--;
      token[token_len] = 0;

      // Check if the first argument is not a child but the value for this
      // token.
      if (*line == '[' || *line == '\"' || *line == '\'' ||
          util::is_start_of_number(line) || is_start_of_color(line) ||
          is_start_of_reference(line)) {
        ConsumeValue(value, line);

        if (pending_multiline) {
          // The value wrapped to the next line, so we should remember the token
          // and continue.
          multi_line_token = token;
          return;
        }
      }
    } else if (token[token_len]) {
      token[token_len] = 0;
      UnescapeString(line);
      value.SetFromStringAuto(line, Value::Set::kAsStatic);
    }
    target->OnToken(current_line_nr, token, value);

    if (is_compact_line) {
      OnCompactLine(line, target);
    }
  }
}

void Parser::OnCompactLine(char* line, ParserTarget* target) {
  target->Enter();
  while (*line) {
    // Consume any whitespace.
    while (is_white_space(line)) {
      line++;
    }

    // Find token.
    char* token = line;
    while (*line != ':' && *line != 0) {
      line++;
    }
    if (!*line) {
      // Syntax error, expected token.
      break;
    }
    *line++ = 0;

    // Consume any whitespace.
    while (is_white_space(line)) {
      line++;
    }

    Value v;
    ConsumeValue(v, line);

    if (pending_multiline) {
      // The value wrapped to the next line, so we should remember the token and
      // continue.
      multi_line_token = token;
      // Since we need to call target->Leave when the multiline is ready, set
      // multi_line_sub_level.
      multi_line_sub_level = 1;
      return;
    }

    // Ready.
    target->OnToken(current_line_nr, token, v);
  }

  target->Leave();
}

void Parser::OnMultiline(char* line, ParserTarget* target) {
  // Consume any whitespace.
  while (is_white_space(line)) {
    line++;
  }

  Value value;
  ConsumeValue(value, line);

  if (!pending_multiline) {
    // Ready with all lines.
    value.SetString(multi_line_value.GetData(), Value::Set::kAsStatic);
    target->OnToken(current_line_nr, multi_line_token.c_str(), value);

    if (multi_line_sub_level) {
      target->Leave();
    }

    // Reset.
    multi_line_value.SetAppendPos(0);
    multi_line_sub_level = 0;
  }
}

void Parser::ConsumeValue(Value& dst_value, char*& line) {
  // Find value (As quoted string, or as auto).
  char* value = line;
  if (*line == '\"' || *line == '\'') {
    const char quote_type = *line;
    // Consume starting quote.
    line++;
    value++;
    // Find ending quote or end.
    while (!IsEndQuote(value, line, quote_type) && *line != 0) {
      line++;
    }
    // Terminate away the quote.
    if (*line == quote_type) {
      *line++ = 0;
    }

    // Consume any whitespace.
    while (is_white_space(line)) {
      line++;
    }
    // Consume any comma.
    if (*line == ',') {
      line++;
    }

    UnescapeString(value);
    dst_value.SetString(value, Value::Set::kAsStatic);
  } else {
    // Find next comma or end.
    while (*line != ',' && *line != 0) {
      line++;
    }
    // Terminate away the comma.
    if (*line == ',') {
      *line++ = 0;
    }

    UnescapeString(value);
    dst_value.SetFromStringAuto(value, Value::Set::kAsStatic);
  }

  // Check if we still have pending value data on the following line and set
  // pending_multiline.
  bool continuing_multiline = pending_multiline;
  pending_multiline = is_pending_multiline(line);

  // Append the multi line value to the buffer.
  if (continuing_multiline || pending_multiline) {
    multi_line_value.AppendString(dst_value.GetString());
  }
}

}  // namespace tb
