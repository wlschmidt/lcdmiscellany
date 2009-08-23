#include "ScriptObjects/ScriptFont.h"
#include "Screen.h"
//#include "Fonts.h"
//#include "emmintrin.h"
#include "Unicode.h"
#include "sdk/v3.01/lglcd.h"
#include "stringUtil.h"
#include <math.h>

#ifdef X64
#pragma comment(lib, "sdk\\v3.01\\lgLcd64.lib")
#pragma comment(lib, "sdk\\v1.03\\lgLcd64.lib")
#else
#pragma comment(lib, "sdk\\v3.01\\lgLcd.lib")
#pragma comment(lib, "sdk\\v1.03\\lgLcd.lib")
#endif

Screen *activeScreen = 0;
Screen *dummyScreen = 0;

//__declspec(align(16))
//lgLcdBitmap160x43x1 lcdbmp;
/*struct {
#ifndef NOGDI
	int junk[3];
#endif
	lgLcdBitmap160x43x1 lcdbmp;
} ;
//*/

void Screen::SetClipRect(RECT *r) {
	if (r) {
		clipRect = *r;
		if (clipRect.left < 0) clipRect.left = 0;
		if (clipRect.top < 0) clipRect.top = 0;
		if (clipRect.bottom >= height) clipRect.bottom = height-1;
		if (clipRect.right >= width) clipRect.right = width-1;
	}
	else {
		clipRect.left = clipRect.top = 0;
		clipRect.bottom = height-1;
		clipRect.right = width-1;
	}
}

Screen::Screen(int width, int height, int bpp) {
	// Transparency value means I'll set them first time I draw.
	activeTextColor.val = 0xFF000000;
	activeBgColor.val = 0xFFFFFFFF;

	activeBgMode = OPAQUE;

	bgColor.val = 0xFFFFFFFF;
	drawColor.val = 0xFF000000;
	this->width = width;
	this->height = height;
	this->pixelCount = width*height;
	this->bpp = bpp;
	activeNewFont = internalFonts[0];
	internalFonts[0]->AddRef();

	HDC hDCDesktop = GetDC(0);
	if (!hDCDesktop) exit(0);
	BITMAPINFO bmi;
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = -height-1;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB; 
	bmi.bmiHeader.biSizeImage = 0;
	bmi.bmiHeader.biXPelsPerMeter = 0;
	bmi.bmiHeader.biYPelsPerMeter = 0;
	bmi.bmiHeader.biClrUsed = 0;
	bmi.bmiHeader.biClrImportant = 0;
	HBITMAP hBmp = CreateDIBSection(hDCDesktop, &bmi, DIB_RGB_COLORS, (void **)&image, NULL, 0);
	memset(image, 0xFF, sizeof(Color4)*width*height);
	hOldFont = 0;
	SetClipRect();
	if (hBmp) {
		hDC = CreateCompatibleDC(hDCDesktop);
		if (hDC) {
			hOldBmp = (HBITMAP)SelectObject(hDC, hBmp);
			if (hOldBmp) {
				return;
			}
			DeleteObject(hDC);
		}
		DeleteObject(hDC);
		exit(0);
	}
}

Screen::~Screen() {
	if (hOldFont) SelectObject(hDC, hOldFont);
	DeleteObject(SelectObject(hDC, hOldBmp));
	DeleteDC(hDC);
	activeNewFont->Release();
}

//odbc32.lib odbccp32.lib 
#ifndef NOGDI
#include <stdio.h>
#endif
// Clips r to fit in r2.  No branches.
void Screen::ClipRect(RECT &r, const RECT &r2) {
	int w = r.left - r2.left;
	r.left -= (w>>31) & w;

	w = r.top - r2.top;
	r.top -= (w>>31) & w;

	w = r2.right - r.right;
	r.right += (w>>31) & w;

	w = r2.bottom - r.bottom;
	r.bottom += (w>>31) & w;
}

// Clips r to fit in screen.
__forceinline void Screen::ClipRect(RECT &r) {
	ClipRect(r, clipRect);
	/*
	r.left &= (-r.left)>>31;
	r.top &= (-r.top)>>31;

	int w = width - 1 - r.right;
	r.right += (w>>31) & w;

	w = height - 1 - r.bottom;
	r.bottom += (w>>31) & w;
	//*/
}

/*int Screen::Init() {
	Cleanup();
	activeOldFont = font_04b08;
	//activeNewFont = 0;
	return 1;
	/*
#ifdef NOGDI
	//bmp.data = (unsigned int*) malloc(160/8*43);
	//if (bmp.data)
#else
	hDC = CreateCompatibleDC(0);
	if (hDC) {
		//hFont = CreateFontW(10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, L"Small Fonts");
		hFont = CreateFontW(8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, L"04b03");
		//hFont = CreateFontW(6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, L"04b24");
		//hFont = CreateFontW(-9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, L"Microsoft Sans Serif");
		if (!hFont) {
			Cleanup();
			return 0;
		}
		struct {
			BITMAPINFOHEADER bmiHeader;
			RGBQUAD bmiColors[2];
		} info;
		//memset(&info, 0, sizeof(info));
		info.bmiHeader.biSize = sizeof(info.bmiHeader);
		info.bmiHeader.biWidth = 160;
		info.bmiHeader.biHeight = -43;
		info.bmiHeader.biPlanes = 1;
		info.bmiHeader.biBitCount = 1;
		info.bmiHeader.biCompression = BI_RGB;
		info.bmiHeader.biSizeImage = BI_RGB;
		info.bmiHeader.biXPelsPerMeter = 3200;
		info.bmiHeader.biYPelsPerMeter = 3200;
		info.bmiHeader.biClrUsed = 2;
		info.bmiHeader.biClrImportant = 2;
		info.bmiColors[0].rgbRed =
			info.bmiColors[0].rgbGreen =
			info.bmiColors[0].rgbBlue = 0;
		info.bmiColors[1].rgbRed =
			info.bmiColors[1].rgbGreen =
			info.bmiColors[1].rgbBlue = 255;

		hBitMap = CreateDIBSection(hDC, (BITMAPINFO*)&info, DIB_RGB_COLORS, (void**)&bmp.data, 0, 0);
		//int w = GetLastError();
		if (hBitMap && InitFont()) {
			activeFont = &font;
			return 1;
		}
	}
#endif

	Cleanup();
	return 0;
	//*/
