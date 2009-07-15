#include "malloc.h"
#include <stdlib.h>
#include "Image.h"
#include "../stringUtil.h"

#include "../Screen.h"
#include "../Util.h"
#include "../unicode.h"
#include "Image.h"
#include "BMP.h"
#include "tga.h"
#include "jpg.h"
#include "gif.h"
#include "png.h"

/*
template <class T>
GenericImage<T> *MakeGenericImage(unsigned int width, unsigned int height, int spp);

template <class T>
GenericImage<T> *ResizeGenericImage(GenericImage<T> *img);
//*/

int MakeAllocatedBitImage(ScriptValue &s, int w, int h) {
	if (!AllocateCustomValue(s, sizeof(AllocatedBitImage) + (((w+31)/32)*h-1) * sizeof(int))) return 0;
	AllocatedBitImage *out = (AllocatedBitImage*) s.stringVal->value;
	out->height = h;
	out->width = w;
	//out->data = (unsigned int*) &(((unsigned char*)out)[sizeof(AllocatedBitImage)]);
	return 1;
}

AllocatedBitImage *MakeAllocatedBitImage(int w, int h) {
	AllocatedBitImage *out = (AllocatedBitImage*) malloc(sizeof(AllocatedBitImage) + (((w+31)/32)*h-1) * sizeof(int));
	if (!out) return 0;
	out->height = h;
	out->width = w;
	//out->data = (unsigned int*) &(((unsigned char*)out)[sizeof(AllocatedBitImage)]);
	return out;
}

int Convert4ColorTo3Color(ScriptValue &src, ScriptValue &dst) {
	GenericImage<unsigned char> *srcImage = (GenericImage<unsigned char> *)src.stringVal->value;
	if (srcImage->spp != 4) {
		dst = src;
		return 1;
	}
	GenericImage<unsigned char> *dstImage;
	if (src.stringVal->refCount == 1) {
		dst = src;
		dstImage = srcImage;
	}
	else {
		src.stringVal->refCount--;
		if (!MakeGenericImage<unsigned char>(dst, srcImage->width, srcImage->height, 3)) return 0;
		dstImage = (GenericImage<unsigned char> *)dst.stringVal->value;
	}
	unsigned long memWidth = (srcImage->width*3+3)&~3;
	for (unsigned int y=0; y<srcImage->height; y++) {
		unsigned char* src = &srcImage->pixels[srcImage->memWidth*y];
		unsigned char* dst = &dstImage->pixels[memWidth*y];
		for (unsigned int x=0; x<srcImage->width;x++,src+=4,dst+=3) {
			if (src[3] == 0xFF) {
				dst[0] = src[0];
				dst[1] = src[1];
				dst[2] = src[2];
			}
			else {
				dst[0] = (unsigned char) ((src[0]*((unsigned int)src[3]))/0xFF + (0xFF - src[3]));
				dst[1] = (unsigned char) ((src[1]*((unsigned int)src[3]))/0xFF + (0xFF - src[3]));
				dst[2] = (unsigned char) ((src[2]*((unsigned int)src[3]))/0xFF + (0xFF - src[3]));
			}
		}
	}
	if (dstImage == srcImage) {
		dstImage->spp = 3;
		if (!ResizeGenericImage<unsigned char>(dst)) {
			dst.Release();
			return 0;
		}
	}
	return 1;
}

