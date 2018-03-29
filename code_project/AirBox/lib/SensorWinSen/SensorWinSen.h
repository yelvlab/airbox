#ifndef _SENSOR_WIN_SEN_H_
#define _SENSOR_WIN_SEN_H_

#include <stdint.h>
#include <stdbool.h>

#define IN  // Tip Param In
#define OUT // Tip Param Out

// ------------ WinSen ------------

enum WinSenPack
{
	ERROR = 0,
	Ask = 'A',
	AskLen = 9,
	AskHeader = '\xFF',
	Default = 'D',

	ZH03_DefaultLen = 24,
	ZH03_DefaultHeader = '\x42',
};

static const char WinSenRequestAsk[] = "\xFF\x01\x86\x00\x00\x00\x00\x00\x79";

static const char WinSenSetModelAsk[] = "\xFF\x01\x78\x41\x00\x00\x00\x00\x46";

static const char WinSenSetModelDefault[] = "\xFF\x01\x78\x40\x00\x00\x00\x00\x47";

// ------------ WinSen ZH03 ------------

uint8_t ZH03Parse(uint8_t * IN Pack, uint16_t IN PackLen, uint16_t * OUT PM1_0, uint16_t * OUT PM2_5, uint16_t * OUT PM10_0);

static const char ZH03IntoSleep[] = "\xFF\x01\xA7\x01\x00\x00\x00\x00\x57";

static const char ZH03ExitSleep[] = "\xFF\x01\xA7\x00\x00\x00\x00\x00\x58";

bool ZH03CheckSleep(uint8_t * IN Pack, uint16_t IN PackLen);

// ------------ WinSen ZE08-CH20 ------------

uint8_t ZE08CH20Parse(uint8_t * IN Pack, uint16_t IN PackLen, uint16_t * OUT PPB, uint16_t * OUT FullRangeOrUgM3);

// ------------ WinSen MH-Z14A ------------

static const char MHZ14ARequestAdjustZero[] = "\xFF\x01\x87\x00\x00\x00\x00\x00\x78";

static const char MHZ14AOpenAutoAdjustZero[] = "\xFF\x01\x79\xA0\x00\x00\x00\x00\xE6";

static const char MHZ14ACloseAutoAdjustZero[] = "\xFF\x01\x79\x00\x00\x00\x00\x00\x86";

const char * MHZ14ARequestAdjustSpan(uint8_t SpanHigh, uint8_t SpanLow);

uint8_t MHZ14AParse(uint8_t * IN Pack, uint16_t IN PackLen, uint16_t * OUT PPM);

static const char MHZ14ASetFullRange2000[] = "\xFF\x01\x99\x00\x00\x00\x07\xD0\x8F";

static const char MHZ14ASetFullRange5000[] = "\xFF\x01\x99\x00\x00\x00\x13\x88\xCB";

static const char MHZ14ASetFullRange10000[] = "\xFF\x01\x99\x00\x00\x00\x27\x10\x2F";

#endif // _SENSOR_WIN_SEN_H_