//}

#ifndef NOGDI
int Screen::InitFont() {

	if (!BeginDraw()) return 0;
	SetFont();
	GetTextMetrics(hDC, &font);
	ABC abc[256];
	int w = GetCharABCWidths(hDC,0, 255, abc);
	if (!w) {
		w = GetCharWidth(hDC, 0, 255, (int*)abc);
		if (!w) return 0;
		for (int i = 255; i>=0; i--) {
			abc[i].abcB = ((int*)abc)[i];
			abc[i].abcC = abc[i].abcA = 0;
		}
	}
	int i;
	int p = 0;
	//for (i=255; i>=0; i--) {
	for (i=0; i<=255; i++) {
		font.chars[i].start = p;
		font.chars[i] = abc[i];
		p += font.chars[i].abcB;
	}

	font.img = MakeAllocatedImage(p, font.tmHeight);
	memset(font.img->data, 0, ((p+31)/32) * font.tmHeight * sizeof(int));
	for (i=0; i<256; i++) {
		font.chars[i].top = 0;
		font.chars[i].bottom = (char)(font.tmHeight-1);
		char c = (char)i;
		TextOut(hDC, -font.chars[i].abcA, 0, &c, 1);
		int width = font.chars[i].abcB;
		GdiFlush();
		FlipBits(0,0, width, font.tmHeight);
		for (int h=0; h<font.tmHeight; h++) {
			int row = h * ((font.img->width+31)/32) + font.chars[i].start/32;
			for (int j=0; j<(width+31)/32; j++) {
				if (bmp.data[h*(160/32) + j]) {
					font.chars[i].bottom = h;
					if (h < font.chars[i].top)
						font.chars[i].top = h;
				}
				font.img->data[row+j] |= 
					(bmp.data[h*(160/32) + j] << (font.chars[i].start & 31));
				if (font.chars[i].start & 31)
					font.img->data[row+j+1] |= 
						(bmp.data[h*(160/32) + j] >> (32-(font.chars[i].start & 31)));
			}
		}
		Clear();
	}

	/*

	char *s = "04b03";
	FILE *outf = fopen("Fonts.h", "ab");
	fprintf(outf, "extern Font *font_%s;\n", s);
	fclose(outf);

	outf = fopen("Fonts.cpp", "ab");
	//fprintf(outf, "#include \"Images.h\"\n");
	fprintf(outf, "unsigned int f%s%s[] = {", s, s);
	w = 0;
	for (i=0; i<sizeof(Font)/4; i++) {
		if (w==32) {
			w = 0;
			fprintf(outf, "\n");
		}
		w++;
		if (((unsigned int*)&font)+i == (unsigned int*)&font.img) {
			fprintf(outf, "(unsigned int) (f%s%s+%u)", s, s, sizeof(Font)/4);
		}
		else
			fprintf(outf, "%u", ((unsigned int*)&font)[i]);
		fprintf(outf, ", ");
	}
	for (i=0; i<3; i++) {
		if (w==32) {
			w = 0;
			fprintf(outf, "\n");
		}
		w++;
		if (i==2) {
			fprintf(outf, "(unsigned int) (f%s%s+%u)", s, s, sizeof(Font)/4 + 3);
		}
		else
			fprintf(outf, "%u", ((unsigned int*)font.img)[i]);
		fprintf(outf, ", ");
	}
	for (i=0; i<((font.img->width+31)/32)*font.tmHeight; i++) {
		if (w==32) {
			w = 0;
			fprintf(outf, "\n");
		}
		w++;
		fprintf(outf, "%u", font.img->data[i]);
		if (i <((font.img->width+31)/32)*font.tmHeight-1)
			fprintf(outf, ", ");
	}
	fprintf(outf, "};\n", s);
	//fprintf(outf, "unsigned char %s%s = {", s, s);
	fprintf(outf, "Font *font_%s = (Font *)f%s%s;\n", s,s,s);

	fclose(outf);
	//*/
	//delete font.img;
	EndDraw();
	return 1;
}

void Screen::SetFont() {
	if (!hPrevFont) {
		BeginGDI();
		hPrevFont = (HFONT) SelectObject(hDC, hFont);
		prevTextColor = SetTextColor(hDC, RGB(255, 255, 255));
		prevBkColor = SetBkColor(hDC, RGB(0, 0, 0));
		prevMapMode = SetMapMode(hDC, MM_TEXT);
	}
}
#endif



