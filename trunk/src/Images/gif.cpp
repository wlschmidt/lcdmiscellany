#include "gif.h"
#include "..\\malloc.h"
#include <string.h>

struct BitReader {
	unsigned char *buffer;
	unsigned int size;
	unsigned int bits;
	unsigned int numBits;
};

struct CompressTable {
	unsigned int size;
	unsigned short prefix[1<<12];
	unsigned char suffix[1<<12];
};

int ReadBits(BitReader *reader, unsigned int numBits, unsigned int *out) {
	unsigned int res = reader->bits;
	unsigned int read = reader->numBits;
	while (read < numBits) {
		unsigned char bits;
		if (!reader->size) {
			numBits = read;
			break;
		}
		bits = reader->buffer[0];
		reader->buffer++;
		reader->size--;
		res += (bits << read);
		read+=8;
	}
	reader->numBits = read-numBits;
	reader->bits = res >> numBits;
	*out = (res & ((1 << numBits)-1));
	return numBits;
}

#pragma pack(push, 1)
struct GifHeader {
	char sig[3];
	char ver[3];
	unsigned short width;
	unsigned short height;
	unsigned char globalColorTableSize:3;
	unsigned char sort:1;
	unsigned char colorRes:3;
	unsigned char globalColorTable:1;
	unsigned char bgColor;
	unsigned char aspect;
};

struct GifGraphicControl {
	unsigned char transparent:1;
	unsigned char userInput:1;
	unsigned char disposal:3;
	unsigned char reserved:3;
	unsigned short delay;
	unsigned char transparentColor;
};

struct GifImageDescriptor {
	unsigned short left;
	unsigned short top;
	unsigned short width;
	unsigned short height;
	unsigned char localColorTableSize:3;
	unsigned char reserved:2;
	unsigned char sort:1;
	unsigned char interlace:1;
	unsigned char localColorTable:1;
};
#pragma pack(pop)

int ReadGifBlocks(unsigned char* &buffer, int &len, unsigned char* &data, int &dataSize) {
	int pos = 0;
	int outLen = 0;
	while (1) {
		if (pos >= len) return -1;
		if (buffer[pos] == 0) break;
		outLen += buffer[pos];
		pos += 1+(int)buffer[pos];
	}
	if (dataSize < outLen) {
		if (!srealloc(data, outLen)) return -1;
		dataSize = outLen;
	}
	outLen = 0;
	while (1) {
		if (*buffer == 0) {
			buffer ++;
			len --;
			return outLen;
		}
		memcpy(data+outLen, buffer+1, *buffer);
		outLen += *buffer;
		len -= 1 + (int)*buffer;
		buffer += 1 + (int)*buffer;
	}
}

