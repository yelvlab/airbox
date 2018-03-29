#include "SensorWinSen.h"

#include <string.h>

#define Uint16To8(High, Low) (((uint16_t)High << 8) | Low)

static uint8_t WinSenSumCheck(uint8_t * Pack, uint16_t Len)
{
	uint8_t res = 0;
	for (uint8_t i = 1; i != Len; i++)
	{
		res += Pack[i];
		// printf("res: %X | Pack[i]: %X\n", res, Pack[i]);
	}
	return (~res) + 1;
}

static bool ZH03SumCheck(uint8_t * Pack, uint16_t PackLen)
{
	if (Uint16To8(Pack[2], Pack[3]) == PackLen - 4)
	{
		uint16_t res = 0, len = Uint16To8(Pack[2], Pack[3]);
		uint16_t asc = Uint16To8(Pack[PackLen - 2], Pack[PackLen - 1]);
		for (uint8_t i = 0; i != len; i++)
		{
			res += Pack[i];
		}
		// printf("%04hhX : %04hhX\n", res, asc);
		return res == asc;
	}
	return false;
}

// With The ZH03IntoSleep Check Result In Monopolize
bool ZH03CheckSleep(uint8_t * IN Pack, uint16_t IN PackLen)
{
	if (0xFF == Pack[0] && 0xA7 == Pack[1] && AskLen == PackLen && Pack[PackLen - 1] == WinSenSumCheck(Pack, PackLen - 1))
	{
		return Pack[2];
	}
	return false;
}

uint8_t ZH03Parse(uint8_t * IN Pack, uint16_t IN PackLen, uint16_t * OUT PM1_0, uint16_t * OUT PM2_5, uint16_t * OUT PM10_0)
{
	// Default Model
	if (0x42 == Pack[0] && 0x4D == Pack[1])
	{
		if (true == ZH03SumCheck(Pack, PackLen))
		{
			*PM1_0 = Uint16To8(Pack[10], Pack[11]);
			*PM2_5 = Uint16To8(Pack[12], Pack[13]);
			*PM10_0 = Uint16To8(Pack[14], Pack[15]);
			return Default;
		}
	}
	// Ask Model
	else if (0xFF == Pack[0] && AskLen == PackLen && Pack[PackLen - 1] == WinSenSumCheck(Pack, PackLen - 1))
	{
		switch (Pack[1])
		{
			case 0x86:
				*PM1_0 = Uint16To8(Pack[2], Pack[3]);
				*PM2_5 = Uint16To8(Pack[4], Pack[5]);
				*PM10_0 = Uint16To8(Pack[6], Pack[7]);
				return Ask;
				// Not Parse Sleep Cmd, Because It needed Monopoly.
			default:
				break;
		}
	}
	return ERROR;
}

uint8_t ZE08CH20Parse(uint8_t * IN Pack, uint16_t IN PackLen, uint16_t * OUT PPB, uint16_t * OUT FullRangeOrUgM3)
{
	if (0xFF == Pack[0] && AskLen == PackLen && Pack[PackLen - 1] == WinSenSumCheck(Pack, PackLen - 1))
	{
		switch (Pack[1])
		{
			// Default Model
			case 0x17:
				*PPB = Uint16To8(Pack[4], Pack[5]);
				*FullRangeOrUgM3 = Uint16To8(Pack[6], Pack[7]);
				return Default;
			// Ask Model
			case 0x86:
				*PPB = Uint16To8(Pack[6], Pack[7]);
				*FullRangeOrUgM3 = Uint16To8(Pack[2], Pack[3]);
				return Ask;
			default:
				break;
		}
	}
	return ERROR;
}

uint8_t MHZ14AParse(uint8_t * IN Pack, uint16_t IN PackLen, uint16_t * OUT PPM)
{
	if (0xFF == Pack[0] && AskLen == PackLen && Pack[PackLen - 1] == WinSenSumCheck(Pack, PackLen - 1))
	{
		switch (Pack[1])
		{
			case 0x86:
				*PPM = Uint16To8(Pack[2], Pack[3]);
				return Ask;
			default:
				break;
		}
	}
	return ERROR;
}

const char * MHZ14ARequestAdjustSpan(uint8_t SpanHigh, uint8_t SpanLow)
{
	static char AdjustSpan[] = "\xFF\x01\x88\x00\x00\x00\x00\x00\x00";
	AdjustSpan[3] = SpanHigh, AdjustSpan[4] = SpanLow;
	AdjustSpan[AskLen - 1] = WinSenSumCheck(AdjustSpan, AskLen - 1);
	return AdjustSpan;
}

#ifdef UNIT_TEST

#include <stdio.h>
#include <assert.h>

