/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * �2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions �2011-2015 Emil Seger�s: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_TEXT_UTF8_H_
#define EL_TEXT_UTF8_H_

#include <cstdint>

namespace el {
namespace text {
namespace utf8 {

typedef uint32_t UCS4;

/** Decodes UTF-8 from a string input to a UCS4 character.
        @param src buffer in UTF-8 that should be decoded. If the buffer
   represent
               a valid character, the pointer will be incremented to the next
   character.
        @param src_end The end of the string.
        @return a UCS4 character, or 0xFFFF if the buffer didn't represent a
   valid character.
*/
UCS4 decode(const char*& src, const char* src_end);

/** Encodes a UCS4 character to UTF-8.
        @param ch UCS-4 value.
        @param dst buffer to receive UTF-8 encoding (must be at least 8 bytes!)
        @return number of bytes needed to represent character
*/
int encode(UCS4 ch, char* dst);

/** Decode the next UCS4 character from a UTF-8 string, and update the index
   variable.
        @param str The UTF-8 string.
        @param i The index of the current position. This will be increased to
   the next position.
        @param i_max The last position (size of str).
*/
UCS4 decode_next(const char* str, size_t* i, size_t i_max);

/** Move to the next character in a UTF-8 string.
        @param str The UTF-8 string.
        @param i The index of the current position. This will be increased to
   the next position.
        @param i_max The last position (size of str).
*/
void move_inc(const char* str, size_t* i, size_t i_max);

/** Move to the previous character in a UTF-8 string.
        @param str The UTF-8 string.
        @param i The index of the current position. This will be decreased to
   the previous position.
*/
void move_dec(const char* str, size_t* i);

/** Count characters before null termination in a UTF-8 string.
        ParseNode: Does not include the null termination!
        @param str The UTF-8 string.
        @param i_max The last position (size of str).
*/
size_t count_characters(const char* str, size_t i_max);

}  // namespace utf8
}  // namespace text
}  // namespace el

#endif  // EL_TEXT_UTF8_H_
