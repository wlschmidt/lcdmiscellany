//#include "hash.h"
#include "png.h"
#include "inflate.h"
//#include "deflate.h"
#include "../malloc.h"
#include <string.h>


struct PNGHeader {
	unsigned int width, height;
	unsigned char depth, colorType, compression, filter, interlace;
};

struct Chunk {
	unsigned int length;
	union {
		unsigned char type[4];
		// for fast/simple comparisons
		unsigned int typei;
	};
	unsigned char *data;
	unsigned int crc;
};

inline unsigned int GetBytes(const unsigned int num, unsigned int &ndata) {
	return ndata&((1<<num)-1);
}



struct Tree {
	Tree *left, *right;
	unsigned int value, count, depth;
	inline int operator< (const Tree &t) const {
		if (count < t.count) return 1;
		if (count == t.count &&
			depth > t.depth) return 1;
		return 0;
	}
};

unsigned int Unwind(Tree *tree, unsigned int *codeLengths, unsigned int depth) {
	if (tree->value!=-1) {
		codeLengths[tree->value] = depth;
		return depth;
	}
	else {
		depth++;
		unsigned int d1 = Unwind(tree->right, codeLengths, depth);
		unsigned int d2 = Unwind(tree->left, codeLengths, depth);
		if (d1>d2) return d1;
		return d2;
	}
}


inline int GetChunk(unsigned char * &data, int &len, Chunk &c, unsigned int* crcTable) {
	if (len < 8) return 0;
	c.length = FlipLong(data);
	c.typei = ((unsigned int*)(&data[4]))[0];
	data += 8;
	len -= 8;
	//c.type = FlipLong(temp+4);
	if (c.length >= (1<<31) || c.length > (unsigned int)len+4) return 0;
	c.data = data;
	data += c.length;
	len -= c.length;
	c.crc = FlipLong(data);
	data+=4;
	len -= 4;

	/*unsigned int crc = 0xFFFFFFFF;
	unsigned int i;
	for (i=4; i<8; i++) {
		crc = crcTable[(crc ^ temp[i]) & 0xFF] ^ (crc >> 8);
	}
	for (i=0; i<c.length; i++) {
		crc = crcTable[(crc ^ c.data[i]) & 0xFF] ^ (crc >> 8);
	}
	crc ^= 0xFFFFFFFF;
	if (crc != c.crc) return 0;
	//*/
	return 1;
}