int LoadGIF(ScriptValue &sv, unsigned char *buffer, int len) {
	if (len < sizeof(GifHeader) || buffer[0] != 'G' || buffer[1] != 'I' || buffer[2] != 'F'
		|| buffer[3] != '8' || buffer[5] != 'a') return 0;
	int version = 87;
	if (buffer[4] == '9') version = 89;
	else if (buffer[4] != '7') return 0;
	GifHeader *header = (GifHeader*)(buffer);

	buffer += sizeof(GifHeader);
	len -= sizeof(GifHeader);
	Color4 global[256];
	if (header->globalColorTable) {
		int colors = 1<<(header->globalColorTableSize+1);
		if (colors * 3 > len) return 0;
		int i;
		for (i=0; i<colors; i++) {
			global[i].r = buffer[0];
			global[i].g = buffer[1];
			global[i].b = buffer[2];
			global[i].a = 0xFF;
			buffer += 3;
			len -= 3;
		}
		for (i=colors; i<256; i++) {
			global[i].r = 0;
			global[i].g = 0;
			global[i].b = 0;
			global[i].a = 0xFF;
		}
		//global[header->bgColor].a = 0;
	}
	unsigned char *data = 0;
	int dataSize = 0;
	GifGraphicControl graphicControl;
	int graphicControlExists = 0;
	int res = 0;
	while (len) {
		if (buffer[0] == 0x21) {
			if (len < 2) {
				break;
			}
			int type = buffer[1];
			buffer += 2;
			len -= 2;
			int blocklen = ReadGifBlocks(buffer, len, data, dataSize);
			if (blocklen < 0) break;
			if (type == 0xF9) {
				graphicControlExists = 0;
				if (len == 4) {
					graphicControlExists = 1;
					graphicControl = *(GifGraphicControl*)data;
				}
			}
		}
		else if (buffer[0] == 0x2C) {
			buffer ++;
			len --;
			if (len < sizeof(GifImageDescriptor)) break;
			GifImageDescriptor *desc = (GifImageDescriptor*)buffer;
			buffer += sizeof(GifImageDescriptor);
			len -= sizeof(GifImageDescriptor);
			Color4 *colors = global;
			Color4 local[256];
			if (desc->localColorTable) {
				int numColors = 1<<(desc->localColorTableSize+1);
				if (numColors * 3 > len) break;
				int i;
				for (i=0; i<numColors; i++) {
					local[i].r = buffer[0];
					local[i].g = buffer[1];
					local[i].b = buffer[2];
					local[i].a = 0xFF;
					buffer += 3;
					len -= 3;
				}
				for (i=numColors; i<256; i++) {
					local[i].r = 0;
					local[i].g = 0;
					local[i].b = 0;
					local[i].a = 0xFF;
				}
				colors = local;
			}
			if (graphicControlExists) {
				colors[graphicControl.transparentColor].a = 0;
			}
			if (len < 1|| buffer[0] < 1 || buffer[0] > 8) break;
			unsigned int clear = (1<<buffer[0]);
			unsigned int bits = buffer[0]+1;
			len--;
			buffer++;
			int blocklen = ReadGifBlocks(buffer, len, data, dataSize);
			if (blocklen <= 0) break;
			BitReader reader = {data, dataSize, 0, 0};
			CompressTable table;
			unsigned int end = clear + 1;
			unsigned int size = end+1;
			if (bits*2 > size) bits++;
			int start = bits;
			if (!MakeGenericImage<unsigned char>(sv, desc->width, desc->height, 4)) break;
			GenericImage<unsigned char> *image = (GenericImage<unsigned char> *) sv.stringVal->value;
			int x = 0, y = 0;
			int happy = 0;
			while (1) {
				unsigned int code;
				if (ReadBits(&reader, bits, &code) != (int)bits) {
					break;
				}
				if (code == end) {
					happy = 1;
					break;
				}
				//dfile->count ++;
				if (code == clear) {
					/*
					dfile->count &= 7;
					if (dfile->count) {
						while (dfile->count < 8) {
							readBits(&dfile->reader, dfile->bits, &code);
							dfile->count++;
						}
						dfile->count = 0;
					}//*/
					size = end+1;
					bits = start;
				}
				else {
					unsigned int added = 1;
					unsigned int i = code;
					if (code >= size) {
						//dfile->end = 1;
						break;
					}
					while (i > clear) {
						added ++;
						i = table.prefix[i];
					}
					i = code;
					//buffer->used += added;
					x = x + added;
					y = y + x/desc->width;
					x %= desc->width;
					int x2 = x;
					int y2 = y;
					//added = 1;
					while (i > clear) {
						x2--;
						if (x2<0) {
							y2--;
							x2 = desc->width-1;
						}
						if (y2 < desc->height) {
							int pos = image->memWidth * y2 + image->spp*x2;
							*(Color4*)&image->pixels[pos] = colors[table.suffix[i]];
						}
						i = table.prefix[i];
						//added++;
					}
					if (y2 < desc->height) {
						x2--;
						if (x2<0) {
							y2--;
							x2 = desc->width-1;
						}
						int pos = image->memWidth * y2 + image->spp*x2;
						*(Color4*)&image->pixels[pos] = colors[i];
					}
					else {
						//break;
					}
					if (size-1 < (unsigned int)(1<<12)) {
						table.suffix[size-1] = i; // first bit output.
						if (size < (unsigned int)(1<<12)) {
							table.prefix[size] = code;
							table.suffix[size] = i; // first bit output.
						}
						size++;
						if (((size-1) & (1<<bits)) && bits != 12) {
							bits++;
						}
					}
					//prev = code;
				}
			}
			if (graphicControlExists) {
				graphicControlExists = 0;
				colors[graphicControl.transparentColor].a = 0xFF;
			}
			res = 1;
			if (desc->interlace) {
				int *dest = (int*) malloc(sizeof(int) * image->height + sizeof(Color4) * image->width);
				if (!dest) break;
				unsigned char *temp = (unsigned char*) (dest+image->height);
				int info[][2] = {{0,8},{4,8},{2,4},{1,2}};
				int row = 0;
				for (int p=0; p<4; p++) {
					for (unsigned int i=info[p][0]; i<image->height; i+=info[p][1]) {
						dest[row++] = i;
					}
				}
				for (unsigned int i=0; i<image->height; i++) {
					while (dest[i] != (int)i) {
						int d = dest[i];
						memcpy(temp, image->pixels + image->memWidth*i, image->memWidth);
						memcpy(image->pixels + image->memWidth*i, image->pixels + image->memWidth*d, image->memWidth);
						memcpy(image->pixels + image->memWidth*d, temp, image->memWidth);
						dest[i] = dest[d];
						dest[d] = d;
					}
				}
				free(dest);
			}
			break;
		}
		else if (buffer[0] == 0x3B) break;
		else break;
	}
	free(data);
	return res;
}

