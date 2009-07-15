#include "tga.h"
#include "..\\malloc.h"
#include <string.h>

/*
inline float MakeScale(unsigned int bitsOrig, unsigned int bitsNew) {
	unsigned int maxOrig = (1<<bitsOrig)-1;
	unsigned int maxNew = (1<<bitsNew)-1;
	//extra is just to make sure rounding works out
	return (maxNew+0.0001f)/(float)maxOrig;
}*/

inline void InitConversionTable(unsigned char* table, unsigned int bitsOrig, unsigned int bitsNew, unsigned char def) {
	if (bitsOrig==0) {
		memset(table, def, 255*sizeof(char));
		return;
	}
	unsigned int maxOrig = (1<<bitsOrig)-1;
	unsigned int maxNew = (1<<bitsNew)-1;
	float f = (maxNew+0.0001f)/(float)maxOrig;
	for (unsigned int i=0; i<=maxOrig; i++) {
		table[i] = (unsigned char) (f*i);
	}
}

struct ColorMapSpec {
	unsigned short firstIndex, length;
	unsigned char entrySize;
};

struct ImageSpec {
	unsigned short xOrigin, yOrigin, width, height;
	unsigned char depth, descriptor;
};

struct TGAHeader {
	unsigned char idLength,
		colorMapType,
		imageType;

	ColorMapSpec colorMapSpec;
	ImageSpec imageSpec;
};