void Screen::SetFont(ObjectValue *obj, ScriptValue &s) {
	/*if (activeOldFont) {
		CreateIntValue(s, activeOldFont == font_04b03);
		activeOldFont = 0;
	}
	else {//*/
		//activeNewFont->AddRef();
	CreateTypeValue(s, SCRIPT_OBJECT, activeNewFont);
	activeNewFont = obj;
	DynamicFont *f = GetNewFontMetrics(activeNewFont);
	if (f->hFont) {
		HFONT temp;
		temp = (HFONT)SelectObject(hDC, f->hFont);
		if (!hOldFont) hOldFont = temp;
	}
	else {
		if (hOldFont) {
			SelectObject(hDC, hOldFont);
			hOldFont = 0;
		}
	}
	return;
}

/*void Screen::SetFont(int i, ScriptValue &s) {
	OldFont *f;
	if (i == 1) {
		f = font_04b03;
	}
	else if (i == 0) {
		f = font_04b08;
	}
	else return;
	if (activeOldFont) {
		CreateIntValue(s, activeOldFont == font_04b03);
	}
	else {
		//activeNewFont->AddRef();
		CreateTypeValue(s, SCRIPT_OBJECT, activeNewFont);
		activeNewFont = 0;
	}
	activeOldFont = f;
	return;
}//*/


const long &Screen::GetFontHeight() {
	//if (activeOldFont) return activeOldFont->tmHeight;
	//else {
	TEXTMETRIC *tm = GetNewFontMetrics(activeNewFont);
	return tm->tmHeight;
	//}
}
int Screen::GetTextWidth(unsigned char *s, int len, int *height) {
	//if (activeOldFont) return activeOldFont->GetWidth(s, len, height);
	//else
	return GetWidth(activeNewFont, s, len, height);
	//return activeFont->GetWidth(s, height);
	/*int i = 0;
	while (*s) {
		i += activeFont->chars[s[0]].width;
		s ++;
	}
	return i;
	//*/
}

/*int Screen::GetTextWidth(wchar_t *w){
	int i = 0;
	unsigned char *s = UTF16toUTF8Alloc(w);
	if (s) {
		while (*s) {
			i += activeFont->chars[((unsigned char*)s)[0]].width;
			s ++;
		}
		free(s);
	}
	return i;
}//*/


void Screen::ColorPixel(unsigned int x, unsigned int y, Color4 color) {
	if (x - clipRect.left < (unsigned int)clipRect.right && y - clipRect.top < (unsigned int)clipRect.bottom) {
		image[y * width + x] = color;
	}
}

void Screen::TogglePixel(unsigned int x, unsigned int y) {
	if (x - clipRect.left < (unsigned int)clipRect.right && y - clipRect.top < (unsigned int)clipRect.bottom) {
		image[y * width + x].val ^= 0xFFFFFFFF;
	}
}

__forceinline void Screen::TogglePixelFast(unsigned int x, unsigned int y) {
	if (x < (unsigned int)width && y < (unsigned int)pixelCount) {
		image[y + x].val ^= 0xFFFFFFFF;
	}
}

__forceinline void Screen::ColorPixelFast(unsigned int x, unsigned int y, Color4 color) {
	if (x < (unsigned int)width && y < (unsigned int)pixelCount) {
		image[y + x] = color;
	}
}

__forceinline void Screen::AlphaColorPixelFast(unsigned int x, unsigned int y, Color4 color) {
	if (x < (unsigned int)width && y < (unsigned int)pixelCount) {
		AlphaColorPixel(y+x, color);
	}
}


void Screen::InvertLine(int x, int y, int x2, int y2) {
	int dx = x2 - x;
	int dy = y2 - y;
	int stepx = 1 | (dx>>31);
	dx = ((dx ^ (dx>>31)) - (dx >> 31))*2;

	int stepy = width * (1 | (dy>>31));
	dy = ((dy ^ (dy>>31)) - (dy >> 31))*2;

	y *= width;
	y2 *= width;

	TogglePixelFast(x, y);
	if (dx > dy) {
		int frac = dy - (dx >> 1);
		while (x != x2) {
			if (frac >= 0) {
				y += stepy;
				frac -= dx;
			}
			x += stepx;
			frac += dy;
			TogglePixelFast(x, y);
		}
	}
	else {
		int frac = dx - (dy >> 1);
		while (y != y2) {
			if (frac >= 0) {
				x += stepx;
				frac -= dy;
			}
			y += stepy;
			frac += dx;
			TogglePixelFast(x, y);
		}
	}
}

