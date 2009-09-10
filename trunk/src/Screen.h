#include "global.h"
//#include <stdio.h>
//#include <stdlib.h>
//#include <xmmintrin.h>
#define NOGDI
#include "Images/image.h"

#ifndef SCREEN_H
#define SCREEN_H

/*struct FunkyBMP {
	unsigned char junk [156];
	lgLcdBitmap160x43x1 hcdbmp;
};


/*
#ifdef NOGDI
typedef struct tagTEXTMETRICA
{
    LONG        tmHeight;
    LONG        tmAscent;
    LONG        tmDescent;
    LONG        tmInternalLeading;
    LONG        tmExternalLeading;
    LONG        tmAveCharWidth;
    LONG        tmMaxCharWidth;
    LONG        tmWeight;
    LONG        tmOverhang;
    LONG        tmDigitizedAspectX;
    LONG        tmDigitizedAspectY;
    BYTE        tmFirstChar;
    BYTE        tmLastChar;
    BYTE        tmDefaultChar;
    BYTE        tmBreakChar;
    BYTE        tmItalic;
    BYTE        tmUnderlined;
    BYTE        tmStruckOut;
    BYTE        tmPitchAndFamily;
    BYTE        tmCharSet;
} TEXTMETRIC;
#endif
*/

struct OldFont;
struct CharData;

struct DoublePoint {
	double x, y;
};
struct DoubleQuad {
	DoublePoint p[4];
};

class Screen {
private:
public:
	void SetClipRect(RECT *r = 0);

	RECT clipRect;
	int width;
	int height;
	int bpp;
	int pixelCount;
	Color4 *image;
	Color4 bgColor;
	Color4 drawColor;

	Color4 activeTextColor;
	Color4 activeBgColor;
	int activeBgMode;

	ObjectValue *activeNewFont;

	HBITMAP hOldBmp;
	HDC hDC;
	HFONT hOldFont;

	// Ready to draw.
	bool ready;

	void Revert();

	//int Init();
	//int Check();

	//void FillRect(RECT &r, unsigned char color);
	void DisplayChar(int dstx, int dsty, CharData* c, BitImage *img);
	void InvertChar(int dstx, int dsty, CharData* c, BitImage *img);
	void ClearChar(int dstx, int dsty, CharData* c, BitImage *img);


	


	void Clear();

	inline void Invert() {
		for (int i=0; i<pixelCount; i++) {
			image[i].val ^= 0xFFFFFFFF;
		}
	}

	void InvertRect(RECT &r);

	inline void InvertRect(int x, int y, int x2, int y2) {
		RECT r = {x, y, x2, y2};
		InvertRect(r);
	}

	void ClearRect(RECT &r);
	inline void ClearRect(int x, int y, int x2, int y2) {
		RECT r = {x, y, x2, y2};
		ClearRect(r);
	}

	void DrawRect(RECT &r);
	inline void DrawRect(int x, int y, int x2, int y2) {
		RECT r = {x, y, x2, y2};
		DrawRect(r);
	}

	void XorRect(RECT &r, Color4 c);
	inline void XorRect(int x, int y, int x2, int y2, Color4 c) {
		RECT r = {x, y, x2, y2};
		XorRect(r, c);
	}

	inline void FillRect(RECT &r, Color4 c);

	inline void AlphaColorPixelBW (int index, unsigned int colora, unsigned int white) {
		unsigned int alpha = 256 - colora - (unsigned int)(colora >> 7);
		unsigned int wmul = white*256;

		image[index].r = (unsigned char)((wmul + (image[index].r-white) * alpha) >> 8);
		image[index].g = (unsigned char)((wmul + (image[index].g-white) * alpha) >> 8);
		image[index].b = (unsigned char)((wmul + (image[index].b-white) * alpha) >> 8);

		// ??
		image[index].a = (unsigned char)((256 * (unsigned int)image[index].a + (colora-image[index].a) * alpha) >> 8);
	}

	void ColorPixel(unsigned int x, unsigned int y, Color4 color);
	void TogglePixel(unsigned int x, unsigned int y);

	void TogglePixelFast(unsigned int x, unsigned int y);
	void ColorPixelFast(unsigned int x, unsigned int y, Color4 color);
	void AlphaColorPixelFast(unsigned int x, unsigned int y, Color4 color);