int Convert2ColorTo1Color(ScriptValue &src, ScriptValue &dst) {
	GenericImage<unsigned char> *srcImage = (GenericImage<unsigned char> *)src.stringVal->value;
	if (srcImage->spp != 2) {
		dst = src;
		return 1;
	}
	GenericImage<unsigned char> *dstImage;
	if (src.stringVal->refCount == 1) {
		dstImage = srcImage;
	}
	else {
		src.stringVal->refCount--;
		if (!MakeGenericImage<unsigned char>(dst, srcImage->width, srcImage->height, 3)) return 0;
		dstImage = (GenericImage<unsigned char> *)dst.stringVal->value;
	}
	unsigned long memWidth = (srcImage->width+3)&~3;
	for (unsigned int y=0; y<srcImage->height; y++) {
		unsigned char* src = &srcImage->pixels[srcImage->memWidth*y];
		unsigned char* dst = &dstImage->pixels[memWidth*y];
		for (unsigned int x=0; x<srcImage->width;x++,src+=2,dst+=1) {
			if (src[1] == 0xFF) {
				dst[0] = src[0];
			}
			else {
				dst[0] = (unsigned char) ((src[0]*((unsigned int)src[1]))/0xFF + (0xFF - src[1]));
			}
		}
	}
	if (dstImage == srcImage) {
		dstImage->spp = 1;
		if (!ResizeGenericImage<unsigned char>(dst)) {
			dst.Release();
			return 0;
		}
	}
	return 1;
}

int Convert3ColorTo1Color(ScriptValue &src, ScriptValue &dst) {
	GenericImage<unsigned char> *srcImage = (GenericImage<unsigned char> *)src.stringVal->value;
	if (srcImage->spp != 3) {
		dst = src;
		return 1;
	}
	GenericImage<unsigned char> *dstImage;
	if (src.stringVal->refCount == 1) {
		dstImage = srcImage;
		dst = src;
	}
	else {
		src.stringVal->refCount--;
		if (!MakeGenericImage<unsigned char>(dst, srcImage->width, srcImage->height, 1)) return 0;
		dstImage = (GenericImage<unsigned char> *)dst.stringVal->value;
	}
	unsigned long memWidth = (srcImage->width+3)&~3;
	for (unsigned int y=0; y<srcImage->height; y++) {
		unsigned char* src = &srcImage->pixels[srcImage->memWidth*y];
		unsigned char* dst = &dstImage->pixels[memWidth*y];
		for (unsigned int x=0; x<srcImage->width;x++,src+=3,dst+=1) {
			dst[0] = (unsigned char) ((1+(unsigned int)src[0] + (unsigned int)src[1]+ (unsigned int)src[2])/3);
			//if (dst[0]!=255)
			//	src=src;
		}
	}
	if (dstImage == srcImage) {
		dstImage->spp = 1;
		if (!ResizeGenericImage<unsigned char>(dst)) {
			dst.Release();
			return 0;
		}
	}
	return 1;
}

