#include "CodeEditField.h"

#include "tb_widgets_reader.h"
#include <ctype.h>

using namespace tb;
TB_WIDGET_FACTORY(CodeEditField, TBValue::TYPE_STRING, WIDGET_Z_TOP) {}

CodeEditField::CodeEditField()
	: TBEditField()
	, inComment(false)
{
}

void CodeEditField::OnInflate(const INFLATE_INFO &info)
{
	TBEditField::OnInflate(info);
}

void CodeEditField::DrawString(int32 x, int32 y, TBFontFace *font, const TBColor &color, const char *str, int32 len)
{
	TBColor finalColor(color);
	StringHasColorOverride(str, len, finalColor);
	TBEditField::DrawString(x, y, font, finalColor, str, len);
}

void CodeEditField::OnBreak()
{
	inComment = false;
}

bool CodeEditField::StringHasColorOverride(const char* str, int32 len, TBColor& colour)
{
	if (strlen(str) >= 2)
	{
		if (str[0] == '/' && str[1] == '/')
		{
			inComment = true;
		}
	}

	if (inComment)
	{
		colour = TBColor(113, 143, 113);
		return true;
	}

	char* keywords[] = {
		"in",
		"vec3",
		"uvec2",
		"const",
		"uniform",
		"void",
		"if",
		"float",
		"vec4",
		"for",
		"uint",
		"abs",
		"sin",
		"cos",
		"texture",
		"int"
	};

	for (int32 keywordIdx = 0; keywordIdx < 16; keywordIdx++)
	{
		char* matchAgainst = keywords[keywordIdx];
		int32 matchLen = (int32)strlen(matchAgainst);
		if (matchLen == len)
		{
			auto matched = true;
			for (int32 i = 0; i < len; i++)
			{
				if (toupper(matchAgainst[i]) != toupper(str[i]))
				{
					matched = false;
					break;
				}
			}

			if (matched)
			{
				colour = TBColor(90, 127, 230);
				return true;
			}
		}
	}

	return false;
}