int LoadTGA(ScriptValue &sv, unsigned char *buffer, int len) {
	TGAHeader head;
	if (len < 18) return 0;
	memcpy(&head, buffer, 3);
	memcpy(&head.colorMapSpec, buffer+3, 5);
	memcpy(&head.imageSpec, buffer+8, 10);
	len -= 18;
	buffer += 18;
	//unsigned char junk[256];
	/*if (fread(junk, 18, 1, f)<1) return 0;

	memcpy(&head, junk, 3);

	head.colorMapSpec.firstIndex = Flip(&junk[3]);
	head.colorMapSpec.length = Flip(&junk[5]);
	head.colorMapSpec.entrySize = junk[7];

	head.imageSpec.xOrigin = Flip(&junk[8]);
	head.imageSpec.yOrigin = Flip(&junk[10]);
	head.imageSpec.width   = Flip(&junk[12]);
	head.imageSpec.height  = Flip(&junk[14]);
	head.imageSpec.depth      = junk[16];
	head.imageSpec.descriptor = junk[17];*/
	unsigned char colorConversionTable[256];
	unsigned char alphaConversionTable[256];

	if (head.imageType==0 || head.imageType>11 ||
		(head.imageType>3 && head.imageType<9)) return 0;
	if (!(head.imageType == 1  && head.colorMapType == 1 && head.colorMapSpec.length) &&
		!(head.imageType == 2  && head.colorMapType == 0) &&
		!(head.imageType == 3  && head.colorMapType == 0) &&

		!(head.imageType == 9  && head.colorMapType == 1 && head.colorMapSpec.length) &&
		!(head.imageType == 10 && head.colorMapType == 0) &&
		!(head.imageType == 11 && head.colorMapType == 0)) return 0;

	/*if (head.colorMapSpec.entrySize>0 && 
		head.colorMapSpec.entrySize!=15 &&
		head.colorMapSpec.entrySize!=16 &&
		head.colorMapSpec.entrySize!=24 &&
		head.colorMapSpec.entrySize!=32) return 0;*/
	if (head.colorMapSpec.entrySize>32 ||
		head.imageSpec.depth>32) return 0;
	//if (head.imageSpec.depth
	if (head.idLength && len < head.idLength) return 0;
	len -= head.idLength;
	buffer += head.idLength;
	if (!head.imageSpec.width || !head.imageSpec.height) return 0;
	//float scale, alphaScale;
	unsigned int i;
	unsigned int temp;
	unsigned int temp2;
	Color4 *colorMap=0;
	unsigned int colorMapOffset = head.colorMapSpec.firstIndex;
	unsigned int bytes, bits, alphaBits;
	if (head.imageSpec.depth<1) return 0;
	if (head.colorMapType) {
		if (!head.colorMapSpec.length ||
			!head.colorMapSpec.entrySize) return 0;
		colorMap = (Color4*) malloc(sizeof(Color4)*head.colorMapSpec.length);
		bytes = (head.colorMapSpec.entrySize+7)/8;
		bits = head.colorMapSpec.entrySize/3;
		if (bits>8) bits = 8;
		//scale = MakeScale(bits, 8);
		alphaBits = head.colorMapSpec.entrySize - bits*3;
		if (alphaBits>8) alphaBits = 8;
		InitConversionTable(colorConversionTable, bits, 8, 0);
		//if (alphaBits) {
		InitConversionTable(alphaConversionTable, alphaBits, 8, 255);
			//alphaScale = MakeScale(alphaBits, 8);
		//}
		for (i=0; i<head.colorMapSpec.length; i++) {
			//((int*)junk)[0] = 0;
			if (len < (int)bytes) {
				free(colorMap);
				return 0;
			}
			if (bytes) {
				temp2 = *(unsigned int*) buffer;
				if (bytes == 1) temp2 &= 0xFF;
				else if (bytes == 2) temp2 &= 0xFFFF;
				else if (bytes == 3) temp2 &= 0xFFFFFF;
				len -= bytes;
				buffer += bytes;
			}
			else temp2 = 0;

			//temp2 = ((unsigned int*)junk)[0];
			temp = temp2>>bits;

			colorMap[i].b = colorConversionTable[temp2-(temp<<bits)];
			temp2 = temp;
			temp = temp2>>bits;

			colorMap[i].g = colorConversionTable[temp2-(temp<<bits)];
			temp2 = temp;
			temp = temp2>>bits;

			temp = temp2>>bits;
			colorMap[i].r = colorConversionTable[temp2-(temp<<bits)];
			temp2 = temp;

			temp = temp2>>alphaBits;
			colorMap[i].a = alphaConversionTable[temp2-(temp<<alphaBits)];
		}
	}

	if (!MakeGenericImage<unsigned char>(sv, head.imageSpec.width, head.imageSpec.height, 4)) return 0;
	GenericImage<unsigned char> * image = (GenericImage<unsigned char> *) sv.stringVal->value;

	bytes = (head.imageSpec.depth+7)/8;
	bits = head.imageSpec.depth/3;
	if (head.imageType == 3 || head.imageType==11)
		bits = head.imageSpec.depth;
	if (bits>8) bits = 8;
	//scale = MakeScale(bits, 8);
	alphaBits = head.imageSpec.depth - bits*3;
	unsigned int alphaBits2 = head.imageSpec.descriptor & 0xF;
	if (alphaBits>alphaBits2) alphaBits = alphaBits2;
	if (alphaBits>8) alphaBits = 8;
	unsigned int width  = head.imageSpec.width,
				 height = head.imageSpec.height;

	if (head.imageType != 1 &&
		head.imageType != 9) {

		InitConversionTable(colorConversionTable, bits, 8, 0);
		//if (alphaBits) {
		InitConversionTable(alphaConversionTable, alphaBits, 8, 255);
	}
	else {
		bits = head.imageSpec.depth;
	}
	//if (alphaBits)
	//	alphaScale = MakeScale(alphaBits, 8);

	// 10 is for careless overwrites.  Not careful about exact bytes amounts in
	// RLE junk
	unsigned char* line = (unsigned char*) malloc(bytes*sizeof(char)*width+10);

	int dx=1, xStart=0, xEnd=width, x, dy=1, yStart=0, yEnd=height, y;
	if (head.imageSpec.descriptor & 0x10) {
		dx = -1;
		xStart = width-1;
		xEnd = -1;
	}
	if (!(head.imageSpec.descriptor & 0x20)) {
		dy = -1;
		yStart = height-1;
		yEnd = -1;
	}
	unsigned int pos;
	unsigned char code;
	for (y=yStart; y!=yEnd; y+=dy) {
		// Really no idea what to do with any of this error handling.
		// Not crashing and just garbling the rest of the image randomly
		// is good enough for me.  (RLE may be garbled...set to black most likely
		// Non-RLE is set to black.
		if (head.imageType<4) {
			if (len < (int) (bytes*sizeof(char)*width)) {
				memset(line, 0, bytes*sizeof(char)*width);
				len = 0;
			}
			else {
				memcpy(line, buffer, bytes*sizeof(char)*width);
				buffer += bytes*sizeof(char)*width;
				len -= bytes*sizeof(char)*width;
			}
		}
		else if (head.imageType>=9 && head.imageType<=11) {
			pos = 0;
			while (pos<width*bytes) {
				if (!len) {
					memset(line, 0, bytes*sizeof(char)*width);
					break;
				}
				code = buffer[0];
				len --;
				buffer++;
				unsigned int data;
				unsigned int count = (code & 0x7F)+1;
				if (count+pos/bytes > width) {
					memset(line, 0, bytes*sizeof(char)*width);
					break;
				}
				if (code & 0x80) {
					if (len < (int)(sizeof(char)*bytes)) {
						memset(line, 0, bytes*sizeof(char)*width);
						len = 0;
						break;
					}
					data = ((unsigned int*) buffer)[0];
					len -= sizeof(char)*bytes;
					buffer += sizeof(char)*bytes;
					for (i=0; i<count; i++) {
						((unsigned int*)&line[pos])[0] = data;
						pos += bytes*sizeof(char);
					}
				}
				else {
					if (len < (int) (sizeof(char)*bytes * count)) {
						len = 0;
						memset(line, 0, bytes*sizeof(char)*width);
						break;
					}
					memcpy(&line[pos], buffer, sizeof(char)*bytes * count);
					buffer += sizeof(char)*bytes * count;
					len -= sizeof(char)*bytes * count;
					pos += bytes*sizeof(char)*count;
				}
			}
		}

		pos=0;
		int imagePos = y*image->memWidth+xStart;
		if (head.imageType == 2 ||
			head.imageType == 10) {

			for (x=xStart; x!=xEnd; x+=dx, imagePos+=4*dx) {
				temp2 =  ((unsigned int*)&line[pos])[0];
				temp = temp2>>bits;

				image->pixels[imagePos+0] = colorConversionTable[temp2-(temp<<bits)];
				temp2 = temp;
				temp = temp2>>bits;

				image->pixels[imagePos+1] = colorConversionTable[temp2-(temp<<bits)];
				temp2 = temp;
				temp = temp2>>bits;

				temp = temp2>>bits;
				image->pixels[imagePos+2] = colorConversionTable[temp2-(temp<<bits)];
				temp2 = temp;

				temp = temp2>>alphaBits;
				image->pixels[imagePos+3] = alphaConversionTable[temp2-(temp<<alphaBits)];

				pos+=bytes;
			}

		}
		else if (head.imageType == 2 ||
			head.imageType == 10) {

			for (x=xStart; x!=xEnd; x+=dx, imagePos+=4*dx) {
				temp2 =  ((unsigned int*)&line[pos])[0];
				temp = temp2>>bits;
				unsigned char color = colorConversionTable[temp2-(temp<<bits)];
				image->pixels[imagePos] = color;
				image->pixels[imagePos+1] = color;
				image->pixels[imagePos+2] = color;
				temp2 = temp;
				temp = temp2>>alphaBits;
				image->pixels[imagePos+3] = alphaConversionTable[temp2-(temp<<alphaBits)];

				pos+=bytes;
			}
		}
		else if (head.imageType == 1 ||
			head.imageType == 9) {

			for (x=xStart; x!=xEnd; x+=dx, imagePos+=dx*4) {
				temp2 =  ((unsigned int*)&line[pos])[0];
				temp = temp2>>bits;
				unsigned int index = temp2-(temp<<bits) - head.colorMapSpec.firstIndex;
				if (index >= head.colorMapSpec.length) index = 0;
				*((Color4*)(&image->pixels[imagePos])) = colorMap[index];

				pos+=bytes;
			}
		}
	}

	free(line);
	if (colorMap) free(colorMap);
	return 1;
}

int SaveTGA(FILE *file, GenericImage<unsigned char> *in) {
	TGAHeader head;
	memset(&head, 0, sizeof(head));
	head.idLength = 0;
	head.colorMapType = 0;
	head.imageType = 2 + (in->spp <= 2);
	head.imageSpec.height = (unsigned short)in->height;
	head.imageSpec.width = (unsigned short)in->width;
	head.imageSpec.depth = 8*in->spp;
	head.imageSpec.descriptor = 8 * (1^(in->spp&1));
	fwrite(&head, 1, 3, file);
	fwrite(&head.colorMapSpec, 1, 5, file);
	fwrite(&head.imageSpec, 1, 10, file);
	for (int i=in->height; i--;) {
		fwrite(&in->pixels[i*in->memWidth], 1, in->spp * in->width, file);
	}
	fclose(file);
	return 1;
}