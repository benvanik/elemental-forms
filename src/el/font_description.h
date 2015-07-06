/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_FONT_DESCRIPTION_H_
#define EL_FONT_DESCRIPTION_H_

#include <algorithm>

#include "el/id.h"

namespace el {

// Describes a font.
// By default when nothing is set, the font is unspecified and means it should
// be inherited from a parent element that specifies a font, or use the default
// font if no parent does.
class FontDescription {
 public:
  FontDescription() = default;
  FontDescription(const FontDescription& src) {
    packed_init_ = src.packed_init_;
    id_ = src.id_;
  }
  const FontDescription& operator=(const FontDescription& src) {
    packed_init_ = src.packed_init_;
    id_ = src.id_;
    return *this;
  }

  // Gets the TBID for the font name (See SetID).
  TBID id() const { return id_; }
  // Sets the font ID of the font to use.
  // This ID maps to the font names in FontInfo, which is managed from
  // FontManager::AddFontInfo, FontManager::GetFontInfo.
  // Example:
  // If a font was added to the font manager with the name "Vera", you can
  // do font_description.set_id(TBIDC("Vera")).
  void set_id(const TBID& id) { id_ = id; }

  // Gets the TBID for the FontFace that matches this font description.
  // This is a ID combining both the font file, and variation (such as size and
  // style), and should be used to identify a certain font face.
  // If this is 0, the font description is unspecified. For a element, that
  // means that the font should be inherited from the parent element.
  TBID font_face_id() const { return id_ + packed_init_; }

  uint32_t size() const { return packed_.size; }
  void set_size(uint32_t size) { packed_.size = std::min(size, 0x8000u); }

  bool is_bold() const { return !!packed_.bold; }
  void set_bold(bool value) { packed_.bold = value ? 1 : 0; }

  bool is_italic() const { return !!packed_.italic; }
  void set_italic(bool value) { packed_.italic = value ? 1 : 0; }
  bool operator==(const FontDescription& fd) const {
    return packed_init_ == fd.packed_init_ && id_ == fd.id_;
  }
  bool operator!=(const FontDescription& fd) const { return !(*this == fd); }

 private:
  TBID id_;
  union {
    struct {
      uint32_t size : 15;
      uint32_t italic : 1;
      uint32_t bold : 1;
    } packed_;
    uint32_t packed_init_ = 0;
  };
};

}  // namespace el

#endif  // EL_FONT_DESCRIPTION_H_