int LoadPNG(ScriptValue &sv, unsigned char *data, int len) {
	if (!data) return 0;
	int i, j;
	if ((len-=8) < 0) return 0;
	// PNG signature
	if (((int*)data)[0] != 0x474e5089 || ((int*)data)[1]!=0x0a1a0a0d) return 0;
	data += 8;

	unsigned int crcTable[256];
	for (i=0; i<256; i++) {
		unsigned int c = (unsigned int)i;
		for (j=0; j<8; j++) {
			if (c&1)
				c = 0xedb88320L ^ (c>>1);
			else
				c >>= 1;
		}
		crcTable[i]=c;
	}

	Chunk c={0,0,0,0};
	PNGHeader head={0,0,0,0,0,0,0};
	/*int *test[3];
	test[0] = (int*)"PLTE";
	test[1] = (int*)"TRNS";
	test[2] = (int*)"IDAT";*/

	unsigned char *compressed = 0;
	unsigned int compressedBytes = 0;
	Color4 *palette = 0;
	unsigned int numPalette = 0;
	int transparentColor[4] = {-1,-1,-1,-1};
	while (GetChunk(data, len, c, crcTable)) {
		// IHDR
		if (c.typei==0x52444849 && c.length == 13) {
			if (head.width) return 0;
			head.width = FlipLong(c.data);
			head.height = FlipLong(&c.data[4]);
			head.depth = c.data[ 8];
			head.colorType = c.data[ 9];
			head.compression = c.data[10];
			head.filter = c.data[11];
			head.interlace = c.data[12];
			if (!head.width ||
				head.width >= (1<<31) ||
				!head.height ||
				head.height >= (1<<31)) return 0;
			if (head.depth!= 1 && head.depth != 2 &&
				head.depth!= 4 && head.depth != 8 &&
				head.depth!= 16) return 0;

			if (head.colorType == 2 && head.depth<8) return 0;
			if (head.colorType == 3 && head.depth>8) return 0;
			if (head.colorType == 4 && head.depth<8) return 0;
			if (head.colorType == 6 && head.depth<8) return 0;
			if (head.colorType!=0 && head.colorType!=2 && 
				head.colorType!=3 && head.colorType!=4 &&
				head.colorType!=6) return 0;
			if (head.compression || head.filter) return 0;
			if (head.interlace!=0 && head.interlace !=1) return 0;
		}
		// IDAT 0x54414449
		else if (c.typei == 0x54414449) {
			if (head.width==0) break;
			if (compressed) {
				if (srealloc(compressed, sizeof(unsigned char) * (c.length+compressedBytes))) {
					memcpy (&compressed[compressedBytes], c.data, c.length);
					compressedBytes += c.length;
				}
			}
			else {
				compressed = (unsigned char*) malloc(sizeof(unsigned char) * c.length);
				if (compressed) {
					memcpy(compressed, c.data, c.length);
					c.data = 0;
					compressedBytes = c.length;
				}
			}
		}
		// IEND
		else if (c.typei == 0x444e4549) {
			i=0;
			break;
		}
		// TRNS
		else if (c.typei == 0x534e5274) {
			if (head.width == 0) break;
			if (head.colorType == 3 && palette) {
				for (i=0; i<(int)numPalette && i<(int)c.length; i++) {
					palette[i].a = c.data[i];
				}
			}
			else if (head.colorType == 0) {
				if (c.length)
					transparentColor[0] = (int) c.data[0];
				if (c.length>0)
					transparentColor[0] += ((int)c.data[1]) <<8;
				transparentColor[2] = 
					transparentColor[1] = 
					transparentColor[0];
			}
			else if (head.colorType == 2) {
				transparentColor[0] =
					transparentColor[1] =
					transparentColor[2] = 0;
				if (c.length)
					transparentColor[0] = ((int) c.data[0]) << 8;
				if (c.length>0)
					transparentColor[0] += (int)c.data[1];
				if (c.length>1)
					transparentColor[1] = ((int) c.data[2]) << 8;
				if (c.length>2)
					transparentColor[1] += (int)c.data[3];
				if (c.length>3)
					transparentColor[2] = ((int) c.data[4]) << 8;
				if (c.length>4)
					transparentColor[2] += (int)c.data[5];
			}
		}
		// PLTE
		else if (c.typei == 0x45544c50) {
			if (head.width==0) break;
			if (compressed || palette) {
				free(compressed);
				compressed = 0;
				break;
			}
			numPalette = c.length/3;
			if (numPalette>256) continue;
			palette = (Color4*) malloc(numPalette * sizeof(Color4));
			if (palette) {
				for (i=0; i<(int)numPalette; i++) {
					// Have to flip, as flip back at end.
					palette[i].b = c.data[i*3];
					palette[i].g = c.data[i*3+1];
					palette[i].r = c.data[i*3+2];
					palette[i].a = 255;
				}
			}
		}
		else {
			// Not recognized and vital
			if (!(c.type[0] & 0x20)) {
				if (compressed) {
					free(compressed);
					compressed = 0;
				}
				break;
			}
		}
	}
	if (!compressed) {
		if (palette) free(palette);
		return 0;
	}

	if (c.typei != 0x444e4549 || compressedBytes<3) {
		if (palette) free(palette);
		free (compressed);
		return 0;
	}
	if (head.colorType == 3 &&
		!palette) {
		free(compressed);
		return 0;
	}

	unsigned int bpp;
	unsigned int spp = 1;
	if (head.colorType ==2 || head.colorType == 6) spp = 3;
	if (head.colorType&4) spp++;
	unsigned int bitsPerPixel = spp*head.depth;
	bpp = (bitsPerPixel+7)/8;
	int size =  (bpp*head.width+1)*head.height;
	unsigned int uncompressedBytes = 0;
	unsigned char * uncompressed = Inflate(compressed, compressedBytes, size, uncompressedBytes, 0);
	free(compressed);
	if (!uncompressed) {
		free(palette);
		return 0;
	}


	unsigned char mask = (1<<head.depth)-1;
	// could make a table, but this is only for grayscale
	// so who cares?
	float scale = (0xFF)/(float)mask;
	unsigned int step = 1;
	if (head.depth==16) step = 2;

	int outspp = 4;
	if (head.depth == 8 && (spp==3 || spp == 1) && transparentColor[0]==-1)
		outspp = 3;
	if (!MakeGenericImage<unsigned char>(sv, head.width, head.height, outspp)) {
		free(uncompressed);
		free(palette);
		return 0;
	}
	GenericImage<unsigned char> *image = (GenericImage<unsigned char> *)sv.stringVal->value;
	unsigned int x, y;
	unsigned int pos=0;
	unsigned int xStart, dx, xEnd,
		yStart, dy, yEnd;
	unsigned int width;
	for (i=0; i<1+head.interlace*6; i++) {
		unsigned int lineLength;
		if (head.interlace==0) {
			xStart = yStart = 0;
			xEnd = width = head.width;
			yEnd = head.height;
			dx=dy=1;
		}
		else {
			xEnd = head.width;
			yEnd = head.height;
			switch(i) {
			case 0:
				dx = 8;
				dy = 8;
				xStart = 0;
				yStart = 0;
				width = (head.width+dx-1-xStart)/8;
				break;

			case 1:
				dx = 8;
				dy = 8;
				xStart = 4;
				yStart = 0;
				width = (head.width+dx-1-xStart)/8;
				break;

			case 2:
				dx = 4;
				dy = 8;
				xStart = 0;
				yStart = 4;
				width = (head.width+dx-1-xStart)/4;
				break;

			case 3:
				dx = 4;
				dy = 4;
				xStart = 2;
				yStart = 0;
				width = (head.width+dx-1-xStart)/4;
				break;

			case 4:
				dx = 2;
				dy = 4;
				xStart = 0;
				yStart = 2;
				width = (head.width+dx-1-xStart)/2;
				break;

			case 5:
				dx = 2;
				dy = 2;
				xStart = 1;
				yStart = 0;
				width = (head.width+dx-1-xStart)/2;
				break;

			case 6:
			default:
				dx = 1;
				dy = 2;
				xStart = 0;
				yStart = 1;
				width = head.width;
			}
		}
		lineLength = (1+(bitsPerPixel*width+7)/8);
		int q = -1;

		for (y=yStart; y<yEnd; y+=dy) {
			if (pos+lineLength>uncompressedBytes) {
				free(uncompressed);
				free(palette);
				return 1;
			}
			unsigned char* pixels = &image->pixels[y*image->memWidth];

			int done = 0;

			if (!done) {
				//unsigned int pos2 = pos;
				if (uncompressed[pos]==1) {
					// Sub
					unsigned char v;
					for (x=bpp+1; x<lineLength; x++) {
						//if (x<bpp+1) v=0;
						//else
						v = uncompressed[pos+x-bpp];
						uncompressed[pos+x] = (uncompressed[pos+x] + v);
					}
				}
				else if (uncompressed[pos]==2) {
					// Up
					if (y) {
						for (x=1; x<lineLength; x++) {
							uncompressed[pos+x] = (uncompressed[pos+x] + uncompressed[pos+x-lineLength]);
						}
					}
				}
				else if (uncompressed[pos]==3) {
					// Average
					unsigned char up=0, left=0;
					for (x=1; x<lineLength; x++) {
						if (y) up = uncompressed[pos+x-lineLength];
						if (x>= bpp+1) left = uncompressed[pos+x-bpp];
						uncompressed[pos+x] = (uncompressed[pos+x] + (up + left)/2);
					}
				}
				else if (uncompressed[pos]==4) {
					// Paeth
					unsigned char up=0, left=0, upleft = 0;
					for (x=1; x<lineLength; x++) {
						if (y) {
							up = uncompressed[pos+x-lineLength];
							if (x>=bpp+1) {
								upleft = uncompressed[pos+x-bpp-lineLength];
								left = uncompressed[pos+x-bpp];
							}
						}
						else {
							if (x>= bpp+1) left = uncompressed[pos+x-bpp];
							up = 0;
						}
						int p = (int) left + (int) up - (int) upleft;
						int pa = abs(p - (int) left);
						int pb = abs(p - (int) up);
						int pc = abs(p - (int) upleft);
						if (pa <= pb && pa <= pc) up = left;
						else if (pb <= pc) up = up;
						else up = upleft;
						//else upleft = upleft;
						uncompressed[pos+x] = (uncompressed[pos+x] + up);
					}
				}
				else if (uncompressed[pos]!=0) {
					free(uncompressed);
					free(palette);
					return 1;
				}
			}




			pos ++;
			int offset = 8 - head.depth;
			// Just junk the last 8 bytes, in case of 16 bit samples.
			// could do better, but see no reason to bother.
			// Most significant first, so that's all I need to worry about.
			if (done<2)
			if (head.colorType == 0) {
				//la, 1-16 bit
				if (head.depth>=8) {
					for (x=xStart; x<xEnd; x+=dx) {
						pixels[0] = pixels[1] = pixels[2] = uncompressed[pos];
						pixels[3] = 255;
						pixels += outspp;
						pos+=step;
					}
				}
				else {
					//1, 2, or 4 bitspp
					for (x=xStart; x<xEnd; x+=dx) {
						unsigned char color = (uncompressed[pos]>>offset) & mask;
						pixels[0] = pixels[1] = pixels[2] = (unsigned char) (color * scale);
						pixels[3] = 255;
						pixels += outspp;
						offset -= head.depth;
						if (offset<0) {
							offset = 8 - head.depth;
							pos++;
						}
						//image->pixels[x+y*head.width].a;
					}
					if (offset != 8 - head.depth) pos++;
				}
			}
			else if (head.colorType == 2) {
				unsigned int temp = head.width*y;
				unsigned int end = temp + xEnd;
				temp +=xStart;
				unsigned char* c = (unsigned char*)&image->pixels[y * image->memWidth];
				unsigned char* e = c + head.width * outspp;
				if (step==1) {
					if (dx==1) {
						while (c<e) {
							((unsigned int*)c)[0] = ((unsigned int*)&uncompressed[pos])[0]|0xFF000000;
							c+=outspp;
							pos+=3;
						}
					}
					else {
						for (; c<e; c+=dx) {
							((unsigned int*)c)[0] = ((unsigned int*)&uncompressed[pos])[0]|0xFF000000;
							c+=outspp;
							pos+=3;
						}
					}
				}
				else {
					for (; temp<end; temp+=dx) {
						pixels[0] = uncompressed[pos];
						pos+=step;
						pixels[1] = uncompressed[pos];
						pos+=step;
						pixels[2] = uncompressed[pos];
						pos+=step;
						pixels[3] = 255;
						pixels+=outspp;
					}
				}
			}
			else if (head.colorType == 3) {
				// Palette, 1, 2, 4, or 8 bit
				for (x=xStart; x<xEnd; x+=dx) {
					unsigned char color = (uncompressed[pos]>>offset) & mask;
					if (color>numPalette) color = 0;
					((Color4*)pixels)[0] = palette[color];
					pixels+=outspp;
					offset-=head.depth;
					if (offset <= 0) {
						offset = 8 - head.depth;
						pos++;
					}
				}
				if (offset != 8 - head.depth) pos++;
			}
			else if (head.colorType == 4) {
				// la, 8-16 bit.
				for (x=xStart; x<xEnd; x+=dx) {
					pixels[0] = pixels[1] = pixels[2] = uncompressed[pos];
					pos+=step;
					pixels[3] = uncompressed[pos];
					pixels += outspp;
					pos+=step;
				}
			}
			else if (head.colorType == 6) {
				// rgba, 8-16 bit.  could combine with 2, but too lazy
				if (step==1) {
					for (x=xStart; x<xEnd; x+=dx) {
						((int*)pixels)[0] = ((int*)&uncompressed[pos])[0];
						pixels += outspp;
						pos+=4;
					}
				}
				else {
					for (x=xStart; x<xEnd; x+=dx) {
						pixels[0] = uncompressed[pos];
						pos+=step;
						pixels[1] = uncompressed[pos];
						pos+=step;
						pixels[2] = uncompressed[pos];
						pos+=step;
						pixels[3] = uncompressed[pos];
						pixels += outspp;
						pos+=step;
					}
				}
			}
		}
	}

	free(uncompressed);
	free(palette);

	pos = 0;
	if (transparentColor[0]!=-1) {
		for (y=0; y<head.height; y++) {
			pos = y * image->memWidth;
			for (x=0; x<head.width; x++) {
				if (image->pixels[pos] == transparentColor[0]>>8 &&
					image->pixels[pos+1] == transparentColor[1]>>8 &&
					image->pixels[pos+2] == transparentColor[2]>>8)
					image->pixels[pos+3] = 0;
				pos += outspp;
			}
		}
	}

	if (image->spp >= 3) {
		for (int y=image->height-1;y>=0; y--) {
			int yp = y*image->memWidth;
			for (int i = yp+(image->width-1)*image->spp; i>=yp; i-=image->spp) {
				unsigned char c = image->pixels[i];
				image->pixels[i] = image->pixels[i+2];
				image->pixels[i+2] = c;
			}
		}
	}

	return 1;
}





















