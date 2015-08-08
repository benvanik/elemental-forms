/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_UTIL_DIMENSION_CONVERTER_H_
#define EL_UTIL_DIMENSION_CONVERTER_H_

#include <string>

#include "el/config.h"

namespace el {
class Value;
}  // namespace el

namespace el {
namespace util {

class StringBuilder;

// Dimensions <= this value will be untouched by conversion in
// DimensionConverter.
// To preserve special constants, those must be <= this value.
constexpr int kInvalidDimension = -5555;

// Converts device independant points to pixels, based on two DPI values.
// Dimensions in Elemental are normally in pixels (if not specified
// differently) and conversion normally take place when loading skin.
class DimensionConverter {
 public:
  DimensionConverter() = default;

  int GetSrcDPI() const { return src_dpi_; }
  int GetDstDPI() const { return dst_dpi_; }
  // Sets the source and destination DPI that will affect the conversion.
  void SetDPI(int src_dpi, int dst_dpi);

  // Gets the file name with destination DPI suffix.
  // The temp buffer will contain the resulting file name.
  // F.ex "foo.png" becomes "foo@192.png"
  void GetDstDPIFilename(const std::string& filename,
                         util::StringBuilder* tempbuf) const;

  // Returns true if the source and destinatin DPI are different.
  bool NeedConversion() const { return src_dpi_ != dst_dpi_; }

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

 private:
  int src_dpi_ = 100;  // The source DPI (Normally the base_dpi from skin).
  int dst_dpi_ = 100;  // The destination DPI (Normally the supported skin DPI
                       // nearest to util::GetDPI).

  // The file name suffix that should be used to load bitmaps in the destination
  // DPI.
  // Examples: "@96", "@196"
  std::string dst_dpi_suffix_;
};

}  // namespace util
}  // namespace el

#endif  // EL_UTIL_DIMENSION_CONVERTER_H_