int unit_test_ZH03()
{
	uint16_t a = 0, b = 0, c = 0;

	uint8_t TestCase1[] = { 0x42, 0x4D, 0x00, 0x14, 0x10, 0x06, 0x00, 0x09, 0x00, 0x0A, 0x00, 0x06, 0x00, 0x09, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD5 };
	uint8_t TestCase2[] = { 0x42, 0x4D, 0x00, 0x14, 0x00, 0x0D, 0x00, 0x11, 0x00, 0x13, 0x00, 0x0D, 0x00, 0x11, 0x00, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x05 };
	uint8_t TestCase3[] = { 0x42, 0x4D, 0x00, 0x14, 0x02, 0x0D, 0x00, 0x11, 0x00, 0x13, 0x00, 0x0D, 0x00, 0x11, 0x00, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x05 };
	assert(ERROR == ZH03Parse(TestCase1, sizeof(TestCase1), &a, &b, &c));
	assert(Default == ZH03Parse(TestCase2, sizeof(TestCase2), &a, &b, &c));
	printf("%04hhX : %04hhX : %04hhX\n", a, b, c);
	assert(ERROR == ZH03Parse(TestCase3, sizeof(TestCase3), &a, &b, &c));

	uint8_t TestCase4[] = { 0xFF, 0x86, 0x00, 0x47, 0x00, 0xC7, 0x03, 0x0F, 0x51 };
	uint8_t TestCase5[] = { 0xFF, 0x86, 0x00, 0x16, 0x00, 0x18, 0x00, 0x10, 0x3C };
	uint8_t TestCase6[] = { 0xFF, 0x86, 0x00, 0x16, 0x00, 0x18, 0x00, 0x00, 0x3C };
	assert(ERROR == ZH03Parse(TestCase4, sizeof(TestCase4), &a, &b, &c));
	assert(Ask == ZH03Parse(TestCase5, sizeof(TestCase5), &a, &b, &c));
	printf("%04hhX : %04hhX : %04hhX\n", a, b, c);
	assert(ERROR == ZH03Parse(TestCase6, sizeof(TestCase6), &a, &b, &c));

	uint8_t TestCase7[] = { 0xFF, 0xA7, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x58 };
	assert(true == ZH03CheckSleep(TestCase7, sizeof(TestCase7)));

	uint8_t TestCase8[] = { 0xFF, 0xA7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x59 };
	assert(false == ZH03CheckSleep(TestCase8, sizeof(TestCase8)));

	uint8_t TestCase9[] = { 0xFF, 0xA7, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x58 };
	assert(true == ZH03CheckSleep(TestCase9, sizeof(TestCase9)));

	uint8_t TestCase10[] = { 0xFF, 0xA7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x59 };
	assert(false == ZH03CheckSleep(TestCase10, sizeof(TestCase10)));

	uint8_t TestCase11[] = { 0xFF, 0xA7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	assert(false == ZH03CheckSleep(TestCase11, sizeof(TestCase11)));

	return 0;
}

int unit_test_ZE08CH20()
{
	uint16_t PPB = 0, FullRange = 0, UgM3 = 0;

	uint8_t TestCase1[] = { 0xFF, 0x17, 0x04, 0x00, 0x00, 0x25, 0x13, 0x88, 0x25 };
	assert(Default == ZE08CH20Parse(TestCase1, sizeof(TestCase1), &PPB, &FullRange));
	printf("PPB: %04hhX | FullRange : %04hhX \n", PPB, FullRange);
	uint8_t TestCase2[] = { 0xFF, 0x17, 0x04, 0x00, 0x00, 0x25, 0x13, 0x88, 0x24 };
	assert(ERROR == ZE08CH20Parse(TestCase2, sizeof(TestCase2), &PPB, &FullRange));

	uint8_t TestCase3[] = { 0xFF, 0x86, 0x00, 0x2A, 0x00, 0x00, 0x00, 0x20, 0x30 };
	assert(Ask == ZE08CH20Parse(TestCase3, sizeof(TestCase3), &PPB, &UgM3));
	printf("PPB: %04hhX | UgM3 : %04hhX \n", PPB, UgM3);
	uint8_t TestCase4[] = { 0xFF, 0x86, 0x00, 0x2A, 0x00, 0x00, 0x00, 0x20, 0x29 };
	assert(ERROR == ZE08CH20Parse(TestCase4, sizeof(TestCase4), &PPB, &UgM3));
	return 0;
}

int unit_test_MHZ14A()
{
	uint16_t PPM = 0;

	uint8_t TestCase1[] = { 0xFF, 0x86, 0x02, 0x20, 0x00, 0x00, 0x00, 0x00, 0x58 };
	assert(Ask == MHZ14AParse(TestCase1, sizeof(TestCase1), &PPM));
	printf("PPM: %04hhX \n", PPM);
	uint8_t TestCase4[] = { 0xFF, 0x86, 0x02, 0x20, 0x00, 0x00, 0x00, 0x01, 0x58 };
	assert(ERROR == MHZ14AParse(TestCase4, sizeof(TestCase4), &PPM));
	uint8_t TestCase2[] = { 0xFF, 0x86, 0x02, 0x20, 0x00, 0x00, 0x00, 0x00, 0x57 };
	assert(ERROR == MHZ14AParse(TestCase2, sizeof(TestCase2), &PPM));
	uint8_t TestCase3[] = { 0xFF, 0x86, 0x02, 0x20, 0x00, 0x00, 0x00, 0x20 };
	assert(ERROR == MHZ14AParse(TestCase3, sizeof(TestCase3), &PPM));

	return 0;
}

int main()
{
	unit_test_ZH03();
	unit_test_ZE08CH20();
	unit_test_MHZ14A();
}
#endif