#ifdef GOAT











































inline void WriteChunk(FILE *f, Chunk &c, unsigned int* crcTable) {
	unsigned int temp2[2];
	temp2[0] = FlipLong((unsigned char*)&c.length);
	temp2[1] = c.typei;
	fwrite(temp2, 8, 1, f);
	fwrite(c.data, c.length, 1, f);
	unsigned char* temp = (unsigned char*)temp2;


	unsigned int crc = 0xFFFFFFFF;
	unsigned int i;
	for (i=4; i<8; i++) {
		crc = crcTable[(crc ^ temp[i]) & 0xFF] ^ (crc >> 8);
	}
	unsigned char *data = c.data;
	int len = c.length;
	//unsigned int test;
	__asm {
		push ebx
		mov esi, crcTable
		xor ecx, ecx
		mov edi, data
		add edi, len
		sub ecx, len
		mov eax, crc
		jz end
		//xor edx, edx
		mov ebx, [edi+ecx]
start:
//		mov edx, 0xFF
		movzx edx, bl
		shr ebx, 8
		xor dl, al
		shr eax, 8
		xor eax, [esi+4*edx];
		inc ecx
		movzx edx, bl
		jz end

		shr ebx, 8
		xor dl, al
		shr eax, 8
		xor eax, [esi+4*edx];
		inc ecx
		movzx edx, bl
		jz end

		shr ebx, 8
		xor dl, al
		shr eax, 8
		xor eax, [esi+4*edx];
		movzx edx, bl
		inc ecx
		mov ebx, [edi+ecx+1]
		jz end

		xor dl, al
		shr eax, 8
		xor eax, [esi+4*edx];
		inc ecx
		jnz start

end:
		not eax
		mov crc, eax
		pop ebx
	}
	/*for (i=0; i<c.length; i++) {
		crc = crcTable[(crc ^ c.data[i]) & 0xFF] ^ (crc >> 8);
	}
	crc ^= 0xFFFFFFFF;*/
	c.crc = FlipLong((unsigned char*)&crc);
	fwrite(&c.crc, 4, 1, f);
}


