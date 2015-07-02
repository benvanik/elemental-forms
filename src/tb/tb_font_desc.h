/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_FONT_DESC_H
#define TB_FONT_DESC_H

#include <algorithm>

#include "tb_id.h"
#include "tb_types.h"

namespace tb {

// Describes a font.
// By default when nothing is set, the font is unspecified and means it should
// be inherited from a parent widget that specifies a font, or use the default
// font if no parent does.
class FontDescription {
 public:
  // Sets the font ID of the font to use.
  // This ID maps to the font names in FontInfo, which is managed from
  // FontManager::AddFontInfo, FontManager::GetFontInfo.
  // Example:
  // If a font was added to the font manager with the name "Vera", you can
  // do font_description.SetID(TBIDC("Vera")).
  void SetID(const TBID& id) { m_id = id; }

  // Gets the TBID for the font name (See SetID).
  TBID GetID() const { return m_id; }

  // Gets the TBID for the FontFace that matches this font description.
  // This is a ID combining both the font file, and variation (such as size and
  // style), and should be used to identify a certain font face.
  // If this is 0, the font description is unspecified. For a widget, that means
  // that the font should be inherited from the parent widget.
  TBID GetFontFaceID() const { return m_id + m_packed_init; }

  void SetSize(uint32_t size) { m_packed.size = std::min(size, 0x8000u); }
  uint32_t GetSize() const { return m_packed.size; }

  // not connected to anything yet
  // void SetBold(bool bold)
  // {
  // m_packed.bold
  // =
  // bold; }
  // bool GetBold() const
  // {
  // return
  // m_packed.bold; }

  // not connected to anything yet
  // void SetItalic(bool italic)
  // {
  // m_packed.italic
  // = italic; }
  // bool GetItalic() const
  // {
  // return
  // m_packed.italic; }

  FontDescription() = default;
  FontDescription(const FontDescription& src) {
    m_packed_init = src.m_packed_init;
    m_id = src.m_id;
  }
  const FontDescription& operator=(const FontDescription& src) {
    m_packed_init = src.m_packed_init;
    m_id = src.m_id;
    return *this;
  }
  bool operator==(const FontDescription& fd) const {
    return m_packed_init == fd.m_packed_init && m_id == fd.m_id;
  }
  bool operator!=(const FontDescription& fd) const { return !(*this == fd); }

 private:
  TBID m_id;
  union {
    struct {
      uint32_t size : 15;
      uint32_t italic : 1;
      uint32_t bold : 1;
    } m_packed;
    uint32_t m_packed_init = 0;
  };
};

}  // namespace tb

#endif  // TB_FONT_DESC_H
