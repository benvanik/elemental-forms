/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_DIMENSION_H
#define TB_DIMENSION_H

#include "tb_debug.h"
#include "tb_str.h"
#include "tb_types.h"

namespace tb {

class TBTempBuffer;
class Value;

// Dimensions <= this value will be untouched by conversion in
// DimensionConverter.
// To preserve special constants, those must be <= this value.
constexpr int kInvalidDimension = -5555;

// Converts device independant points to pixels, based on two DPI values.
// Dimensions in Turbo Badger are normally in pixels (if not specified
// differently) and conversion normally take place when loading skin.
class DimensionConverter {
  int m_src_dpi = 100;  // The source DPI (Normally the base_dpi from skin).
  int m_dst_dpi = 100;  // The destination DPI (Normally the supported skin DPI
                        // nearest to TBSystem::GetDPI).
  std::string m_dst_dpi_str;  // The file suffix that should be used to load
                              // bitmaps in destinatin DPI.
 public:
  DimensionConverter() = default;

  // Sets the source and destination DPI that will affect the conversion.
  void SetDPI(int src_dpi, int dst_dpi);

  int GetSrcDPI() const { return m_src_dpi; }
  int GetDstDPI() const { return m_dst_dpi; }

  // Gets the file name suffix that should be used to load bitmaps in the
  // destination DPI.
  // Examples: "@96", "@196"
  const std::string& GetDstDPIStr() const { return m_dst_dpi_str; }

  // Gets the file name with destination DPI suffix.
  // The temp buffer will contain the resulting file name.
  // F.ex "foo.png" becomes "foo@192.png"
  void GetDstDPIFilename(const std::string& filename,
                         TBTempBuffer* tempbuf) const;

  // Returns true if the source and destinatin DPI are different.
  bool NeedConversion() const { return m_src_dpi != m_dst_dpi; }

  // Converts device independant points to pixels.
  int DpToPx(int dp) const;

  // Convert millimeters to pixels.
  int MmToPx(int mm) const;

  // Gets a pixel value from string in any of the following formats:
  // str may be nullptr. def_value is returned on fail.
  //     Device independent point:    "1", "1dp"
  //     Pixel value:                 "1px"
  int GetPxFromString(const char* str, int def_value) const;

  // Gets a pixel value from Value.
  // value may be nullptr. def_value is returned on fail.
  // Number formats are treated as dp.
  // String format is treated like for GetPxFromString.
  int GetPxFromValue(Value* value, int def_value) const;
};

}  // namespace tb

#endif  // TB_DIMENSION_H