void SavePngRGBA(FILE *f, ImageRGBA *image) {
	unsigned int temp[5] = {0x474e5089, 0x0a1a0a0d};
	fwrite(temp, 8, 1, f);
	unsigned int i, j;

	unsigned int crcTable[256];
	for (i=0; i<256; i++) {
		unsigned int c = (unsigned int)i;
		for (j=0; j<8; j++) {
			if (c&1)
				c = 0xedb88320L ^ (c>>1);
			else
				c >>= 1;
		}
		crcTable[i]=c;
	}

	Chunk c;
	c.typei = 0x52444849;
	c.length = 13;
	c.data = (unsigned char*) temp;
	((unsigned int*)c.data)[0] = FlipLong((unsigned char*)&image->width);
	((unsigned int*)c.data)[1] = FlipLong((unsigned char*)&image->height);
	c.data[8] = 8;
	c.data[9] = 2;
	c.data[10] = 0;
	c.data[11] = 0;
	c.data[12] = 0;

	WriteChunk(f, c, crcTable);

	unsigned int lineLength = 1+3*image->width;
	unsigned int bpp = 3;
	unsigned char* uncompressed = (unsigned char*) malloc(lineLength*image->height+16);

	//unsigned int pos;
	int x, y;
	//unsigned int adler32 = 1L;
	//unsigned int counts[257];
	unsigned int lengths[257];
	for (i=0; i<256; i++) {
		if (i>250) lengths[i] = 256-i;
		else lengths[i] = i;
		if (i>15) 
			lengths[i] = 8;
		else {
			unsigned int temp = 3;
			while (lengths[i]) {
				temp++;
				lengths[i]>>=1;
			}
			lengths[i] = temp;
		}
		//if (i>=128) lengths[i] = 9;
		//else lengths[i] = 8;
	}
	lengths[0]=0;
	unsigned int len;
	//unsigned int end = 1;
	int width = image->width;
	int height = image->height;
	int width4 = width*4;
	//for (j=0; j<end; j++) {
		unsigned char *uncompressed2 = uncompressed;
		Color4 *pixels = image->pixels;
		//pos = 0;//pos2=0;
		for (y=0; y<height; y++) {
			//unsigned int initialPos = pos;
			/*if (0&&y==0) {
				uncompressed[pos++] = 1;
				uncompressed[pos++] = image->pixels[pos2].r;
				uncompressed[pos++] = image->pixels[pos2].g;
				uncompressed[pos++] = image->pixels[pos2].b;
				pos2++;
				for (x=1; x<image->width; x++) {
					uncompressed[pos] = image->pixels[pos2].r - image->pixels[pos2-1].r;
					pos++;
					uncompressed[pos] = image->pixels[pos2].g - image->pixels[pos2-1].g;
					pos++;
					uncompressed[pos] = image->pixels[pos2].b - image->pixels[pos2-1].b;
					pos++;
					pos2++;
				}
			}
			else {*/
			unsigned int score0=lengths[0], score1=lengths[1], score2=lengths[2], score3=lengths[3], score4=lengths[4];
			Color4* color=pixels;
			unsigned int up=0, left=0, upleft=0;
			unsigned int x1, x2, x3, x4;
			if (0) {
				for (x=0; x<width; x++) {
					for (unsigned int t=0; t<3; t++) {
						if (y) {
							up = (&color[-width].r+t)[0];
							if (x) {
								left = (&color[-1].r+t)[0];
								upleft = (&color[-width-1].r+t)[0];
							}
						}
						else if (x) {
							left = (&color[-1].r+t)[0];
						}
						unsigned int val = (&color[0].r+t)[0];

						//x0 = val;
						x1 = (val - left)%256;
						x2 = (val - up)%256;
						x3 = (val - (up+left)/2)%256;

						//*
						//int pa, pb, pc;
						__asm {
							/*mov eax, up
							mov edx, upleft
							mov ecx, left
							sub eax, edx
							sub ecx, edx
							mov edx, eax
							add eax, ecx
							// edx = pa
							// ecx = pb
							// eax = pc
							mov esi, edx
							mov edi, ecx
							neg esi
							cmovg edx, esi
							neg edi
							mov esi, eax
							cmovg ecx, edi
							neg esi
							//mov edi, upleft
							cmovg eax, esi

							mov pa, edx
							mov pb, ecx
							mov pc, eax*/
							mov eax, up
							mov edx, upleft
							mov ecx, left
							sub eax, edx
							sub ecx, edx
							mov edx, eax
							add eax, ecx
							// edx = pa
							// ecx = pb
							// eax = pc
							mov esi, edx
							mov edi, ecx
							neg esi
							cmovg edx, esi
							neg edi
							mov esi, eax
							cmovg ecx, edi
							neg esi
							mov edi, upleft
							cmovg eax, esi
							cmp ecx, eax
							cmovle edi, up
							cmovle eax, ecx
							cmp edx, eax
							mov eax, val
							cmovle edi, left
							sub eax, edi
							and eax, 0xFF
							mov x4, eax
						}
						/*int p1 =(int) left - (int) upleft; 
						int p2 =(int) up - (int) upleft; 
						//int p = (int) left + (int) up - (int) upleft;
						int pa = abs(p2);
						int pb = abs(p1);
						int pc = abs(p1+p2);
						if (pa <= pb && pa <= pc) x4 = x1;
						else if (pb <= pc) x4 = x2;
						else x4 = (val - upleft)%256;
						//*/

						/*if (x0>128) x0 = 256-x0;
						if (x1>128) x1 = 256-x1;
						if (x2>128) x2 = 256-x2;
						if (x3>128) x3 = 256-x3;
						if (x4>128) x4 = 256-x4;
						/*if (x1>5) x1*=2;
						if (x2>5) x2*=2;
						if (x3>5) x3*=2;
						if (x4>5) x4*=2;
						//*/
						//if (x2<6||x2>254) x2 = 1;
						//if (x3<6||x3>254) x3 = 1;
						score0+=lengths[val];
						score1+=lengths[x1];
						score2+=lengths[x2];
						score3+=lengths[x3];
						score4+=lengths[x4];
					}

					color++;

				}
	//score4 = score3+5;
				if (score0<=score1 && score0<=score2 && score1<=score3 && score1 <= score4) {
					uncompressed2++[0] = 0;
					for (x=0; x<width; x++) {
						uncompressed2[0] = pixels->r;
						uncompressed2[1] = pixels->g;
						uncompressed2[2] = pixels->b;
						uncompressed2+=3;
						pixels++;
					}
				}
				else if (score1<score2 && score1<score3 && score1 < score4) {
					uncompressed2++[0] = 1;
					int tx=0, ty=0, tz=0;
					for (x=0; x<width; x++) {
						uncompressed2[0] = pixels->r - tx;
						uncompressed2[1] = pixels->g - ty;
						uncompressed2[2] = pixels->b - tz;
						uncompressed2+=3;
						tx = pixels->r;
						ty = pixels->g;
						tz = pixels->b;
						pixels++;
					}
				}
				else if (score2<score3 && score2 < score4) {
					uncompressed2++[0] = 2;
					int sx=0, sy=0, sz=0;
					for (x=0; x<width; x++) {
						if (y) {
							sx = pixels[-width].r;
							sy = pixels[-width].g;
							sz = pixels[-width].b;
						}
						uncompressed2[0] = pixels->r - sx;
						uncompressed2[1] = pixels->g - sy;
						uncompressed2[2] = pixels->b - sz;
						uncompressed2+=3;
						pixels++;
					}
				}
				else if (score3<score4) {
					uncompressed2++[0] = 3;
					int tx=0, ty=0, tz=0;
					int sx=0, sy=0, sz=0;
					for (x=0; x<width; x++) {
						if (x) {
							tx = pixels[-1].r;
							ty = pixels[-1].g;
							tz = pixels[-1].b;
						}
						if (y) {
							sx = pixels[-width].r;
							sy = pixels[-width].g;
							sz = pixels[-width].b;
						}
						uncompressed2[0] = pixels->r - (sx+tx)/2;
						uncompressed2[1] = pixels->g - (sy+ty)/2;
						uncompressed2[2] = pixels->b - (sz+tz)/2;
						uncompressed2+=3;
						pixels++;
					}
				}
				else {
					uncompressed2++[0] = 4;
					up = upleft = left = 0;
					for (x=0; x<width; x++) {
						for (unsigned int t=0; t<3; t++) {
							if (y) {
								up = (&pixels[-width].r+t)[0];
								if (x) {
									left = (&pixels[-1].r+t)[0];
									upleft = (&pixels[-width-1].r+t)[0];
								}
								else {
									left=upleft=0;
								}
							}
							else if (x) {
								left = (&pixels[-1].r+t)[0];
								up=upleft=0;
							}
							unsigned int val = (&pixels->r+t)[0];

							//int pa, pb, pc, p1;
							__asm {
								mov eax, up
								mov edx, upleft
								mov ecx, left
								sub eax, edx
								sub ecx, edx
								mov edx, eax
								add eax, ecx
								// edx = pa
								// ecx = pb
								// eax = pc
								mov esi, edx
								mov edi, ecx
								neg esi
								cmovg edx, esi
								neg edi
								mov esi, eax
								cmovg ecx, edi
								neg esi
								mov edi, upleft
								cmovg eax, esi
								cmp ecx, eax
								cmovle edi, up
								cmovle eax, ecx
								cmp edx, eax
								mov eax, val
								cmovle edi, left
								mov ecx, uncompressed2
								sub eax, edi
								inc ecx
								mov [ecx-1], al
								mov uncompressed2, ecx

								//mov pa, edx
								//mov pb, ecx
								//mov pc, eax
							}
							/*int p1 =(int) left - (int) upleft; 
							int p2 =(int) up - (int) upleft; 
							//int p = (int) left + (int) up - (int) upleft;
							int pa = abs(p2);
							int pb = abs(p1);
							int pc = abs(p1+p2);
							*/
							/*p1 =(int) left - (int) upleft; 
							int p2 =(int) up - (int) upleft; 
							//int p = (int) left + (int) up - (int) upleft;
							int pa2 = abs(p2);
							int pb2 = abs(p1);
							int pc2 = abs(p1+p2);
							if (pa2!=pa || pb2!=pb || pc2!=pc) {
								x=x;
							}*/
							/*if (pa <= pb && pa <= pc) p1 = left;
							else if (pb <= pc) p1 = up;
							else p1 = upleft;
							uncompressed2++[0] = (val - p1)%256;*/
						}

						pixels++;
					}
				}
			}
			else {
				uncompressed2++[0] = 4;
				up = upleft = left = 0;
				for (x=0; x<width; x++) {
					for (unsigned int t=0; t<3; t++) {
						unsigned char *c = (&pixels[0].r)+t;
						/*c[0] = 0xA2;
						c[-4] = 0xA0;
						c[-width4] = 0xA4;
						c[-width4-4] = 0xA2;
						//*/
						if (y) {
							up = (&pixels[-width].r+t)[0];
							if (x) {
								left = (&pixels[-1].r+t)[0];
								upleft = (&pixels[-width-1].r+t)[0];
							}
							else {
								left=upleft=0;
							}
						}
						else if (x) {
							left = (&pixels[-1].r+t)[0];
							up=upleft=0;
						}
						unsigned int val = (&pixels->r+t)[0];

						__asm {
							mov eax, up
							mov edx, upleft
							mov ecx, left
							sub eax, edx
							sub ecx, edx
							mov edx, eax
							mov esi, eax
							add eax, ecx
							// edx = pa
							// ecx = pb
							// eax = pc
							neg esi
							mov edi, ecx
							cmovg edx, esi
							neg edi
							mov esi, eax
							cmovg ecx, edi
							neg esi
							mov edi, upleft
							cmovg eax, esi
							cmp ecx, eax
							mov esi, uncompressed2
							cmovle edi, up
							cmovle eax, ecx
							cmp edx, eax
							mov eax, val
							cmovle edi, left
							inc esi
							sub eax, edi
							mov [esi-1], al
							mov uncompressed2, esi

							//mov pa, edx
							//mov pb, ecx
							//mov pc, eax
						}
						//x--;
						/*int p1 =(int) left - (int) upleft; 
						int p2 =(int) up - (int) upleft; 
						//int p = (int) left + (int) up - (int) upleft;
						int pa = abs(p2);
						int pb = abs(p1);
						int pc = abs(p1+p2);
						*/
						/*p1 =(int) left - (int) upleft; 
						int p2 =(int) up - (int) upleft; 
						//int p = (int) left + (int) up - (int) upleft;
						int pa2 = abs(p2);
						int pb2 = abs(p1);
						int pc2 = abs(p1+p2);
						if (pa2!=pa || pb2!=pb || pc2!=pc) {
							x=x;
						}*/
						/*if (pa <= pb && pa <= pc) p1 = left;
						else if (pb <= pc) p1 = up;
						else p1 = upleft;
						uncompressed2++[0] = (val - p1)%256;*/
					}

					pixels++;
				}
			}
			//if (j==end-1)
			//	adler32= UpdateAdler32(adler32, &uncompressed[initialPos], pos-initialPos);
		}
		//*
		len = uncompressed2 - uncompressed;
		/*if (j!=end-1) {
			counts[256] = 1;
			for (i=0; i<256; i++) {
				counts[i]=0;
			}
			for (i=0; i<len; i++) {
				counts[uncompressed[i]]++;
			}
			unsigned int temp = 257;
			if (j==0)
				GenLengths(counts, lengths, temp, 15);
			for (i=0; i<256; i++) {
				if (lengths[i]==0) lengths[i] = 15;
			}//*/
		//}
		//break;
	//}
	//unsigned int test = UpdateAdler32(1L, uncompressed, lineLength*image->height);

	unsigned int compressedBytes = 0;
	unsigned char *compressed = Deflate(uncompressed, len, compressedBytes, lineLength);

	free(uncompressed);

	c.typei = 0x54414449;
	c.length = compressedBytes;
	c.data = compressed;
	WriteChunk(f, c, crcTable);
	//FILE *temp2 = fopen ("test.bin", "wb");
	//fwrite(&compressed[0], 1, compressedBytes, temp2);
	//fclose(temp2);
	free(compressed);

	c.typei = 0x444e4549;
	c.length = 0;
	c.data = 0;
	WriteChunk(f, c, crcTable);

	fclose(f);

}

















































