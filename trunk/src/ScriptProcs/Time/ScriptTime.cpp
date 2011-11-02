#include "ScriptTime.h"
#include <stdio.h>
#include <stdlib.h>
#include "../../Global.h"
#include "../../timer.h"
#include "../../malloc.h"
#include "../../stringUtil.h"
#include "../../Unicode.h"
#include "../../util.h"

void GetTime(ScriptValue &s, ScriptValue *args) {
	CreateIntValue(s, time64i());
}

void ScriptGetTickCount(ScriptValue &s, ScriptValue *args) {
	CreateIntValue(s, GetTickCountNoOverflow());
}

static const char *tzlist[][2] = {
	{"A",   	"A  -1"},
	{"ACST",   	"ACS-9:30ACD"},
	{"AST",   	"AST4ADT"},
	{"AEST",   	"AES-10AED"},
	{"AHST",   	"AHT10ADT"},
	{"AHT",   	"AHT10ADT"},
	{"AKST",   	"AKS9AKD"},
	{"AZT",   	"AZT2AZD"},
	{"AWST",   	"AWS-8ADT"},
	{"B",   	"B  -2"},
	{"BST",   	"BST-1"},
	{"BZT",   	"BZT2BZD"},
	{"C",   	"C  -3"},
	{"CST",   	"CST6CDT"},
	{"CET",   	"CET-1CES"},
	{"CAS",   	"CST-9:30CST"},
	{"CXT",   	"CXT-7"},
	{"D",   	"D  -4"},
	{"E",   	"E  -5"},
	{"EST",   	"EST5EDT"},
	{"EET",   	"EET-2EDT"},
	{"F",   	"F  -6"},
	{"G",   	"G  -7"},
	{"GMT",   	"GMT0"},
	{"H",   	"H  -8"},
	{"HAA",   	"HAA3"},
	{"HAC",   	"HAC5"},
	{"HAST",   	"HAS10HAD"},
	{"HAE",   	"HAE4"},
	{"HAP",   	"HAP7"},
	{"HAR",   	"HAR6"},
	{"HAT",   	"HAT2:30"},
	{"HAY",   	"HAY8"},
	{"HNA",   	"HNA4"},
	{"HNC",   	"HNC6"},
	{"HNE",   	"HNE5"},
	{"HNP",   	"HNP8"},
	{"HNR",   	"HNR7"},
	{"HNT",   	"HNT3:30"},
	{"HNY",   	"HNY9"},
	{"HST",   	"HST10:30HDT"},
	{"I",   	"I  -9"},
	{"IST",   	"IST-5:30IDT"},
	{"IRT",   	"IRT-3:30IRD"},
	{"JST",   	"JST-9JDT"},
	{"JVT",   	"JVT-7:30JVD"},
	{"K",   	"K  -10"},
	{"KET",   	"KET-2:30"},
	{"L",   	"L  -11"},
	{"M",   	"M  -12"},
	{"MST",   	"MST7MDT"},
	{"MEZ",   	"MEZ-1MES"},
	{"N",   	"N  1"},
	{"NAT",   	"NAT11"},
	{"NST",   	"NST3:30NDT"},
	{"NFT",   	"NFT-11:30"},
	{"NZT",   	"NZT-12NZD"},
	{"O",   	"O  2"},
	{"P",   	"P  3"},
	{"PST",   	"PST8PDT"},
	{"Q",   	"Q  4"},
	{"R",   	"R  5"},
	{"S",   	"S  6"},
	{"T",   	"T  7"},
	{"U",   	"U  8"},
	{"V",   	"V  9"},
	{"W",   	"W  10"},
	{"WEDT",   	"WET0WED"},
	{"WEST",   	"WET0WES"},
	{"WET",   	"WET0"},
	{"WST",   	"WST-8WST"},
	{"YST",   	"YST-9YDT"},
	{"X",   	"X  11"},
	{"Y",   	"Y  12"},
	{"Z",   	"Z  0"},
	{"UTC",   	"UTC0"},
};

void SetLocale(ScriptValue &s, ScriptValue *args) {
	CreateIntValue(s, 0!=SetThreadLocale(MAKELCID(MAKELANGID(args[0].i32, args[1].i32), args[2].i32)));
}

