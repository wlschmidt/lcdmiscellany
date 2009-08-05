#ifndef IMAGE_H
#define IMAGE_H

#include "..\\ScriptValue.h"
#include <math.h>

struct BitImage {
	int width, height;
	unsigned int data[1];
};

struct AllocatedBitImage : public BitImage {
//protected:
	//inline ~AllocatedImage() {free(data);}
public:
	/*
	inline AllocatedImage(unsigned int* d, int w, int h) {
		data = d;
		width = w;
		height = h;
	}
	*/
};

AllocatedBitImage *MakeAllocatedBitImage(int w, int h);

struct Color4;
struct Color3 {
	unsigned char b,g,r;
	Color3 &operator=(const Color4 &);
};

struct Color4 {
	union {
		struct {unsigned char b,g,r,a;};
		unsigned int val;
		Color3 color3;
	};
};

__forceinline Color3& Color3::operator=(const Color4 &c4) {
	*this = c4.color3;
	return *this;
}


inline unsigned long FlipLong(unsigned char *c) {
	unsigned long s;
	((unsigned char*)&s)[0] = c[3];
	((unsigned char*)&s)[1] = c[2];
	((unsigned char*)&s)[2] = c[1];
	((unsigned char*)&s)[3] = c[0];
	return s;
}


inline unsigned short Flip(unsigned char *c) {
	unsigned short s;
	((unsigned char*)&s)[0] = c[1];
	((unsigned char*)&s)[1] = c[0];
	return s;
}

template <class T>
struct GenericImage {
	unsigned long width;
	unsigned long height;
	int spp;
	int memWidth;
	T *pixels;
	inline void Cleanup() {
		free(this);
	}
	void GetPixel(Color4 *c, int x, int y) const;
	// Assumes pixel is within image.
	void GetPixelBilinear(Color4 *c, float x, float y) const;
};

inline void GenericImage<unsigned char>::GetPixel(Color4 *c, int x, int y) const {
	unsigned char *px = &pixels[memWidth * y + x * spp]; 
	if (spp == 4) {
		*c = *(Color4*)px;
	}
	else if (spp == 3) {
		c->color3 = *(Color3*)px;
		c->a = 255;
	}
	else if (spp == 2) {
		c->r = c->g = c->b = px[0];
		c->a = px[1];
	}
	else if (spp == 1) {
		c->r = c->g = c->b = px[0];
		c->a = 255;
	}
}