	//int Display(int device);

#ifndef NOGDI
	inline void DrawIcon(int x, int y, int dx, int dy, HICON hIcon) {
		BeginGDI();
		FlipBits(x, y, dx, dy);
		DrawIconEx(hDC, x, y, hIcon, dx, dy, 0, NULL, DI_NORMAL);
		GdiFlush();
		FlipBits(x, y, dx, dy);
	}
#endif
	int DisplayTextCentered(int x, int y, unsigned char *text, int len=-1, int highlight = 0, int mode = 0);
	inline int DisplayTextCentered(int x, int y, char *text, int len=-1, int highlight = 0, int mode = 0) {
		return DisplayTextCentered(x, y, (unsigned char *)text, len, highlight, mode);
	}

	int DisplayTextRight(int x, int y, unsigned char *text, int len=-1, int highlight = 0, int mode = 0);
	inline int DisplayTextRight(int x, int y, char *text, int len=-1, int highlight = 0, int mode = 0) {
		return DisplayTextRight(x, y, (unsigned char *)text, len, highlight, mode);
	}

	void DisplayText(int x, int y, unsigned char *text, int len, int highlight, int mode=0, RECT *clip=0, int flags=0);
	//void DisplayText(int x, int y, unsigned char *text, int len=-1, int highlight = 0, int removeFirstSpace = 1, int mode = 0, RECT *r=0);
	inline void DisplayText(int x, int y, char *text, int len=-1, int highlight = 0, int mode = 0) {
		DisplayText(x, y, (unsigned char*) text, len, highlight, mode);
	}

	void DisplayText(int x, int y, wchar_t *text, int len = -1, int highlight = 0, int mode = 0);
	/*inline void Screen::DisplayTextRight(int x, int y, char *text) {
		RECT r = {0, 1+y, x, 44};
		DrawText(hDC, text, strlen(text), &r, DT_RIGHT);
	}*/

	inline void DisplayImage(int dstx, int dsty, const BitImage *img) {
		DisplayImage(dstx, dsty, 0, 0, img->width, img->height, img);
	}

	void DisplayImage(int dstx, int dsty, int srcx, int srcy, int width, int height, const GenericImage<unsigned char> *img);

	void DisplayImage(int dstx, int dsty, int srcx, int srcy, int width, int height, const BitImage *img);
	void InvertImage(int dstx, int dsty, int srcx, int srcy, int width, int height, const BitImage *img);
	void ClearImage(int dstx, int dsty, int srcx, int srcy, int width, int height, const BitImage *img);

	void DisplayTransformedImageTriangle(DoublePoint *dst, DoublePoint *src, const GenericImage<unsigned char> *img);
	void DisplayTransformedImage(DoubleQuad *dst, DoubleQuad *src, const GenericImage<unsigned char> *img);

	int IntersectImage(int dstx, int dsty, int srcx, int srcy, int width, int height, const BitImage *img);

	//{
	//	TextOut(hDC, x, y+1, text, strlen(text));
	//}

	__forceinline void ClearLine(int x, int y, int x2, int y2) {
		ColorLine(x, y, x2, y2, bgColor);
	}

	__forceinline void DrawLine(int x, int y, int x2, int y2) {
		ColorLine(x, y, x2, y2, drawColor);
	}

	void InvertLine(int x, int y, int x2, int y2);
	void ColorLine(int x, int y, int x2, int y2, Color4 color);
	/*inline void DrawLine(int x, int y, int x2, int y2) {
		MoveToEx(hDC, x, y, NULL);
		LineTo(hDC, x2, y2);
	}*/
	/*inline void DoubleBox(RECT r) {
		r.left += 1;
		r.right -= 1;
		r.top += 1;
		r.bottom -= 1;

	}//*/
	void ClipRect(RECT &r);
	void ClipRect(RECT &r, const RECT &r2);

	inline void DoubleBox(int x, int y, int x2, int y2) {
		DrawRect(x,y,x2,y2);
		ClearRect(x+1,y+1,x2-1,y2-1);
		//Rect r = {x, y, x2, y2);
		//DoubleBox(r);
	}

	Screen(int width, int height, int bpp);
	virtual ~Screen();

	//inline ~Screen() {
	//}
	const long &GetFontHeight();
	int GetTextWidth(unsigned char *s, int len, int *height);
	inline int GetTextWidth(char *s, int len, int *height) {
		return GetTextWidth((unsigned char*)s, len, height);
	}
	//int GetTextWidth(wchar_t *s);
	void SetFont(ObjectValue *obj, ScriptValue &s);
};

extern Screen *activeScreen;
// Active screen when not drawing.  For font size ops.
extern Screen *dummyScreen;

#endif