int Convert1ColorToBitImage(ScriptValue &src, ScriptValue &dst, int cutoff) {
	GenericImage<unsigned char> *img = (GenericImage<unsigned char> *) src.stringVal->value;
	if (!img || img->spp!=1) return 0;
	if (!MakeAllocatedBitImage(dst, img->width, img->height)) return 0;
	BitImage *out = (BitImage*) dst.stringVal->value;
	int memWidth = ((out->width+31)/32);
	memset(out->data, 0, memWidth*out->height*4);
	int y;
	if (cutoff < 0) {
		int *row1 = ((int*) calloc(2*(img->width*2+2), sizeof(int)));
		if (row1) {
			int *mem = row1;
			row1++;
			int *row2 = row1 + img->width+2;
			for (y=0; y<out->height; y++) {
				unsigned char* src = &img->pixels[img->memWidth*y];
				unsigned char* dst = (unsigned char*) &out->data[memWidth*y];
				for (int x=0; x<out->width; x++) {
					//if (*dst) {
					//	dst=dst;
					//}
					int diff = 0;
					int val = 256*(int)*src + row1[x];
					if (val < 256*0x80) {
						*dst |= 1<<(x&0x7);
						diff = val;
					}
					else {
						diff = val-256*0xFF;
					}
					diff /=16;
					row1[x+1] += 7*diff;
					row2[x-1] += 3*diff;
					row2[x] += 5*diff;
					row2[x+1] += 1*diff;
					row1[x] = 0;
					src++;
					dst += !((x+1)&7);
				}
				int *t = row1;
				row1 = row2;
				row2 = t;
			}
			free(mem);
			return 1;
		}
		cutoff = 0;
	}
	if (!cutoff) {
		__int64 sum = 0;
		int count = 0;
		for (y=0; y<out->height; y++) {
			unsigned char* src = &img->pixels[img->memWidth*y];
			for (int x=0; x<out->width; x++) {
				count ++;
				sum += *src;
				src++;
			}
		}
		cutoff = (int)(sum/count);
	}
	for (y=0; y<out->height; y++) {
		unsigned char* src = &img->pixels[img->memWidth*y];
		unsigned char* dst = (unsigned char*) &out->data[memWidth*y];
		for (int x=0; x<out->width; x++) {
			//if (*dst) {
			//	dst=dst;
			//}
			if (*src < cutoff) {
				*dst |= 1<<(x&0x7);
			}
			src++;
			dst += !((x+1)&7);
		}
	}
	return 1;
}
/*
BitImage *Convert1ColorToBitImage(GenericImage<unsigned char> *img) {
	if (!img || img->spp!=1) return 0;
	BitImage *out = MakeAllocatedBitImage(img->width, img->height);
	if (!out) return 0;
	int memWidth = ((out->width+31)/32);
	memset(out->data, 0, memWidth*out->height*4);
	for (int y=0; y<out->height; y++) {
		unsigned char* src = &img->pixels[img->memWidth*y];
		unsigned char* dst = (unsigned char*) &out->data[memWidth*y];
		for (int x=0; x<out->width; x++) {
			//if (*dst) {
			//	dst=dst;
			//}
			if (*src < 0x80) {
				*dst |= 1<<(x&0x7);
			}
			src++;
			dst += !((x+1)&7);
		}
	}
	return out;
}
//*/
#include <stdio.h>
#include <stdlib.h>

int ConvertTo1ColorImage(ScriptValue &src, ScriptValue &dst) {
	GenericImage<unsigned char> *img = (GenericImage<unsigned char>*) src.stringVal->value;
	if (img->spp >= 3) {
		if (img->spp == 4) {
			ScriptValue mid;
			return (Convert4ColorTo3Color(src, mid) && Convert3ColorTo1Color(mid, dst));
		}
		else {
			return Convert3ColorTo1Color(src, dst);
		}
	}
	else if (img->spp == 2) {
		return Convert2ColorTo1Color(src, dst);
	}
	dst = src;
	return 1;
}


int Zoom(ScriptValue &srcv, ScriptValue &dst, double scale) {
	GenericImage<unsigned char> *src = (GenericImage<unsigned char>*) srcv.stringVal->value;
	unsigned int outWidth = (unsigned int)(src->width * scale+(0.5));
	unsigned int outHeight =  (unsigned int)(src->height * scale+(0.5));
	if (outWidth < outHeight) {
		if (outWidth < 1) {
			outWidth = 1;
			scale = 1/(double)src->width;
			outHeight =  (unsigned int)(src->height * scale+(0.5));
		}
	}
	//OleLoadPicture(9,9,9,9,9);
	else {
		if (outHeight < 1) {
			outHeight = 1;
			scale = 1/(double)src->height;
			outWidth = (unsigned int)(src->width * scale+(0.5));
		}
	}
	if (scale <1.001 && scale > 0.999) {
		outHeight = src->height;
		outWidth = src->width;
	}
	if (!MakeGenericImage<unsigned char>(dst, outWidth, outHeight, src->spp)) return 0;
	GenericImage<unsigned char> *out = (GenericImage<unsigned char> *)dst.stringVal->value;
	// return input image.
	if (scale <1.001 && scale > 0.999) {
		memcpy(out->pixels, src->pixels, out->memWidth*out->height);
		return 1;
	}
	// Just use the sum.
	if (1 || scale < 0.54) {
		double xFact = src->width/(double)outWidth;
		double yFact = src->height/(double)outHeight;
		for (unsigned int y = 0; y<outHeight; y++) {
			unsigned int starty = (unsigned int) (y * yFact);
			unsigned int endy = (unsigned int) ((y+1) * yFact);
			if (endy >= src->height) endy = src->height-1;

			for (unsigned int x = 0; x<outWidth; x++) {
				double weightSum = 0;
				double colorSum = 0;
				unsigned int startx = (unsigned int) (x * xFact);
				unsigned int endx = (unsigned int) ((x+1) * xFact);
				if (endx >= src->width) endx = src->width-1;

				for (int c = 0; c<out->spp; c++) {
					double sum = 0;
					double weightSum = 0;
					for (unsigned int y2 = starty; y2 <= endy; y2++) {
						unsigned char *px = &src->pixels[y2 * src->memWidth + out->spp*startx+c];
						double yWeight = 1;
						if (y2 == starty) yWeight = 1 - (y * yFact-starty);
						if (y2 == endy) yWeight = (y+1) * yFact - endy;
						for (unsigned int x2 = startx; x2 <= endx; x2++) {
							double xWeight = 1;
							if (x2 == startx) xWeight = 1 - (x * xFact-startx);
							if (x2 == endx) xWeight = (x+1) * xFact - endx;
							double weight = xWeight * yWeight;
							sum += *px * weight;
							weightSum += weight;
							//out += out->spp;
							px += out->spp;
						}
					}
					out->pixels[y*out->memWidth + x*out->spp + c] = (unsigned char) (sum/weightSum);
				}

			}
		}
	}
	return 1;
}



