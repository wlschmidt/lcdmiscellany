#include "ScriptDrawing.h"
#include "../Screen.h"
#include "../ScriptObjects/ScriptFont.h"
#include "../Unicode.h"
#include "../malloc.h"
#include "../vm.h"

void ClearScreen(ScriptValue &s, ScriptValue *args) {
	activeScreen->Clear();
}

void DrawPixel(ScriptValue &s, ScriptValue *args) {
	activeScreen->ColorPixel(args[0].i32, args[1].i32, activeScreen->drawColor);
}

void InvertPixel(ScriptValue &s, ScriptValue *args) {
	activeScreen->TogglePixel(args[0].i32, args[1].i32);
}

void ClearPixel(ScriptValue &s, ScriptValue *args) {
	activeScreen->ColorPixel(args[0].i32, args[1].i32, activeScreen->bgColor);
}

void DrawLine(ScriptValue &s, ScriptValue *args) {
	activeScreen->DrawLine(args[0].i32, args[1].i32, args[2].i32, args[3].i32);
}

void InvertLine(ScriptValue &s, ScriptValue *args) {
	activeScreen->InvertLine(args[0].i32, args[1].i32, args[2].i32, args[3].i32);
}

void ClearLine(ScriptValue &s, ScriptValue *args) {
	activeScreen->ClearLine(args[0].i32, args[1].i32, args[2].i32, args[3].i32);
}

void ClearRect(ScriptValue &s, ScriptValue *args) {
	activeScreen->ClearRect(args[0].i32, args[1].i32, args[2].i32, args[3].i32);
}

void InvertRect(ScriptValue &s, ScriptValue *args) {
	activeScreen->InvertRect(args[0].i32, args[1].i32, args[2].i32, args[3].i32);
}

void DrawRect(ScriptValue &s, ScriptValue *args) {
	activeScreen->DrawRect(args[0].i32, args[1].i32, args[2].i32, args[3].i32);
}

void ColorRect(ScriptValue &s, ScriptValue *args) {
	RECT r = {
		args[0].i32,
		args[1].i32,
		args[2].i32,
		args[3].i32,
	};
	activeScreen->FillRect(r, *(Color4*)&args[4].i32);
}

void ColorLine(ScriptValue &s, ScriptValue *args) {
	if (args[0].type != SCRIPT_LIST) return;
	int numVals = args[0].listVal->numVals;
	ScriptValue *vals = args[0].listVal->vals;
	if (numVals < 5) return;

	Color4 color;
	color.val = (int) CoerceIntNoRelease(vals[0]);
	int lx = (int) CoerceIntNoRelease(vals[1]);
	int ly = (int) CoerceIntNoRelease(vals[2]);
	for (int i = 4; i < numVals; i+=2) {
		int x = (int) CoerceIntNoRelease(vals[i-1]);
		int y = (int) CoerceIntNoRelease(vals[i]);

		activeScreen->ColorLine(lx, ly, x, y, color);

		lx = x;
		ly = y;
	}
}

void XorRect(ScriptValue &s, ScriptValue *args) {
	Color4 c;
	c.val = args[4].i32;
	activeScreen->XorRect(args[0].i32, args[1].i32, args[2].i32, args[3].i32, c);
}

void TextSize(ScriptValue &s, ScriptValue *args) {
	int height;
	if (CreateListValue(s, 2)) {
		int width = activeScreen->GetTextWidth(args[0].stringVal->value, args[0].stringVal->len, &height);
		s.listVal->PushBack(width);
		s.listVal->PushBack(height);
	}
}

void DrawClippedText(ScriptValue &s, ScriptValue *args) {
	RECT r = {args[1].i32, args[2].i32, args[3].i32, args[4].i32};
	activeScreen->DisplayText(args[1].i32, args[2].i32, args[0].stringVal->value, args[0].stringVal->len, 0, 0, &r, args[5].i32);
}

void DisplayText(ScriptValue &s, ScriptValue *args) {
	activeScreen->DisplayText(args[1].i32, args[2].i32, args[0].stringVal->value, args[0].stringVal->len, args[3].i32, args[4].i32);
	int junk;
	CreateIntValue(s, activeScreen->GetTextWidth(args[0].stringVal->value, args[0].stringVal->len, &junk));
}

void DisplayTextCentered(ScriptValue &s, ScriptValue *args) {
	int width = activeScreen->DisplayTextCentered(args[1].i32, args[2].i32, args[0].stringVal->value, args[0].stringVal->len, args[3].i32, args[4].i32);
	CreateIntValue(s, width);
}

void Redraw(StringValue *dev);
void RedrawNow(StringValue *dev);

void DisplayTextRight(ScriptValue &s, ScriptValue *args) {
	int width = activeScreen->DisplayTextRight(args[1].i32, args[2].i32, args[0].stringVal->value, args[0].stringVal->len, args[3].i32, args[4].i32);
	CreateIntValue(s, width);
}

