#ifndef IMAGE_H
#define IMAGE_H

#include "..\\ScriptValue.h"

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
};

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