void Screen::ColorLine(int x, int y, int x2, int y2, Color4 color) {
	int dx = x2 - x;
	int dy = y2 - y;
	int stepx = 1 | (dx>>31);
	dx = ((dx ^ (dx>>31)) - (dx >> 31))*2;

	int stepy = width * (1 | (dy>>31));
	dy = ((dy ^ (dy>>31)) - (dy >> 31))*2;

	y *= width;
	y2 *= width;

	if (color.a == 0xFF) {
		ColorPixelFast(x, y, color);
		if (dx > dy) {
			int frac = dy - (dx >> 1);
			while (x != x2) {
				if (frac >= 0) {
					y += stepy;
					frac -= dx;
				}
				x += stepx;
				frac += dy;
				ColorPixelFast(x, y, color);
			}
		}
		else {
			int frac = dx - (dy >> 1);
			while (y != y2) {
				if (frac >= 0) {
					x += stepx;
					frac -= dy;
				}
				y += stepy;
				frac += dx;
				ColorPixelFast(x, y, color);
			}
		}
	}
	else {
		AlphaColorPixelFast(x, y, color);
		if (dx > dy) {
			int frac = dy - (dx >> 1);
			while (x != x2) {
				if (frac >= 0) {
					y += stepy;
					frac -= dx;
				}
				x += stepx;
				frac += dy;
				ColorPixelFast(x, y, color);
			}
		}
		else {
			int frac = dx - (dy >> 1);
			while (y != y2) {
				if (frac >= 0) {
					x += stepx;
					frac -= dy;
				}
				y += stepy;
				frac += dx;
				AlphaColorPixelFast(x, y, color);
			}
		}
	}
}

void NormalizeRect(RECT &r) {
	if (r.right < r.left) {
		int temp = r.left;
		r.left = r.right;
		r.right = temp;
	}
	if (r.top > r.bottom) {
		int temp = r.top;
		r.top = r.bottom;
		r.bottom = temp;
	}
}

void Screen::InvertRect(RECT &r) {
	Color4 c;
	c.val = 0xFFFFFFFF;
	XorRect(r, c);
}

void Screen::XorRect(RECT &r, Color4 c) {
	NormalizeRect(r);
	ClipRect(r);
	if (r.left>r.right || r.bottom<r.top) return;
	int starty = r.top * width;
	int endx = starty + r.right;
	starty += r.left;
	int endy = (r.bottom+1) * width;
	while (starty < endy) {
		int pos = starty;
		while (pos <= endx) {
			image[pos++].val ^= c.val;
		}
		starty += width;
		endx += width;
	}
}

void Screen::FillRect(RECT &r, Color4 c) {
	NormalizeRect(r);
	ClipRect(r);
	if (r.left>r.right || r.bottom<r.top) return;
	int starty = r.top * width;
	int endx = starty + r.right;
	starty += r.left;
	int endy = (r.bottom+1) * width;
	if (c.a == 0xFF) {
		while (starty < endy) {
			int pos = starty;
			while (pos <= endx) {
				image[pos++] = c;
			}
			starty += width;
			endx += width;
		}
	}
	else {
		while (starty < endy) {
			int pos = starty;
			while (pos <= endx) {
				AlphaColorPixel(pos, c);
				pos++;
			}
			starty += width;
			endx += width;
		}
	}
}

void Screen::ClearRect(RECT &r) {
	FillRect(r, bgColor);
}

void Screen::DrawRect(RECT &r) {
	FillRect(r, drawColor);
}


void Screen::DisplayText(int x, int y, wchar_t *text, int len, int highlight, int mode) {
	unsigned char temp [1000];
	if (len < 0) {
		len = (int) wcslen(text);
	}
	if (len >= 1000) len = 999;
	temp[999] = 0;
	UTF16toASCII(temp, text, len);
	DisplayText(x, y, temp, len, highlight, mode);
}


int Screen::DisplayTextRight(int x, int y, unsigned char *text, int len, int highlight, int mode) {
	//if (!activeFont) return 0;
	int junk;
	int pos = 0;
	int width = 0;
	int height = 0;
	while (pos < len) {
		while(pos<len && (text[pos] == ' ' || text[pos] == '\t')) pos++;
		if (pos == len) break;
		int start = pos;
		while (pos<len && text[pos] != '\r' && text[pos] != '\n') {
			pos++;
		}
		int temp = pos;
		while (temp > start && (text[temp-1] == ' ' || text[temp-1] == '\t')) temp--;
		int width2 = GetTextWidth(text+start, temp-start, &junk);
		if (width2 > width) width = width2;
		DisplayText(x-width2+1, y, text+start, temp-start, highlight, mode);
		pos ++;
		if (pos<len && text[pos] == '\n' && text[pos-1] == '\r') pos++;
		height += junk;
		y+=junk;
	}
	return width;
}

int Screen::DisplayTextCentered(int x, int y, unsigned char *text, int len, int highlight, int mode) {
	int junk;
	int pos = 0;
	int width = 0;
	int height = 0;
	while (pos < len) {
		while(pos<len && (text[pos] == ' ' || text[pos] == '\t')) pos++;
		if (pos == len) break;
		int start = pos;
		while (pos<len && text[pos] != '\r' && text[pos] != '\n') {
			pos++;
		}
		int temp = pos;
		while (temp > start && (text[temp-1] == ' ' || text[temp-1] == '\t')) temp--;
		int width2 = GetTextWidth(text+start, temp-start, &junk);
		if (width2 > width) width = width2;
		DisplayText(x-width2/2+1, y, text+start, temp-start, highlight, mode);
		pos ++;
		if (pos<len && text[pos] == '\n' && text[pos-1] == '\r') pos++;
		height += junk;
		y+=junk;
	}
	return width;
}

void FlipBR (Color4 *c) {
	unsigned char temp = c->r;
	c->r = c->b;
	c->b = temp;
}