inline void GenericImage<unsigned char>::GetPixelBilinear(Color4 *c, float x, float y) const {
	float x1, x2, y1, y2;
	int wx1, wx2, wy1, wy2;

	if (x < 0) x = 0;
	x1 = floor(x);
	x2 = x1+1.0f;
	if (x2 >= width) {
		x = x2 = (float)width-1.0f;
		x1 = x2 - 1;
	}
	wx1 = (int)(256*(x2 - x));
	wx2 = 256-wx1;

	if (y < 0) y = 0;
	y1 = floor(y);
	y2 = y1+1.0f;
	if (y2 >= height) {
		y = y2 = (float)height-1.0f;
		y1 = y2 - 1;
	}
	wy1 = (int)(256*(y2 - y));
	wy2 = 256 - wy1;

	unsigned char *px1y1 = &pixels[memWidth * (int)y1 + spp * (int)x1];
	unsigned char *px2y1 = px1y1 + spp;
	unsigned char *px1y2 = px1y1 + memWidth;
	unsigned char *px2y2 = px1y1 + memWidth+spp;
	Color4 *cx1y1 = (Color4*) px1y1;
	Color4 *cx2y1 = (Color4*) px2y1;
	Color4 *cx1y2 = (Color4*) px1y2;
	Color4 *cx2y2 = (Color4*) px2y2;

	if (spp >= 3) {
		int wx1y1 = wx1*wy1;
		int wx2y1 = wx2*wy1;
		int wx1y2 = wx1*wy2;
		int wx2y2 = wx2*wy2;
		c->r = (wx1y1 * cx1y1->r + wx2y1 * cx2y1->r + 
			    wx1y2 * cx1y2->r + wx2y2 * cx2y2->r + 128*256) / (256*256);
		c->g = (wx1y1 * cx1y1->g + wx2y1 * cx2y1->g + 
				wx1y2 * cx1y2->g + wx2y2 * cx2y2->g + 128*256) / (256*256);
		c->b = (wx1y1 * cx1y1->b + wx2y1 * cx2y1->b + 
				wx1y2 * cx1y2->b + wx2y2 * cx2y2->b + 128*256) / (256*256);
		if (spp == 3) {
			c->a = 255;
		}
		else {
			c->a = (wx1y1 * cx1y1->a + wx2y1 * cx2y1->a + 
					wx1y2 * cx1y2->a + wx2y2 * cx2y2->a + 128*256) / (256*256);
		}
	}
	else {
		c->r = c->g = c->b = (wy1 * (wx1 * px1y1[0] + wx2 * px2y1[0]) + 
							  wy2 * (wx1 * px1y2[0] + wx2 * px2y2[0]) + 128*256) / (256*256);
		if (spp == 1) {
			c->a = 255;
		}
		else {
			c->a = (wy1 * (wx1 * px1y1[1] + wx2 * px2y1[1]) + 
					wy2 * (wx1 * px1y2[1] + wx2 * px2y2[1]) + 128*256) / (256*256);
		}
	}
}
template <class T>
inline int MakeGenericImage(ScriptValue &s, unsigned int width, unsigned int height, int spp) {
	unsigned long memWidth = (width*spp+3)&~3;
	if (!AllocateCustomValue(s, 1+memWidth*height * sizeof(T) + sizeof(GenericImage<T>))) return 0;
	// The 1 allows writing beyond the end, which I do when loading pngs.
	//GenericImage<T> *out = (GenericImage<T>*) malloc((1+memWidth*height * sizeof(T) + ((sizeof(GenericImage<T>)+15))&~15) );
	//if (!out) return 0;
	GenericImage<T> *out = (GenericImage<T>*) s.stringVal->value;
	out->width = width;
	out->memWidth = memWidth;
	out->height = height;
	out->spp = spp;
	out->pixels = (T*) (((unsigned char*)out) + sizeof(GenericImage<T>));
	return 1;
}

template <class T>
inline int ResizeGenericImage(ScriptValue &s) {
	GenericImage<T> *out = (GenericImage<T>*) s.stringVal->value;
	int memWidth = (out->width*out->spp+3)&~3;
	if (!ResizeCustomValue(s, 1+memWidth*out->height * sizeof(T) + sizeof(GenericImage<T>))) return 0;
	out = (GenericImage<T>*) s.stringVal->value;
	out->memWidth = memWidth;
	//GenericImage<T> *out = (GenericImage<T>*) realloc(img, (img->memWidth*img->height * sizeof(T) + ((sizeof(GenericImage<T>)+15))&~15) );
	//if (!out) return img;
	out->pixels = (T*) (((unsigned char*)out) + sizeof(GenericImage<T>));
	return 1;
}

GenericImage<int> *MakeGenericImage(unsigned int width, unsigned int height, int spp);

// Note:  Safe to blindly call on 3 color images.
int Convert4ColorTo3Color(ScriptValue &src, ScriptValue &dst);

// Safe to call on 1-color images
int Convert2ColorTo1Color(ScriptValue &src, ScriptValue &dst);

int Convert3ColorTo1Color(ScriptValue &src, ScriptValue &dst);
//template <class T>
//inline GenericImage<T> * ResizeImage

//BitImage *Convert1ColorToBitImage(GenericImage<unsigned char> *img);

int Convert1ColorToBitImage(ScriptValue &src, ScriptValue &dest, int cutoff);

// May resize image.  Does nothing when passed a null pointer
int ConvertTo1ColorImage(ScriptValue &src, ScriptValue &dst);

//int Convert1ColorToBitImage(ScriptValue &s, GenericImage<unsigned char> *img, int cufoff);

int Zoom(ScriptValue &src, ScriptValue &dst, double scale);

int MakeAllocatedBitImage(ScriptValue &s, int w, int h);

int LoadBitImage(ScriptValue &s, unsigned char *path, double zoom, int cutoff);

//int LoadBitImage(unsigned char *path, double zoom);
int LoadMemoryColorImage(ScriptValue &sv, unsigned char *data, int len);
int LoadMemoryImage(ScriptValue &s, unsigned char *data, int len, double zoom, int cutoff);

int LoadColorImage(ScriptValue &s, unsigned char *path);

#endif
