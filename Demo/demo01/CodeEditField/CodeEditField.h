#pragma once

#include "tb_editfield.h"

#include <vector>

class CodeEditField : public tb::TBEditField
{
public:
	enum SpecialStringTypes
	{
		Keyword,
		Variable
	};
	CodeEditField();

	virtual void OnInflate(const tb::INFLATE_INFO &info);

private:
	virtual void DrawString(tb::int32 x, tb::int32 y, tb::TBFontFace *font, const tb::TBColor &color, const char *str, tb::int32 len);
	virtual void OnBreak();

	bool StringHasColorOverride(const char* str, tb::int32 len, tb::TBColor& colour);

	bool inComment;
};