void Screen::DisplayText(int x, int y, unsigned char *text, int len, int highlight, int mode, RECT *clip, int flags) {
	RECT oldClip;
	if (clip && !(flags & DT_NOCLIP)) {
		oldClip = clipRect;
		ClipRect(clipRect, *clip);
	}
	//if (!activeFont) return;
	if (len == -1) len = (int)strlen((char*)text);
	int xstart = x;
	int height;
	DynamicFont *f = GetNewFontMetrics(activeNewFont);
	if (f->hFont) {
		int slen = len;
		wchar_t *temp = UTF8toUTF16Alloc(text, &slen);
		if (!temp) return;
		RECT r = {x, y, this->width, this->height};
		if (clip) r = *clip;
		int useBgMode = TRANSPARENT;
		Color4 useTextColor = drawColor;
		Color4 useBgColor = bgColor;

		if (mode < 0) {
			useTextColor = bgColor;
			useBgColor = drawColor;
		}
		FlipBR(&useTextColor);
		FlipBR(&useBgColor);
		useTextColor.val &= 0xFFFFFF;
		useBgColor.val &= 0xFFFFFF;

		/*if (highlight) {
			bgMode = OPAQUE;
		}//*/
		if (useBgMode != activeBgMode) {
			SetBkMode(hDC, activeBgMode = useBgMode);
		}
		if (useTextColor.val != activeTextColor.val) {
			SetTextColor(hDC, activeTextColor.val=useTextColor.val);
		}
		if (useBgMode != TRANSPARENT && useBgColor.val != activeBgColor.val) {
			SetBkColor(hDC, activeBgColor.val=useBgColor.val);
		}
		DrawTextW(hDC, temp, slen, &r, (flags & DT_NOCLIP) | DT_NOPREFIX | DT_EXPANDTABS);
		free(temp);
		GdiFlush();
	}
	else {
		height = f->tmHeight;

		int bold = 0;
		while (len > 0) {
			if (y<-height) {
				y += height;
				while (len>0 && text[0] != '\n' && text[0] != '\r') {
					len--;
					text++;
				}
				len--;
				text++;
				if (len  && text[0]== '\n' && text[-1] == '\r') {
					len--;
					text++;
				}
				continue;
			}
			if (y >= this->height) return;
			x = xstart;
			int start = x;
			int end = x;
			/*if (activeOldFont) {
				if (removeFirstSpace) start -= activeOldFont->chars[text[0]].abcA;
				//x -= font.chars[text[0]].abcA;
				// Before edge
				while (x + activeOldFont->chars[text[0]].width - activeOldFont->chars[text[0]].abcC < 0 &&
					len > 0 && text[0] != '\n' && text[0] != '\r') {

					x += activeOldFont->chars[text[0]].width;
					text++;
					len--;
				}
				while (len > 0 && text[0] != '\n' && text[0] != '\r') {
					if (mode == 0)
						DisplayChar(x, y, &activeOldFont->chars[text[0]], &activeOldFont->img);
					else if (mode > 0)
						InvertChar(x, y, &activeOldFont->chars[text[0]], &activeOldFont->img);
					else if (mode < 0)
						ClearChar(x, y, &activeOldFont->chars[text[0]], &activeOldFont->img);
					x+=activeOldFont->chars[text[0]].width;

					end = x;
					// ran off edge
					if (x >= 160) break;
					text+=1;
					len--;
				}
				if (x >= 160) {
					while (len && text[0] != '\n' && text[0] != '\r') {
						len--;
						text++;
					}
				}
				len--;
				text++;
				//while (--len);
				if (highlight) {
					//if (!text[0]) end-= activeFont->chars[text[-1]].abcC;
					InvertRect(start-1, y, end, y+activeOldFont->tmHeight);
				}
			}
			else {//*/
				BitImage *img;
				int charLen;
				unsigned long l = NextUnicodeChar(text, &charLen);
				//text += charLen;
				//len -= charLen;
				CharData *c = GetCharData(f, l, img, bold);
				//if (removeFirstSpace) start -= c->abcA;
				//x -= font.chars[text[0]].abcA;
				// Before edge
				while (len > 0 && x + c->width - c->abcC < 0 && l != '\n' && l != '\r') {
					if (l == 2) {
						bold ^= 1;
					}

					x += c->width;
					//do {
						text += charLen;
						len -= charLen;
						l = NextUnicodeChar(text, &charLen);
						c = GetCharData(f, l, img, bold);
					//}
					//while (!c && len > 0);
				}
				while (len > 0 && l != '\n' && l != '\r') {
					if (l == 2) {
						bold ^= 1;
					}
					else {
						if (mode == 0)
							DisplayChar(x, y, c, img);
						else if (mode > 0)
							InvertChar(x, y, c, img);
						else if (mode < 0)
							ClearChar(x, y, c, img);
						x += c->width;

						end = x;
						// ran off edge
						if (x + c->abcA >= this->width) break;
					}
					//do {
					text += charLen;
					len -= charLen;
					l = NextUnicodeChar(text, &charLen);
					c = GetCharData(f, l, img, bold);
					//}
					//while (!c && len > 0);
				}
				if (len) {
					while (len && text[0] != '\n' && text[0] != '\r') {
						if (text[0] == 2) {
							bold ^= 1;
						}
						len--;
						text++;
					}
				}
				len--;
				text++;
				if (len>0 && text[0] == '\n' && text[-1] == '\r') {
					len--;
					text++;
				}
				//while (--len);
				if (highlight) {
					//if (!text[0]) end-= activeFont->chars[text[-1]].abcC;
					InvertRect(start-1, y, end, y+f->tmHeight);
				}
			//}
			y += height;
		}
	}
	if (clip && !(flags & DT_NOCLIP)) {
		clipRect = oldClip;
	}
}

