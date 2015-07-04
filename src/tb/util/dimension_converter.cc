/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include <algorithm>
#include <cctype>
#include <cstdlib>

#include "tb/util/dimension_converter.h"
#include "tb/util/metrics.h"
#include "tb/util/string.h"
#include "tb/util/string_builder.h"
#include "tb/value.h"

namespace tb {
namespace util {

void DimensionConverter::SetDPI(int src_dpi, int dst_dpi) {
  src_dpi_ = src_dpi;
  dst_dpi_ = dst_dpi;
  dst_dpi_suffix_.clear();
  if (NeedConversion()) {
    dst_dpi_suffix_ = tb::util::format_string("@%d", dst_dpi_);
  }
}

void DimensionConverter::GetDstDPIFilename(const std::string& filename,
                                           util::StringBuilder* tempbuf) const {
  size_t dot_pos = filename.find_last_of('.');
  tempbuf->ResetAppendPos();
  tempbuf->Append(filename.c_str(), dot_pos);
  tempbuf->AppendString(dst_dpi_suffix_);
  tempbuf->AppendString(filename.c_str() + dot_pos);
}

int DimensionConverter::DpToPx(int dp) const {
  if (dp <= kInvalidDimension || dp == 0 || !NeedConversion()) {
    return dp;
  }
  if (dp > 0) {
    dp = dp * dst_dpi_ / src_dpi_;
    return std::max(dp, 1);
  } else {
    dp = dp * dst_dpi_ / src_dpi_;
    return std::min(dp, -1);
  }
}

int DimensionConverter::MmToPx(int mm) const {
  if (mm <= kInvalidDimension || mm == 0) {
    return mm;
  }
  return int(mm * util::GetDPI() / 25.4f + 0.5f);
}

int DimensionConverter::GetPxFromString(const char* str, int def_value) const {
  if (!str || !util::is_start_of_number(str)) {
    return def_value;
  }
  size_t len = strlen(str);
  int val = atoi(str);
  // "dp" and unspecified unit is dp.
  if ((len > 0 && isdigit(str[len - 1])) ||
      (len > 2 && strcmp(str + len - 2, "dp") == 0)) {
    return DpToPx(val);
  } else if (len > 2 && strcmp(str + len - 2, "mm") == 0) {
    return MmToPx(val);
  } else {
    return val;
  }
}

int DimensionConverter::GetPxFromValue(Value* value, int def_value) const {
  if (!value) {
    return def_value;
  }
  if (value->GetType() == Value::Type::kInt) {
    return DpToPx(value->GetInt());
  } else if (value->GetType() == Value::Type::kFloat) {
    // FIX: We might want float versions of all dimension functions.
    return DpToPx((int)value->GetFloat());
  }
  return GetPxFromString(value->GetString(), def_value);
}

}  // namespace util
}  // namespace tb