void FormatTime(ScriptValue &s, ScriptValue *args) {
	unsigned char *c = args[0].stringVal->value;
	if (!AllocateStringValue(s, (int)(3 * strlen(c) + 14))) return;
	static bool settz = 0;
	if (args[0].stringVal->value[0] == 'S') {
		args[0].stringVal->value[0] = args[0].stringVal->value[0];
	}
	// Nasty layout, but boils down to:  If a tz is specified, try to set it.
	// If either couldn't set it or not specified and was set previously, unset it.
	while (1) {
		if (args[2].stringVal->len >= 1 && args[2].stringVal->len <= 20) {
			if (args[2].stringVal->len < 4 || (args[2].stringVal->len == 4 && !IsNumber(args[2].stringVal->value[3]))) {
				for (int i=0; i<sizeof(tzlist)/sizeof(tzlist[0]); i++) {
					if (!stricmp(tzlist[i][0], (char*)args[2].stringVal->value)) {
						char temp[60] = "TZ=";
						sprintf(temp+3, tzlist[i][1]);
						if (!_putenv(temp)) {
							_tzset();
							settz = 1;
							break;
						}
					}
				}
			}
			else {
				wchar_t temp[60] = L"TZ=";
				if (UTF8toUTF16(temp+3, args[2].stringVal->value, args[2].stringVal->len) > 0) {
					if (!_wputenv(temp)) {
						_tzset();
						settz = 1;
						break;
					}
				}
			}
		}
		if (settz) {
			if (!_putenv("TZ=")) {
				_tzset();
				settz = 0;
			}
		}
		break;
	}
	static LCID locale = 0;
	if (!locale) {
		locale = GetThreadLocale();
	}
	__int64 x = args[1].intVal;
	if (!x) x = time64i();
	tm *date = _localtime64(&x);
	SYSTEMTIME st;

	st.wYear = date->tm_year+1900;
	st.wMonth = date->tm_mon+1;
	st.wDayOfWeek = date->tm_wday;
	st.wDay = date->tm_mday;
	st.wHour = date->tm_hour;
	st.wMinute = date->tm_min;
	st.wSecond = date->tm_sec;
	st.wMilliseconds = 0;
	char *out = (char*) s.stringVal->value;
	char * start = (char*) c;
	//static const char *wday[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	//static const char *wday2[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
	//static const char *months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	//static const char *months2[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
	while (*start) {
		if (LCASE(start[0]) == 'h') {
			int h = date->tm_hour;
			if (*start == 'h') {
				if (h >12) h -= 12;
				else if (!h) h = 12;
			}
			GenericDisplay(start, out, h);
		}
		else if (LCASE(start[0]) == 'n')
			GenericDisplay(start, out, date->tm_min);
		else if (LCASE(start[0]) == 's')
			GenericDisplay(start, out, date->tm_sec);
		else if (LCASE(start[0]) == 't') {
			char letter = 'A';
			if (date->tm_hour >= 12) letter = 'P';
			if (start[0] == 't')
				out++[0]  = LCASE(letter);
			else
				out++[0]  = letter;
			start ++;
			if (start[0] == 't') {
				out++[0]  = 'm';
				start ++;
			}
			else if (start[0] == 'T') {
				out++[0]  = 'M';
				start ++;
			}
		}
		else if (LCASE(start[0]) == 'd') {
			if (LCASE(start[1]) == 'd' && LCASE(start[2]) == 'd') {
				start += 2;
				int incr = 1;
				wchar_t day[100];
				int len;
				if (LCASE(start[1]) == 'd') {
					len = GetDateFormat(locale, 0, &st, L"dddd", day, sizeof(day)/sizeof(wchar_t));
					//day = wday2[date->tm_wday];
					incr = 2;
				}
				else {
					len = GetDateFormat(locale, 0, &st, L"ddd", day, sizeof(day)/sizeof(wchar_t));
				}
				if (len < 0) len = 0;
				else if (len > 99) len = 99;
				day[len] = 0;
				int e = GetLastError();
				wchar_t *p = day;
				int i;
				for (i=0; i<len && i <99; i++) {
					p[i] = towupper(p[i]);
					if (start[0] != 'D') break;
				}
				UTF16toUTF8(out, day);
				out += strlen(out);
				start += incr;
			}
			else
				GenericDisplay(start, out, date->tm_mday);
		}
		else if (LCASE(start[0]) == 'm') {
			if (LCASE(start[1]) == 'm' && LCASE(start[2]) == 'm') {
				start += 2;
				int incr = 1;
				wchar_t day[100];
				int len;
				if (LCASE(start[1]) == 'm') {
					len = GetDateFormat(locale, 0, &st, L"MMMM", day, sizeof(day)/sizeof(wchar_t));
					//day = wday2[date->tm_wday];
					incr = 2;
				}
				else {
					len = GetDateFormat(locale, 0, &st, L"MMM", day, sizeof(day)/sizeof(wchar_t));
				}
				if (len < 0) len = 0;
				else if (len > 99) len = 99;
				day[len] = 0;
				int e = GetLastError();
				wchar_t *p = day;
				int i;
				for (i=0; i<len && i <99; i++) {
					p[i] = towupper(p[i]);
					if (start[0] != 'M') break;
				}
				UTF16toUTF8(out, day);
				out += strlen(out);
				start += incr;
			}
			else
				GenericDisplay(start, out, 1+date->tm_mon);
		}
		else if (LCASE(start[0]) == 'y') {
			int year = date->tm_year%100;
			if (LCASE(start[1]) == 'y') {
				start++;
				if (LCASE(start[1]) == 'y') {
					start ++;
					year = 1900 + date->tm_year;
					if (LCASE(start[1]) == 'y') start++;
				}
			}
			sprintf(out, "%02i", year);
			out+=strlen(out);
			start++;
		}
		else {
			out ++[0] = start++[0];
		}
		if ((out-(char*)s.stringVal->value) + 20 > s.stringVal->len) {
			size_t d = out - (char*)s.stringVal->value;
			ResizeStringValue(s, s.stringVal->len+50);
			out = (char*)s.stringVal->value + d;
		}
	}
	ResizeStringValue(s, (int)(out-(char*)s.stringVal->value));
}

