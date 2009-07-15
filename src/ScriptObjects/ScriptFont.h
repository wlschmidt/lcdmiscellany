#ifndef SCRIPT_FONTS_H
#define SCRIPT_FONTS_H

#include "../global.h"
#include "../ScriptValue.h"
#include "../Images/image.h"

struct CharData {
	unsigned short start :14;
	unsigned short inc : 2;
	char abcA, abcB, abcC, width, top, bottom;
	inline void operator =(const ABC &abc) {
		abcA = abc.abcA;
		abcB = abc.abcB;
		abcC = abc.abcC;
		width = abc.abcA + abc.abcB + abc.abcC;
		inc = 0;
	}
};

extern ObjectValue *internalFonts[1];
/*
struct OldFont : public TEXTMETRICA {
	CharData chars[256];
	BitImage img;

	inline int LineCount(unsigned char *text, int len = -1) {
		if (len < 0) {
			len = 0;
			while (text[len]) len++;
		}
		int line = 1;
		int lines = 1;
		for (int i=0; i<len; i++) {
			if (text[i] == '\n') {
				line++;
			} 
			else if (text[i] == '\r') {
				if (i < len-1 && text[i+1] == '\n') i++;
				line++;
			}
			else lines = line;
		}
		return lines;
	}

	inline int GetWidth(unsigned char *text, int len, int *height) {
		if (len < 0) {
			len = 0;
			while (text[len]) len++;
		}
		int tout = 0;
		*height = 1;
		int out = -chars[text[0]].abcA;
		for (int i=0; i<len; i++) {
			if (text[i] == '\r' || text[i] == '\n') {
				if (i) {
					out -= chars[text[i-1]].abcC;
					if (out < tout) {
						out = tout;
					}
				}
				if (len == 1) break;
				tout = -chars[text[1]].abcA;
				height[0] ++;
				if (text[i] == '\r' && text[i+1] == '\n') i++;
				continue;
			}
			tout += chars[text[i]].width;
		}
		*height *= tmHeight;
		if (len) {
			out -= chars[text[len-1]].abcC;
			if (out < tout) out = tout;
		}
		return out;
	}
	inline int GetWidth(char *text, int len, int *height) {
		return GetWidth((unsigned char *) text, len, height);
	}
};
//*/
struct DynamicFont;

struct CharBlock {
	CharData chars[128];
	BitImage *img;
	inline void Cleanup() {
		free(img);
	}
	int TryInit(int rangeStart, DynamicFont *f);
	int Init(int rangeStart, DynamicFont *f, HDC hDC);
};

struct CharBlockBlock {
	CharBlock *blocks[128];
	inline void Cleanup() {
		for (int i=0; i<128; i++) {
			if ((unsigned __int64)blocks[i]>1) {
				blocks[i]->Cleanup();
				free(blocks[i]);
			}
		}
	}
};

struct CharBlockBlockBlock {
	CharBlockBlock *blocks[128];
	inline void Cleanup() {
		for (int i=0; i<128; i++) {
			if ((unsigned __int64)blocks[i]>1) {
				blocks[i]->Cleanup();
				free(blocks[i]);
			}
		}
	}
};

struct DynamicFont : public TEXTMETRIC {
	CharBlock chars;
	CharBlockBlock *block;
	CharBlockBlockBlock *blockBlock;
	StringValue *name;

	DynamicFont *boldFont;
	ObjectValue *obj;

	HFONT hFont;

	int width;
	int height;
	short bold;
	short italics;
	BYTE quality;
	void Cleanup() {
		if (hFont) {
			DeleteObject(hFont);
		}
		if (obj) obj->Release();
		chars.Cleanup();
		if (((size_t)block)&~1) {
			block->Cleanup();
			free(block);
		}
		if (((size_t)blockBlock)&~1) {
			blockBlock->Cleanup();
			free(blockBlock);
		}
		name->Release();
	}
};

DynamicFont *GetNewFontMetrics(ObjectValue *o);
int GetWidth(ObjectValue *o, unsigned char *s, int length, int *height);

void Font(ScriptValue &s, ScriptValue *args);
//void GetFontVals(ScriptValue &s, ScriptValue *args);

void DeleteFont(ScriptValue &s, ScriptValue *args);
void UseFont(ScriptValue &s, ScriptValue *args);

extern unsigned int FontType;

CharData *GetCharData(DynamicFont *f, unsigned long l, BitImage* &b, int bold);
int GetWidth(ObjectValue *o, unsigned char *s, int length, int *height);

int InitFonts();
void CleanupFonts();

void GetFontHeight(ScriptValue &s, ScriptValue *args);

#endif