void SavePngRGB(FILE *f, ImageRGB *image) {
	unsigned int temp[5] = {0x474e5089, 0x0a1a0a0d};
	fwrite(temp, 8, 1, f);
	unsigned int i, j;

	unsigned int crcTable[256];
	for (i=0; i<256; i++) {
		unsigned int c = (unsigned int)i;
		for (j=0; j<8; j++) {
			if (c&1)
				c = 0xedb88320L ^ (c>>1);
			else
				c >>= 1;
		}
		crcTable[i]=c;
	}

	Chunk chunk;
	chunk.typei = 0x52444849;
	chunk.length = 13;
	chunk.data = (unsigned char*) temp;
	((unsigned int*)chunk.data)[0] = FlipLong((unsigned char*)&image->width);
	((unsigned int*)chunk.data)[1] = FlipLong((unsigned char*)&image->height);
	chunk.data[8] = 8;
	chunk.data[9] = 2;
	chunk.data[10] = 0;
	chunk.data[11] = 0;
	chunk.data[12] = 0;

	WriteChunk(f, chunk, crcTable);

	unsigned int lineLength = 1+3*image->width;
	unsigned int bpp = 3;
	unsigned char* uncompressed = (unsigned char*) malloc(lineLength*image->height+16);

	//unsigned int pos;
	int y;
	//unsigned int adler32 = 1L;
	/*unsigned int counts[257];
	unsigned int lengths[257];
	for (i=0; i<256; i++) {
		if (i>250) lengths[i] = 256-i;
		else lengths[i] = i;
		if (i>15) 
			lengths[i] = 8;
		else {
			unsigned int temp = 3;
			while (lengths[i]) {
				temp++;
				lengths[i]>>=1;
			}
			lengths[i] = temp;
		}
		//if (i>=128) lengths[i] = 9;
		//else lengths[i] = 8;
	}
	lengths[0]=0;*/
	unsigned int len;
	//unsigned int end = 1;
	int width = image->width;
	int height = image->height;
	int width3 = width*3;
	//for (j=0; j<end; j++) {
		unsigned char *c;
		unsigned char *uncompressed2 = uncompressed;
		//Color3 *pixels = image->pixels;
		//pos = 0;//pos2=0;
		c = &image->pixels[0].r;
		for (y=0; y<height; y++) {
			//unsigned int initialPos = pos;
			/*if (0&&y==0) {
				uncompressed[pos++] = 1;
				uncompressed[pos++] = image->pixels[pos2].r;
				uncompressed[pos++] = image->pixels[pos2].g;
				uncompressed[pos++] = image->pixels[pos2].b;
				pos2++;
				for (x=1; x<image->width; x++) {
					uncompressed[pos] = image->pixels[pos2].r - image->pixels[pos2-1].r;
					pos++;
					uncompressed[pos] = image->pixels[pos2].g - image->pixels[pos2-1].g;
					pos++;
					uncompressed[pos] = image->pixels[pos2].b - image->pixels[pos2-1].b;
					pos++;
					pos2++;
				}
			}
			else {*/
			//unsigned int score0=lengths[0], score1=lengths[1], score2=lengths[2], score3=lengths[3], score4=lengths[4];
			//Color3* color=pixels;
			//unsigned int up=0, left=0, upleft=0;
#ifdef SEARCH_FILTERS
			unsigned int score0=0, score1=1, score2=2, score3=3, score4=4;
			unsigned char * pos = c;
			unsigned int up=0, left=0, upleft=0;
			unsigned int x1, x2, x3, x4;
				for (x=0; x<width3; x++) {
						if (y) {
							up = pos[-width3];
							if (x>2) {
								left = pos[-3];
								upleft = pos[-width3-3];
							}
						}
						else if (x) {
							left = pos[-3];
						}
						unsigned int val = pos[0];

						//x0 = val;
						x1 = (val - left)&0xFF;
						x2 = (val - up)&0xFF;
						x3 = (val - (up+left)/2)&0xFF;

						//int pa, pb, pc;
						__asm {
							mov eax, up
							mov edx, upleft
							mov ecx, left
							sub eax, edx
							sub ecx, edx
							mov edx, eax
							add eax, ecx
							// edx = pa
							// ecx = pb
							// eax = pc
							mov esi, edx
							mov edi, ecx
							neg esi
							cmovg edx, esi
							neg edi
							mov esi, eax
							cmovg ecx, edi
							neg esi
							mov edi, upleft
							cmovg eax, esi
							cmp ecx, eax
							cmovle edi, up
							cmovle eax, ecx
							cmp edx, eax
							mov eax, val
							cmovle edi, left
							sub eax, edi
							and eax, 0xFF
							mov x4, eax
						}
						score0+=abs(((char*)&val)[0]);
						score1+=abs(((char*)&x1)[0]);
						score2+=abs(((char*)&x2)[0]);
						score3+=abs(((char*)&x3)[0]);
						score4+=abs(((char*)&x4)[0]);
						
						//score0+=val;
						//score1+=x1;
						//score2+=x2;
						//score3+=x3;
						//score4+=x4;

						/*if (y) {
							if (val!=pos[-width3-1]) score0+=90;
							if (x1!=pos[-width3-1]) score1+=90;
							if (x2!=pos[-width3-1]) score2+=90;
							if (x3!=pos[-width3-1]) score3+=90;
							if (x4!=pos[-width3-1]) score4+=90;
						}
						if (x) {
							if (val!=pos[-3]) score0+=40;
							if (x1!=pos[-3]) score1+=40;
							if (x2!=pos[-3]) score2+=40;
							if (x3!=pos[-3]) score3+=40;
							if (x4!=pos[-3]) score4+=40;
						}//*/

					pos++;

				}
				//score0=score1=score2=score3=1;
				//score4=0;
				if (score0<=score1 && score0<=score2 && score1<=score3 && score1 <= score4) {
					uncompressed2++[0] = 0;
					for (x=0; x<width3; x++) {
						uncompressed2++[0] = c++[0];
					}
				}
				else if (score1<score2 && score1<score3 && score1 < score4) {
					uncompressed2++[0] = 1;
					int tx=0, ty=0, tz=0;
					for (x=0; x<width; x++) {
						uncompressed2[0] = c[0] - tx;
						tx = c[0];
						uncompressed2[1] = c[1] - ty;
						ty = c[1];
						uncompressed2[2] = c[2]- tz;
						tz = c[2];
						uncompressed2+=3;
						c+=3;
					}
				}
				else if (score2<score3 && score2 < score4) {
					uncompressed2++[0] = 2;
					int sx=0, sy=0, sz=0;
					for (x=0; x<width; x++) {
						if (y) {
							sx = c[-width3];
							sy = c[-width3+1];
							sz = c[-width3+2];
						}
						uncompressed2[0] = c[0] - sx;
						uncompressed2[1] = c[1] - sy;
						uncompressed2[2] = c[2] - sz;
						uncompressed2+=3;
						c+=3;
					}
				}
				else if (score3<score4) {
					uncompressed2++[0] = 3;
					int tx=0, ty=0, tz=0;
					int sx=0, sy=0, sz=0;
					for (x=0; x<width; x++) {
						if (x) {
							tx = c[-3];
							ty = c[-2];
							tz = c[-1];
						}
						if (y) {
							sx = c[-width3];
							sy = c[-width3+1];
							sz = c[-width3+2];
						}
						uncompressed2[0] = c[0] - (sx+tx)/2;
						uncompressed2[1] = c[1] - (sy+ty)/2;
						uncompressed2[2] = c[2] - (sz+tz)/2;
						uncompressed2+=3;
						c+=3;
					}
				}
				else {
			//up = upleft = left = 0;
			//x=0;
				/*else if (score1<score2 && score1<score3 && score1 < score4) {
					uncompressed2++[0] = 1;
					int tx=0, ty=0, tz=0;
					for (x=0; x<width; x++) {
						uncompressed2[0] = pixels->r - tx;
						uncompressed2[1] = pixels->g - ty;
						uncompressed2[2] = pixels->b - tz;
						uncompressed2+=3;
						tx = pixels->r;
						ty = pixels->g;
						tz = pixels->b;
						pixels++;
					}
				}*/
#endif
			uncompressed2++[0] = 4;
			//up = upleft = left = 0;
			//x=0;
				/*else if (score1<score2 && score1<score3 && score1 < score4) {
					uncompressed2++[0] = 1;
					int tx=0, ty=0, tz=0;
					for (x=0; x<width; x++) {
						uncompressed2[0] = pixels->r - tx;
						uncompressed2[1] = pixels->g - ty;
						uncompressed2[2] = pixels->b - tz;
						uncompressed2+=3;
						tx = pixels->r;
						ty = pixels->g;
						tz = pixels->b;
						pixels++;
					}
				}*/
			if (y) {
				uncompressed2[0] = c[0] - c[-width3];
				uncompressed2[1] = c[1] - c[1-width3];
				uncompressed2[2] = c[2] - c[2-width3];
				uncompressed2+=3;
				c+=3;
					__asm {
						mov ecx, width3
						mov esi, c
						xor eax, eax
						mov edi, esi
						//sub eax, 4
						sub edi, ecx
						//add esi, 3
						//and eax, 0xFFFFFFF0

						sub ecx, 3
						mov edx, uncompressed2
						jle stop
						//mov eax, width3
						//prefetcht0 [esi+2*eax]
//paethLoop:

						// upleft
						movdqu xmm0, [edi-3+eax]
						//mov esi, c
						//pxor xmm4, xmm4
						//or xmm3, xmm3
						// left
						movdqu xmm2, [esi-3+eax]
						// val
						//movdqu xmm3, [esi]
						// up
						movdqu xmm1, [edi+eax]


						movdqa xmm4, xmm0
						movdqa xmm5, xmm0
						movdqa xmm6, xmm0
						movdqa xmm7, xmm0

						// b/c, for left (2)
						pmaxub xmm4, xmm1
						pminub xmm5, xmm1

						// a/c, for top (1)
						pmaxub xmm6, xmm2
						pminub xmm7, xmm2

						psubusb xmm4, xmm5
						psubusb xmm6, xmm7

						pcmpeqb xmm5, xmm1
						pcmpeqb xmm7, xmm2


						movdqa xmm3, xmm4
						pxor xmm7, xmm5
						movdqa xmm5, xmm4

						pminub xmm3, xmm6
						pmaxub xmm5, xmm6

						psubb xmm5, xmm3
						pxor xmm3, xmm3
						pcmpeqb xmm7, xmm3
						movdqu xmm3, xmm4
						por xmm7, xmm5

						pminub xmm3, xmm6
						pminub xmm3, xmm7

						pcmpeqb xmm4, xmm3
						pcmpeqb xmm6, xmm3
						movdqa xmm5, xmm4
						pcmpeqb xmm7, xmm3

						pandn xmm5, xmm6
						//add edx, 16

						movdqa xmm6, xmm4

						pand xmm2, xmm4
						por xmm6, xmm5
						pand xmm5, xmm1
						pandn xmm6, xmm7

						movdqu xmm3, [esi+eax]
						pand xmm6, xmm0
						paddb xmm2, xmm5
						psubb xmm3, xmm6
						//add edi, 16
						//add esi, 16

						psubb xmm3, xmm2

						//sub ecx, 16
						movdqu [edx+eax], xmm3
						add eax, 16
						cmp ecx, eax
						jle stop

						add eax, edx
						and eax, ~15
						sub eax, edx

						movdqu xmm2, [esi-3+eax]
						movdqu xmm0, [edi-3+eax]
						movdqu xmm1, [edi+eax]
paethLoop:

						// upleft
						//mov esi, c
						//pxor xmm4, xmm4
						//or xmm3, xmm3
						// left
						// val
						//movdqu xmm3, [esi]
						// up


						movdqa xmm4, xmm0
						movdqa xmm5, xmm0
						movdqa xmm6, xmm0
						movdqa xmm7, xmm0

						// b/c, for left (2)
						pmaxub xmm4, xmm1
						pminub xmm5, xmm1

						// a/c, for top (1)
						pmaxub xmm6, xmm2
						pminub xmm7, xmm2

						psubusb xmm4, xmm5
						psubusb xmm6, xmm7

						pcmpeqb xmm5, xmm1
						pcmpeqb xmm7, xmm2


						pxor xmm7, xmm5
						pxor xmm3, xmm3
						pcmpeqb xmm7, xmm3

						movdqa xmm3, xmm4
						movdqa xmm5, xmm4

						pminub xmm3, xmm6
						pmaxub xmm5, xmm6

						psubb xmm5, xmm3
						//movdqa xmm3, xmm4
						por xmm7, xmm5

						//pminub xmm3, xmm6
						pminub xmm3, xmm7

						prefetcht0 [edi+eax+0x100]
						prefetcht0 [esi+eax+0x100]
						prefetcht0 [edx+eax+0x100]

						pcmpeqb xmm4, xmm3
						pcmpeqb xmm6, xmm3
						//movdqa xmm5, xmm4
						pcmpeqb xmm7, xmm3

						//add edx, 16

						//movdqa xmm6, xmm4

						pand xmm2, xmm4
						//por xmm4, xmm6
						pand xmm7, xmm0
						pand xmm1, xmm6
						pandn xmm6, xmm7

						movdqu xmm3, [esi+eax]
						movdqu xmm0, [edi+eax+13]
						paddb xmm6, xmm1
						//pandn xmm5, xmm1

						pandn xmm4, xmm6
						//paddb xmm4, xmm5
						paddb xmm4, xmm2
						movdqu xmm2, [esi+eax+13]
						movdqu xmm1, [edi+eax+16]
						psubb xmm3, xmm4
						//add edi, 16
						//add esi, 16

						//psubb xmm3, xmm5

						//sub ecx, 16
						movdqa [edx+eax], xmm3
						add eax, 16
						cmp ecx, eax
						jg paethLoop
stop:
//						mov ecx, width3
//						sub ecx, 3
						//mov ecx, width3
						//sub ecx, 3
						add esi, ecx
						add edx, ecx
						//mov ecx, width3

						mov c, esi
						mov uncompressed2, edx
						//mov x, ecx
						//mov x, eax
					}
				//}
				//uncompressed2+=x;
				//c+=x;
				//x = (width3-x);
			}
			//unsigned char *c = &pixels[0].r;
			else {
				int x=0;
				while (x<=2) {
					uncompressed2++[0] = c++[0];
					x++;
				}
				for (; x<width3; x++) {
					uncompressed2++[0] = c[0]-c[-3];
					c++;
				}
			}
#ifdef SEARCH_FILTERS
			}
#endif
		}
		len = uncompressed2 - uncompressed;
		/*if (j!=end-1) {
			counts[256] = 1;
			for (i=0; i<256; i++) {
				counts[i]=0;
			}
			for (i=0; i<len; i++) {
				counts[uncompressed[i]]++;
			}
			unsigned int temp = 257;
			if (j==0)
				GenLengths(counts, lengths, temp, 15);
			for (i=0; i<256; i++) {
				if (lengths[i]==0) lengths[i] = 15;
			}
		}//*/
		//break;
	//}
	//unsigned int test = UpdateAdler32(1L, uncompressed, lineLength*image->height);

	for (i=len; i<len+15; i++) {
		uncompressed[i]=0;
	}
	unsigned int compressedBytes = 0;
	unsigned char *compressed = DeflateSSE2(uncompressed, len, compressedBytes, lineLength);

	free(uncompressed);

	chunk.typei = 0x54414449;
	chunk.length = compressedBytes;
	chunk.data = compressed;
	WriteChunk(f, chunk, crcTable);
	//FILE *temp2 = fopen ("test.bin", "wb");
	//fwrite(&compressed[0], 1, compressedBytes, temp2);
	//fclose(temp2);
	free(compressed);

	chunk.typei = 0x444e4549;
	chunk.length = 0;
	chunk.data = 0;
	WriteChunk(f, chunk, crcTable);

	fclose(f);

}



void SavePNG(FILE *f, Image *image) {
	if (image->spp == 3) {
		ImageRGB *image2 = new ImageRGB(image->width, image->height, image->pixels);
		SavePngRGB(f, image2);
		image2->pixels = 0;
		delete image2;
	}
	else if (image->spp == 4) {
		ImageRGBA *image2 = new ImageRGBA(image->width, image->height, image->pixels);
		SavePngRGBA(f, image2);
		image2->pixels = 0;
		delete image2;
	}
}

#endif