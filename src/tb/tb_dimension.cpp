/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_dimension.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>

#include "tb_system.h"
#include "tb_tempbuffer.h"
#include "tb_types.h"
#include "tb_value.h"

namespace tb {

void TBDimensionConverter::SetDPI(int src_dpi, int dst_dpi) {
  m_src_dpi = src_dpi;
  m_dst_dpi = dst_dpi;
  m_dst_dpi_str.clear();
  if (NeedConversion()) {
    m_dst_dpi_str = tb::format_string("@%d", m_dst_dpi);
  }
}

void TBDimensionConverter::GetDstDPIFilename(const std::string& filename,
                                             TBTempBuffer* tempbuf) const {
  size_t dot_pos = 0;
  for (dot_pos = filename.size() - 1; dot_pos > 0; dot_pos--) {
    if (filename[dot_pos] == '.') break;
  }
  tempbuf->ResetAppendPos();
  tempbuf->Append(filename.c_str(), dot_pos);
  tempbuf->AppendString(GetDstDPIStr());
  tempbuf->AppendString(filename.c_str() + dot_pos);
}

int TBDimensionConverter::DpToPx(int dp) const {
  if (dp <= kInvalidDimension || dp == 0 || !NeedConversion()) return dp;
  if (dp > 0) {
    dp = dp * m_dst_dpi / m_src_dpi;
    return std::max(dp, 1);
  } else {
    dp = dp * m_dst_dpi / m_src_dpi;
    return std::min(dp, -1);
  }
}

int TBDimensionConverter::MmToPx(int mm) const {
  if (mm <= kInvalidDimension || mm == 0) return mm;

  return (int)(mm * TBSystem::GetDPI() / 25.4f + 0.5f);
}

int TBDimensionConverter::GetPxFromString(const char* str,
                                          int def_value) const {
  if (!str || !is_start_of_number(str)) return def_value;
  size_t len = strlen(str);
  int val = atoi(str);
  // "dp" and unspecified unit is dp.
  if ((len > 0 && isdigit(str[len - 1])) ||
      (len > 2 && strcmp(str + len - 2, "dp") == 0))
    return DpToPx(val);
  else if (len > 2 && strcmp(str + len - 2, "mm") == 0)
    return MmToPx(val);
  else
    return val;
}

int TBDimensionConverter::GetPxFromValue(TBValue* value, int def_value) const {
  if (!value) return def_value;
  if (value->GetType() == TBValue::Type::kInt)
    return DpToPx(value->GetInt());
  else if (value->GetType() == TBValue::Type::kFloat)
    // FIX: We might want float versions of all dimension functions.
    return DpToPx((int)value->GetFloat());
  return GetPxFromString(value->GetString(), def_value);
}

}  // namespace tb