__forceinline void Screen::DisplayChar(int dstx, int dsty, CharData* c, BitImage *img) {
	DisplayImage(dstx+c->abcA, dsty+c->top, c->start, c->top-c->inc, c->abcB, c->bottom - c->top+1, img);
}

__forceinline void Screen::ClearChar(int dstx, int dsty, CharData* c, BitImage *img) {
	ClearImage(dstx+c->abcA, dsty+c->top, c->start, c->top-c->inc, c->abcB, c->bottom - c->top+1, img);
}

__forceinline void Screen::InvertChar(int dstx, int dsty, CharData* c, BitImage *img) {
	InvertImage(dstx+c->abcA, dsty+c->top, c->start, c->top-c->inc, c->abcB, c->bottom - c->top+1, img);
}

void Screen::DisplayTransformedImageTriangle(DoublePoint *dst, DoublePoint *src, const GenericImage<unsigned char> *img) {
	DoublePoint d1, d2, d3, s1, s2, s3;
	int base;
	if (dst[0].y <= dst[1].y && dst[0].y <= dst[2].y) {
		base = 0;
	}
	else if (dst[1].y <= dst[2].y) {
		base = 1;
	}
	else {
		base = 2;
	}
	d1 = dst[base];
	s1 = src[base];

	if (dst[(base+1)%3].y <= dst[(base+2)%3].y) {
		d2 = dst[(base+1)%3];
		s2 = src[(base+1)%3];

		d3 = dst[(base+2)%3];
		s3 = src[(base+2)%3];
	}
	else {
		d2 = dst[(base+2)%3];
		s2 = src[(base+2)%3];

		d3 = dst[(base+1)%3];
		s3 = src[(base+1)%3];
	}

	double dx1=0, dx2=0, dx3=0;

	double dsx1 = 0, dsx2 = 0, dsx3 = 0;
	double dsy1 = 0, dsy2 = 0, dsy3 = 0;

	if (d2.y - d1.y > 0.00001) {
		dx1 = (d2.x - d1.x)/(d2.y - d1.y);

		dsx1 = (s2.x - s1.x)/(d2.y - d1.y);
		dsy1 = (s2.y - s1.y)/(d2.y - d1.y);
	}
	if (d3.y - d1.y > 0.00001) {
		dx2 = (d3.x - d1.x)/(d3.y - d1.y);

		dsx2 = (s3.x - s1.x)/(d3.y - d1.y);
		dsy2 = (s3.y - s1.y)/(d3.y - d1.y);
	}
	if (d3.y - d2.y > 0.00001) {
		dx3 = (d3.x - d2.x)/(d3.y - d2.y);

		dsx3 = (s3.x - s2.x)/(d3.y - d2.y);
		dsy3 = (s3.y - s2.y)/(d3.y - d2.y);
	}
	d1.y -= 0.0000000001;
	d3.y += 0.0000000001;

	double y0 = ceil(d1.y);
	if (y0 < 0) y0 = 0;

	double xstart = d1.x + dx2 * (y0 - d1.y);
	double sxstart = s1.x + dsx2 * (y0 - d1.y);
	double systart = s1.y + dsy2 * (y0 - d1.y);

	double xstop  = d1.x + dx1 * (y0 - d1.y);
	double sxstop = s1.x + dsx1 * (y0 - d1.y);
	double systop = s1.y + dsy1 * (y0 - d1.y);

	if (xstart > xstop) {
		xstart += 0.0000001;
		xstop  -= 0.0000001;
	}
	else {
		xstart -= 0.0000001;
		xstop  += 0.0000001;
	}

	int y = (int) y0;
	double endy = min(d2.y, height-1);
	double endy2 = min(d3.y, height-1);
	int dx = 1, x0, x1;
	if (dx1 < dx2) dx = -1;
	for (;y <= endy; y++, xstart += dx2, sxstart += dsx2, systart += dsy2, xstop += dx1, sxstop += dsx1, systop += dsy1) {
		if (dx > 0) {
			x0 = (int)ceil(xstart);
			x1 = (int)ceil(xstop);
			if (x0 > x1) continue;
		}
		else {
			x0 = (int)floor(xstart);
			x1 = (int)floor(xstop);
			if (x0 < x1) continue;
		}
		if (x0<0) {
			if (x1 <= 0) continue;
			x0 = 0;
		}
		if (x0 >= width) {
			if (x1 >= width) continue;
			x0 = width-1;
		}
		if (x1 < -1) x1 = -1;
		if (x1 >= width) x1 = width;

		double dsx = 0, dsy = 0;
		if (xstop != xstart) {
			dsx = dx * (sxstop - sxstart) / (xstop-xstart);
			dsy = dx * (systop - systart) / (xstop-xstart);
		}
		double sx = sxstart + dsx * dx * (x0-xstart);
		double sy = systart + dsy * dx * (x0-xstart);

		for (int x=x0; x!=x1; x+=dx, sx += dsx, sy += dsy) {
			Color4 c;
			img->GetPixelBilinear(&c, (float)sx, (float)sy);
			AlphaColorPixel(width * y + x, c);
		}
	}

	xstop = d2.x + (dx3 * (y - d2.y));
	sxstop = s2.x + (dsx3 * (y - d2.y));
	systop = s2.y + (dsy3 * (y - d2.y));

	// dx might be wrong in case of flat top, so fix here.
	if (xstart > xstop) {
		xstop  -= 0.0000001;
		dx = -1;
	}
	else {
		xstop  += 0.0000001;
		dx = 1;
	}

	for (;y <= endy2; y++, xstart += dx2, sxstart += dsx2, systart += dsy2, xstop += dx3, sxstop += dsx3, systop += dsy3) {
		if (dx > 0) {
			x0 = (int)ceil(xstart);
			x1 = (int)ceil(xstop);
			if (x0 > x1) continue;
		}
		else {
			x0 = (int)floor(xstart);
			x1 = (int)floor(xstop);
			if (x0 < x1) continue;
		}
		if (x0<0) {
			if (x1 <= 0) continue;
			x0 = 0;
		}
		if (x0 >= width) {
			if (x1 >= width) continue;
			x0 = width-1;
		}
		if (x1 < -1) x1 = -1;
		if (x1 >= width) x1 = width;
		double dsx = 0, dsy = 0;
		if (xstop != xstart) {
			dsx = dx * (sxstop - sxstart) / (xstop-xstart);
			dsy = dx * (systop - systart) / (xstop-xstart);
		}
		double sx = sxstart + dsx * dx * (x0-xstart);
		double sy = systart + dsy * dx * (x0-xstart);
		for (int x=x0; x!=x1; x+=dx, sx += dsx, sy += dsy) {
			//if (x < 0 || x >= width || y < 0 || y >= height) {
			//	x=x;
			//}
			Color4 c;
			img->GetPixelBilinear(&c, (float)sx, (float)sy);
			AlphaColorPixel(width * y + x, c);
		}
	}
}

