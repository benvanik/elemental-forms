// ================================================================================
// == This file is a part of TinkerBell UI Toolkit. (C) 2011-2013, Emil Seger�s  ==
// ==                   See tinkerbell.h for more information.                   ==
// ================================================================================

#include "tb_test.h"
#include "tinkerbell.h"

#ifdef TB_UNIT_TESTING

using namespace tinkerbell;

TB_TEST_GROUP(tb_color)
{
	TB_TEST(set_from_string_rrggbbaa)
	{
		TBColor col;
		col.SetFromString("#11223344", 9);
		TB_VERIFY(col.r == 0x11);
		TB_VERIFY(col.g == 0x22);
		TB_VERIFY(col.b == 0x33);
		TB_VERIFY(col.a == 0x44);
	}
	TB_TEST(set_from_string_rrggbb)
	{
		TBColor col;
		col.SetFromString("#112233", 7);
		TB_VERIFY(col.r == 0x11);
		TB_VERIFY(col.g == 0x22);
		TB_VERIFY(col.b == 0x33);
		TB_VERIFY(col.a == 0xff);
	}
	TB_TEST(set_from_string_rgba)
	{
		TBColor col;
		col.SetFromString("#1234", 5);
		TB_VERIFY(col.r == 0x11);
		TB_VERIFY(col.g == 0x22);
		TB_VERIFY(col.b == 0x33);
		TB_VERIFY(col.a == 0x44);
	}
	TB_TEST(set_from_string_rgb)
	{
		TBColor col;
		col.SetFromString("#123", 4);
		TB_VERIFY(col.r == 0x11);
		TB_VERIFY(col.g == 0x22);
		TB_VERIFY(col.b == 0x33);
		TB_VERIFY(col.a == 0xff);
	}
	TB_TEST(set_from_string_invalid)
	{
		TBColor col;
		col.SetFromString("123", 3);
		TB_VERIFY(col.r == 0x0);
		TB_VERIFY(col.g == 0x0);
		TB_VERIFY(col.b == 0x0);
		TB_VERIFY(col.a == 0xff);
	}
}

#endif // TB_UNIT_TESTING