void NeedRedraw(ScriptValue &s, ScriptValue *args) {
	StringValue * string = 0;
	if (args[1].stringVal->value[0])
		string = args[1].stringVal;
	if (args[0].intVal==-1)
		RedrawNow(string);
	else
		Redraw(string);
}

int *FormatText(unsigned char *s, int len, int width, int &count) {
	int wordLen = 0;
	int lineLen = 0;
	int line = 0;
	int wordChars = 0;
	int lineChars = 0;
	int pos = 0;
	int maxLines = 1+len /10;
	int *breaks = (int*)malloc(sizeof(int)*maxLines);
	DynamicFont *f = GetNewFontMetrics(activeScreen->activeNewFont);
	int bold = 0;
	while (pos < len) {
		int l;
		unsigned int c = NextUnicodeChar(s+pos, &l);
		if (c==2) {
			wordChars++;
			bold ^= 1;
			pos ++;
			continue;
		}
		BitImage *b;

		if (c == '\r') {
			c = '\n';
			if (pos < len-1 && s[pos+1] == '\n') {
				pos++;
			}
		}
		if (c == '\n') {
			pos ++;
			wordChars = 0;
			lineChars = 0;
			wordLen = 0;
			lineLen = 0;
			/*if (line == maxLines) {
				if (!srealloc(breaks, maxLines * 2 * sizeof(int))) continue;
				maxLines *= 2;
			}
			breaks[line] = pos;
			line ++;
			//*/
			continue;
		}
		CharData *d = GetCharData(f, c, b, bold);
		if (c == ' ' || c == '\t') {
			if (d) {
				lineLen += d->width + wordLen;
			}
			lineChars += wordChars + l;
			wordLen = 0;
			wordChars = 0;
			pos+=l;
		}
		else {
			int charLen;
			if (d) {
				charLen = d->width;
			}
			else {
				charLen = 0;
			}
			if (c == '(' || c == '[' || c == '{') {
				lineLen += wordLen;
				lineChars += wordChars;
				wordChars = 0;
				wordLen = 0;
			}
			if (wordLen + charLen + lineLen <= width || (!wordLen && !lineLen)) {
				if (c == '\\' || c == '>'  || c == '<' || c == ')' || c == ']' || c == '}' || c == '/' || c == ',' || c == '+' || c == '.' || c == '?' || c == '&' || c == '=') {
					lineLen += charLen + wordLen;
					lineChars += wordChars + l;
					wordChars = 0;
					wordLen = 0;
					pos += l;
				}
				else {
					wordChars += l;
					wordLen += charLen;
					pos += l;
				}
			}
			else {
				if (line == maxLines) {
					if (!srealloc(breaks, maxLines * 2 * sizeof(int))) continue;
					maxLines *= 2;
				}
				if (lineChars) {
					breaks[line] = pos-wordChars;
				}
				else {
					breaks[line] = pos;
					wordLen = 0;
					wordChars = 0;
				}
				lineLen = 0;
				lineChars = 0;
				line ++;
			}
		}
	}
	count = line;
	return breaks;
}

void FormatText(ScriptValue &s, ScriptValue *args) {
	int count;
	int *breaks = FormatText(args[0].stringVal->value, args[0].stringVal->len, args[1].i32, count);
	if (AllocateStringValue(s, args[0].stringVal->len + count)) {
		int out = 0;
		int i = 0;
		for (int p=0; p<args[0].stringVal->len; p++) {
			if (i < args[0].stringVal->len && breaks[i] == p) {
				s.stringVal->value[out++] = '\n';
				i++;
			}
			if (args[0].stringVal->value[p] != '\r' || args[0].stringVal->value[p+1] == '\n') {
				s.stringVal->value[out++] = args[0].stringVal->value[p];
			}
			else {
				s.stringVal->value[out++] = '\n';
			}
		}
	}
	free(breaks);
	/*
	Forma
	ScriptValue sv;
	AllocateStringValue(&sv, args[0].stringVal->len*2);
	*/
}

void SetBgColor(ScriptValue &s, ScriptValue *args) {
	CreateIntValue(s, activeScreen->bgColor.val);
	activeScreen->bgColor.val = args[0].i32;
}

void SetDrawColor(ScriptValue &s, ScriptValue *args) {
	CreateIntValue(s, activeScreen->drawColor.val);
	activeScreen->drawColor.val = args[0].i32;
}

void MyRGB(ScriptValue &s, ScriptValue *args) {
	Color4 c = {args[2].i32, args[1].i32, args[0].i32, 0xFF};
	CreateIntValue(s, c.val);
}

void MyRGBA(ScriptValue &s, ScriptValue *args) {
	Color4 c = {args[2].i32, args[1].i32, args[0].i32, args[3].i32};
	CreateIntValue(s, c.val);
}

void GetHDC(ScriptValue &s, ScriptValue *args) {
	CreateIntValue(s, (__int64)activeScreen->hDC);
}