void Screen::DisplayTransformedImage(DoubleQuad *dst, DoubleQuad *src, const GenericImage<unsigned char> *img) {
	if (img->width < 2 || img->height < 2) return;
	{
		DoublePoint dstPt[3] = {
			dst->p[0],
			dst->p[1],
			dst->p[2]
		};
		DoublePoint srcPt[3] = {
			src->p[0],
			src->p[1],
			src->p[2]
		};
		DisplayTransformedImageTriangle(dstPt, srcPt, img);
	}
	{
		DoublePoint dstPt[3] = {
			dst->p[2],
			dst->p[3],
			dst->p[0]
		};
		DoublePoint srcPt[3] = {
			src->p[2],
			src->p[3],
			src->p[0]
		};
		DisplayTransformedImageTriangle(dstPt, srcPt, img);
	}
}

void Screen::DisplayImage(int dstx, int dsty, int srcx, int srcy, int width, int height, const GenericImage<unsigned char> *img) {
	int dx = dstx-srcx;
	int dy = dsty-srcy;

	RECT r2 = {0, 0, img->width - 1, img->height - 1};
	RECT r = {srcx, srcy, srcx+width-1, srcy+height-1};

	ClipRect(r, r2);

	r.left += dx;
	r.right += dx;
	r.top += dy;
	r.bottom += dy;

	ClipRect(r);

	if (((r.right - r.left) | (r.bottom - r.top)) < 0) return;
	{
		int srcWidth = (img->width * img->spp+3)&~3;
		int srcy = srcWidth * (r.top - dy) + (r.left - dx) * img->spp;
		int y = r.top * this->width;
		int endy = r.bottom * this->width;
		if (img->spp == 3) {
			while (y <= endy) {
				unsigned char *src = img->pixels + srcy;
				for (int x = r.left; x<=r.right; x++) {
					image[x+y].val = src[0] + (((int)src[1])<<8) + (((int)src[2])<<16) + 0xFF000000;
					src += 3;
				}
				srcy += srcWidth;
				y += this->width;
			}
		}
		else if (img->spp == 1) {
			while (y <= endy) {
				unsigned char *src = img->pixels + srcy;
				for (int x = r.left; x<=r.right; x++) {
					image[x+y].val = src[0] + (((int)src[0])<<8) + (((int)src[0])<<16) + 0xFF000000;
					src ++;
				}
				srcy += srcWidth;
				y += this->width;
			}
		}
		else if (img->spp == 4) {
			while (y <= endy) {
				Color4* src = (Color4*) (img->pixels + srcy);
				for (int x = r.left; x<=r.right; x++) {
					AlphaColorPixel(x+y, *src);
					src ++;
				}
				srcy += srcWidth;
				y += this->width;
			}
		}
		else if (img->spp == 2) {
			while (y <= endy) {
				unsigned char *src = img->pixels + srcy;
				for (int x = r.left; x<=r.right; x++) {
					AlphaColorPixelBW(x+y, src[1], src[0]);
					/*
					unsigned short alpha = src[1];
					unsigned short nalpha = 255-src[1];
					unsigned char srcColor = alpha * src[0];
					image[x+y].r = (nalpha * image[x+y].r + srcColor+1)/255;
					image[x+y].g = (nalpha * image[x+y].g + srcColor+1)/255;
					image[x+y].b = (nalpha * image[x+y].b + srcColor+1)/255;
					image[x+y].a = (nalpha * image[x+y].a + alpha*alpha+1)/255;
					//*/
					src += 2;
				}
				srcy += srcWidth;
				y += this->width;
			}
		}
	}
}