int LoadMemoryColorImage(ScriptValue &sv, unsigned char *data, int len) {
	return (LoadPNG(sv, data, len) ||
		LoadJpeg(sv, data, len) ||
		LoadFileBMP(sv, data, len) ||
		LoadGIF(sv, data, len) ||
		LoadTGA(sv, data, len));
}

int LoadMemoryImage(ScriptValue &s, unsigned char *data, int len, double zoom, int cutoff) {
	ScriptValue sv;
	if (!LoadMemoryColorImage(sv, data, len)) return 0;

	int res = 0;
	ConvertTo1ColorImage(sv, sv);

	if (zoom > 0.00000001) {
		ScriptValue temp;
		if (Zoom(sv, temp, zoom)) {
			sv.Release();
			sv = temp;
		}
	}

	res = Convert1ColorToBitImage(sv, s, cutoff);
	sv.stringVal->Release();

	return res;
}

int LoadBitImage(ScriptValue &s, unsigned char *path, double zoom, int cutoff) {
	int res = 0;
	if (path) {
		wchar_t *path3 = UTF8toUTF16Alloc(path);
		if (!path3) return 0;
		wchar_t *path2 = GetFile(path3);
		free(path3);
		if (!path2) return 0;
		FILE *in = _wfopen(path2, L"rb");
		free(path2);
		if (!in) return 0;

		fseek(in, 0, SEEK_END);
		int len = ftell(in);
		fseek(in, 0, SEEK_SET);
		unsigned char *data = (unsigned char*) malloc(sizeof(unsigned char)*(len));
		if (data) {
			if ((int)fread(data, 1, len, in) == len) {
				res = LoadMemoryImage(s, data, len, zoom, cutoff);
			}
			free(data);
		}
		fclose(in);
	}

	return res;
}


int LoadColorImage(ScriptValue &s, unsigned char *path) {
	int res = 0;
	if (path) {
		wchar_t *path3 = UTF8toUTF16Alloc(path);
		if (!path3) return 0;
		wchar_t *path2 = GetFile(path3);
		free(path3);
		if (!path2) return 0;
		FILE *in = _wfopen(path2, L"rb");
		free(path2);
		if (!in) return 0;

		fseek(in, 0, SEEK_END);
		int len = ftell(in);
		fseek(in, 0, SEEK_SET);
		unsigned char *data = (unsigned char*) malloc(sizeof(unsigned char)*(len));
		if (data) {
			if ((int)fread(data, 1, len, in) == len) {
				res = LoadMemoryColorImage(s, data, len);
			}
			free(data);
		}
		fclose(in);
	}

	return res;
}