void Screen::DisplayImage(int dstx, int dsty, int srcx, int srcy, int width, int height, const BitImage *img) {
	int dx = dstx-srcx;
	int dy = dsty-srcy;

	RECT r2 = {0, 0, img->width - 1, img->height - 1};
	RECT r = {srcx, srcy, srcx+width-1, srcy+height-1};

	ClipRect(r, r2);

	r.left += dx;
	r.right += dx;
	r.top += dy;
	r.bottom += dy;

	ClipRect(r);


	if (((r.right - r.left) | (r.bottom - r.top)) < 0) return;

	{
		int srcWidth = (img->width+31)/32;
		int srcy = srcWidth * (r.top - dy);
		int y = r.top * this->width;
		int endy = r.bottom * this->width;
		if (drawColor.a == 0xFF) {
			while (y <= endy) {
				for (int x = r.left; x<=r.right; x++) {
					int srcx = x - dx;
					image[x+y].val = drawColor.val + ((image[x+y].val-drawColor.val) & (((img->data[srcy + srcx/32] >> (srcx&31))&1)-1));
					//if (img->data[srcy + srcx/32] & (1 << (srcx&31)))
					//	SetPixelFast(x, y);
				}
				srcy += srcWidth;
				y += this->width;
			}
		}
		else {
			while (y <= endy) {
				for (int x = r.left; x<=r.right; x++) {
					int srcx = x - dx;
					if ((img->data[srcy + srcx/32] >> (srcx&31))&1) {
						AlphaColorPixel(x+y, drawColor);
					}
					//if (img->data[srcy + srcx/32] & (1 << (srcx&31)))
					//	SetPixelFast(x, y);
				}
				srcy += srcWidth;
				y += this->width;
			}
		}
	}
}

int Screen::IntersectImage(int dstx, int dsty, int srcx, int srcy, int width, int height, const BitImage *img) {
	int dx = dstx-srcx;
	int dy = dsty-srcy;
	RECT r;

	RECT r2 = {0, 0, img->width - 1, img->height - 1};

	r.left = srcx;
	r.right = srcx+width-1;
	r.top = srcy;
	r.bottom = srcy+height-1;

	ClipRect(r, r2);

	r.left += dx;
	r.right += dx;
	r.top += dy;
	r.bottom += dy;

	ClipRect(r);

	if (((r.right - r.left) | (r.bottom - r.top)) < 0) return 0;
	{
		int srcWidth = (img->width+31)/32;
		int srcy = srcWidth * (r.top - dy);
		int y = r.top * this->width;
		int endy = r.bottom * this->width;
		while (y <= endy) {
			for (int x = r.left; x<=r.right; x++) {
				int srcx = x - dx;
				if (img->data[srcy + srcx/32] & (1 << (srcx&31))) {
					if (image[x+y].val != bgColor.val) return 1;
				}
			}
			srcy += srcWidth;
			y += this->width;
		}
	}
	return 0;
}

void Screen::InvertImage(int dstx, int dsty, int srcx, int srcy, int width, int height, const BitImage *img) {
	int dx = dstx-srcx;
	int dy = dsty-srcy;
	RECT r;

	RECT r2 = {0, 0, img->width - 1, img->height - 1};

	r.left = srcx;
	r.right = srcx+width-1;
	r.top = srcy;
	r.bottom = srcy+height-1;

	ClipRect(r, r2);

	r.left += dx;
	r.right += dx;
	r.top += dy;
	r.bottom += dy;

	ClipRect(r);


	if (((r.right - r.left) | (r.bottom - r.top)) < 0) return;

	{
		int srcWidth = (img->width+31)/32;
		int srcy = srcWidth * (r.top - dy);
		int y = r.top * this->width;
		int endy = r.bottom * this->width;
		while (y <= endy) {
			for (int x = r.left; x<=r.right; x++) {
				int srcx = x - dx;
				image[x+y].val = image[x+y].val ^ ((0xFFFFFFFF) & (-(int)((img->data[srcy + srcx/32] >> (srcx&31))&1)));
				//if (img->data[srcy + srcx/32] & (1 << (srcx&31)))
				//	SetPixelFast(x, y);
			}
			srcy += srcWidth;
			y += this->width;
		}
	}
}

void Screen::ClearImage(int dstx, int dsty, int srcx, int srcy, int width, int height, const BitImage *img) {
	int dx = dstx-srcx;
	int dy = dsty-srcy;
	RECT r;

	RECT r2 = {0, 0, img->width - 1, img->height - 1};

	r.left = srcx;
	r.right = srcx+width-1;
	r.top = srcy;
	r.bottom = srcy+height-1;

	ClipRect(r, r2);

	r.left += dx;
	r.right += dx;
	r.top += dy;
	r.bottom += dy;

	ClipRect(r);


	if (((r.right - r.left) | (r.bottom - r.top)) < 0) return;

	{
		int srcWidth = (img->width+31)/32;
		int srcy = srcWidth * (r.top - dy);
		int y = r.top * this->width;
		int endy = r.bottom * this->width;
		while (y <= endy) {
			for (int x = r.left; x<=r.right; x++) {
				int srcx = x - dx;
				image[x+y].val = bgColor.val + ((image[x+y].val-drawColor.val) & (((img->data[srcy + srcx/32] >> (srcx&31))&1)-1));
				//if (img->data[srcy + srcx/32] & (1 << (srcx&31)))
				//	ClearPixelFast(x, y);
			}
			srcy += srcWidth;
			y += this->width;
		}
	}
}
