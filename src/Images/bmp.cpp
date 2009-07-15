#include "bmp.h"
//#include "util.h"
#include <windows.h>
#include <malloc.h>

/*Header * BMP::LoadHeader(HANDLE hFile) {
	tagBITMAPFILEHEADER fileHeader;
	tagBITMAPINFO *info=0;
	Header * out = LoadHeader(hFile, &fileHeader, &info);
	if (info) free(info);
	return out;
}*/

/*Header * BMP::LoadHeader(HANDLE hFile, tagBITMAPFILEHEADER *fileHeader, tagBITMAPINFO **info) {
	Header * out = 0;
	long size;
	ReadFile(hFile, fileHeader, sizeof(tagBITMAPFILEHEADER), (unsigned long*)&size, 0);
	if (size<sizeof(tagBITMAPFILEHEADER) ||
		fileHeader->bfType!=((short int *)"BM")[0] ||
		fileHeader->bfReserved1!=0 ||
		fileHeader->bfReserved2!=0) {
		return 0;
	}

	long int temp;

	ReadFile(hFile, &temp, 4, (unsigned long*)&size, 0);
	if (size<4) return 0;
	*info = (tagBITMAPINFO*) malloc(temp);
	SetFilePointer(hFile, -4, 0, 1);

	ReadFile(hFile, *info, temp,(unsigned long*) &size, 0);

	out = new Header();
	out->depth = info[0]->bmiHeader.biBitCount;
	out->width = info[0]->bmiHeader.biWidth;
	out->height = info[0]->bmiHeader.biHeight;
	strcpy(out->format, "BMP");
	out->supported=-1;
	out->type = TYPE_IMAGE;
	if (size==temp) {
		out->supported=0;
		if (info[0]->bmiHeader.biPlanes==1) {
			if (info[0]->bmiHeader.biCompression==BI_RGB) {
				if (info[0]->bmiHeader.biBitCount==24 ||
					info[0]->bmiHeader.biBitCount==8 ||
					info[0]->bmiHeader.biBitCount==4 ||
					info[0]->bmiHeader.biBitCount==1)
					out->supported = 1;
			}
			else if (info[0]->bmiHeader.biCompression==BI_RLE8 &&
				info[0]->bmiHeader.biBitCount==8) {
					out->supported = 1;
			}
			else if (info[0]->bmiHeader.biCompression==BI_RLE4 &&
				info[0]->bmiHeader.biBitCount==4) {
					out->supported = 1;
			}
		}
	}
	return out;
}*/

int LoadMemoryBMP(ScriptValue &sv, BITMAPINFO *info, unsigned char*data, unsigned int len) {
	int happy=0;
	//int types[] = {BI_RGB,BI_RLE8,BI_RLE4,BI_BITFIELDS,BI_JPEG,BI_PNG};
	if (info->bmiHeader.biCompression==BI_RGB) {
		if (info->bmiHeader.biBitCount==32 ||
			info->bmiHeader.biBitCount==24 ||
			info->bmiHeader.biBitCount==8 ||
			info->bmiHeader.biBitCount==4 ||
			info->bmiHeader.biBitCount==1)
			happy = 1;
	}
	else if ((info->bmiHeader.biCompression==BI_RLE8) &&
		info->bmiHeader.biBitCount==8) {
			happy = 1;
	}
	else if (info->bmiHeader.biCompression==BI_RLE4 &&
		info->bmiHeader.biBitCount==4) {
			happy = 1;
	}
	if (info->bmiHeader.biBitCount == 8 && (unsigned)info->bmiHeader.biClrUsed > 256)
		happy = 0;
	if (info->bmiHeader.biBitCount == 4 && (unsigned)info->bmiHeader.biClrUsed > 16)
		happy = 0;

	if (!happy) {
		return 0;
	}

	unsigned long width = info->bmiHeader.biWidth;
	unsigned long height = info->bmiHeader.biHeight;
	if (width > 10000 || height > 10000 || width*height > 50000000) return 0;
	if (!MakeGenericImage<unsigned char>(sv, width, height, 3 + (info->bmiHeader.biBitCount==32))) return 0;
	GenericImage<unsigned char> *image = (GenericImage<unsigned char>*) sv.stringVal->value;
	unsigned long i;
	unsigned long j;
	if (info->bmiHeader.biBitCount==24 || info->bmiHeader.biBitCount==32) {
		unsigned int bmpLineLen = image->spp*image->width;
		unsigned int bmpMemLineLen = (3+bmpLineLen)&~3;
		unsigned int junk = (((4-(info->bmiHeader.biBitCount/8)*image->width)&3))&~3;
		int orMask = 0xFF000000;
		unsigned int mul = image->spp;
		unsigned int lineLen = width*mul;
		unsigned char *pos = image->pixels + (height-1) * image->memWidth;
		while(pos >= image->pixels) {
			if (len < bmpLineLen) {
				memset(pos, 0, lineLen);
				continue;
			}
			for (unsigned int x = 0; x<lineLen; x+=mul) {
				pos[x] = data[x];
				pos[x+1] = data[x+1];
				pos[x+2] = data[x+2];
				if (mul == 4)
					pos[x+3] = data[x+3];
			}
			len -= bmpMemLineLen;
			data += bmpMemLineLen;
			pos -= image->memWidth;
		}
	}
	else {
		unsigned char smask = 0x80, mask = 0x1;
		unsigned int numColors=2;
		int depth = info->bmiHeader.biBitCount;
		if (depth==8) {
			mask = smask = 0xFF;
			numColors = info->bmiHeader.biClrUsed;
		}
		else if (depth==4) {
			smask = 0xF0;
			mask = 0xF;
			numColors = info->bmiHeader.biClrUsed;
		}
		int inv = 8/depth;

		unsigned char byte;
		unsigned int bit=0;
		int compression = info->bmiHeader.biCompression;
		Color4 * color = (Color4*) info->bmiColors;
		if (compression == BI_RLE8 || compression == BI_RLE4) {
			memset(image->pixels, 0, height * image->memWidth);
			//line = image->pixels[new unsigned char[fileHeader.bfSize-fileHeader.bfOffBits];
			//ReadFile(hFile, line, fileHeader.bfSize-fileHeader.bfOffBits, (unsigned long*)&size, 0);
			//fread(line, fileHeader.bfSize-fileHeader.bfOffBits, 1, in);
			//unsigned char *data = line;
			//unsigned long pos=2;
			i=height-1;
			unsigned long j=0;
			//unsigned char bits[10];
			while (len >= 2) {
				unsigned char arg1 = data[0];
				unsigned char arg2 = data[1];
				data += 2;
				len -= 2;
				if (arg1) {
					if (compression == BI_RLE8) {
						for (int t=0; t<arg1; t++) {
							if (j<width && i<height && j>=0 && i>=0) {
								((Color3*)&image->pixels)[i*image->memWidth+j] = color[arg2];
							}
							j++;
						}
					}
					else {
						for (int t=0; t<arg1; t++) {
							if (j<width && i<height && j>=0 && i>=0) {
								((Color3*)&image->pixels)[i*image->memWidth+j] = color[arg2>>4];
							}
							j++;
							t++;
							if (t<arg1) {
								if (j<width && i<height && j>=0 && i>=0) {
									((Color3*)&image->pixels)[i*image->memWidth+j] = color[arg2&0xF];
								}
								j++;
							}
						}
					}
				}
				else if (arg2>2) {
					if (compression == BI_RLE8) {
						for (int t=0; t<arg2; t++) {
							if (!len) break;
							if (j<width && i<height && j>=0 && i>=0) {
								((Color3*)&image->pixels)[i*image->memWidth+j] = color[data[0]];
							}
							len--;
							data++;
							j++;
						}
					}
					else {
						for (int t=0; t<arg2; t++) {
							if (!len) break;
							if (j<width && i<height && j>=0 && i>=0) {
								((Color3*)&image->pixels)[i*image->memWidth+j] = color[data[0]>>4];
							}
							j++;
							t++;
							if (t<arg2) {
								if (j<width && i<height && j>=0 && i>=0) {
									((Color3*)&image->pixels)[i*image->memWidth+j] = color[data[0]&0xF];
								}
								j++;
							}
							len--;
							data++;
						}
					}
				}
				else if (arg2==2) {
					if (len >= 2) {
						j+=data[0];
						i-=data[1];
						data+=2;
						len-=2;
					}
					else break;
				}
				else if (arg2==0) {
					i--;
					j=0;
				}
				else if (arg2==1) {
					break;
				}
			}
		}
		else {
			unsigned int lineLen = (image->width+(inv-1))/inv;
			unsigned int memLineLen = (lineLen+3)&0xFFFFFFFC;
			//unsigned char *line = &image->pixels[height*image->memWidth - memLineLen];
			for (i=height; i--; ) {
				unsigned char *pos = image->pixels+i*image->memWidth;
				if (len<lineLen) {
					memset(pos, 0, image->memWidth);
					continue;
				}
				//ReadFile(hFile, line, ((image->width+(inv-1))/inv+3)&0xFFFFFFFCL, (unsigned long*)&size, 0);
				//fread(line, lineLen, 1, in);
				//in->Read(line, lineLen);
				//unsigned int temp = size*8/depth;
				//unsigned char *line = data;
				bit=0;
				for (j=0; j<lineLen; j++) {
					//fread(&byte,1,1,in);
					unsigned int k=smask;
					byte = data[j];
					int shift = 8-depth;
					while (k && bit<width) {
						//if (bit<temp)
						((Color3*)pos)[bit] = color[((k&byte)>>shift)&mask];
						//else {
						//	((Color3*)pos)[bit].r=0;
						//	((Color3*)pos)[bit].g=0;
						//	((Color3*)pos)[bit].b=0;
							//image->pixels[i*width+bit].a=0;
						//}
						bit++;
						k>>=depth;
						shift-=depth;
					}
				}
				data += memLineLen;
				len -= memLineLen;
			}
			//padRead(hFile, &j);
		}
	}
	return 1;
}
//*/

struct SBITMAPINFO : public BITMAPINFO {
	RGBQUAD colorSpace[256*4];
};

int LoadMemoryBMP(ScriptValue &sv, unsigned char *in, int len, unsigned int seekPos) {
	BITMAPINFO* info3 = (BITMAPINFO*) in;
	SBITMAPINFO info;
	//RGBQUAD colorSpace[256*4];

	//unsigned long temp;

	if (len<4 || info3->bmiHeader.biSize < 24 || (int)info3->bmiHeader.biSize > len || info3->bmiHeader.biClrUsed > 256) return 0;
	info.bmiHeader = info3->bmiHeader;
	len -= info.bmiHeader.biSize;
	in += info.bmiHeader.biSize;

	int colors = info.bmiHeader.biClrUsed;
	if (!colors) {
		if (info.bmiHeader.biBitCount == 4)
			colors = 16;
		if (info.bmiHeader.biBitCount == 8)
			colors = 256;
	}
	if (info.bmiHeader.biCompression == BI_BITFIELDS) {
		if (len < 12) return 0;
		memcpy(info.bmiColors, in, 12);
		len -= 12;
		in += 12;
	}
	else if (colors) {
		if (len < 4*colors) {
			return 0;
		}
		memset(info.bmiColors+colors, 0, 4*(256-colors));
		memcpy(info.bmiColors, in, 4*colors);
		len -= 4*colors;
		in += 4*colors;
		for (int i=0; i<colors; i++) {
			unsigned char w = info.bmiColors[i].rgbRed;
			info.bmiColors[i].rgbRed = info.bmiColors[i].rgbGreen;
			info.bmiColors[i].rgbGreen = w;
		}
	}
	if (seekPos) {
		seekPos -= info.bmiHeader.biSize;
		seekPos -= colors*4;
		if (seekPos< 0) {
			return 0;
		}
		while (seekPos--) {
			in++;
			len--;
		}
		if (len < 0) return 0;
	}
	unsigned int rsize = info.bmiHeader.biSizeImage;
	if (info.bmiHeader.biSizeImage == 0) {
		if (info.bmiHeader.biCompression == BI_RGB)
			rsize = (((info.bmiHeader.biBitCount/8)*info.bmiHeader.biWidth + 3)&~3)*info.bmiHeader.biHeight;
	}
	if (rsize > 150000000) {
		return 0;
	}
	//int len;
	//unsigned char * data = in->GetRestOfData(&len);
	//unsigned char* data = (unsigned char*)malloc(rsize);
	if (len<(int)rsize) {
		//free(data);
		return 0;
	}
	
	if (LoadMemoryBMP(sv, &info, in, rsize)) {
		//free(data);
		return 1;
	}
	if (colors) {
		for (int i=0; i<colors; i++) {
			unsigned char w = info.bmiColors[i].rgbRed;
			info.bmiColors[i].rgbRed = info.bmiColors[i].rgbGreen;
			info.bmiColors[i].rgbGreen = w;
		}
	}
	//unsigned char *test = (unsigned char*) malloc(rsize+colors*4);
	//memcpy(test, info->bmiColors, colors*4);
	//memcpy(test+colors*4, data, rsize);

	tagBITMAPINFO info2;
	memset(&info2, 0, sizeof(info2));
	info2.bmiHeader.biCompression = BI_RGB;
	info2.bmiHeader.biSize = sizeof(info2);
	info2.bmiHeader.biWidth = info.bmiHeader.biWidth;
	info2.bmiHeader.biHeight = info.bmiHeader.biHeight;
	info2.bmiHeader.biXPelsPerMeter = 3200;
	info2.bmiHeader.biYPelsPerMeter = 3200;
	info2.bmiHeader.biPlanes = 1;
	info2.bmiHeader.biBitCount = 24;
	HDC hDC = CreateCompatibleDC(0);
	int res = 0;
	if (hDC) {
		void *data2 = 0;
		HBITMAP hBitMap = CreateDIBSection(hDC, &info2, DIB_RGB_COLORS, (void**)&data2, 0, 0);
		if (hBitMap) {
			if (data2) {
				HANDLE hPrevBitMap = SelectObject(hDC, hBitMap);
				//StretchDIBits(hDC,0,0,info2.bmiHeader.biWidth,info2.bmiHeader.biHeight,0,0,info2.bmiHeader.biWidth,info2.bmiHeader.biHeight, data, info, DIB_RGB_COLORS, SRCCOPY);
				SetDIBits(hDC, hBitMap, 0, info2.bmiHeader.biHeight, in, &info, DIB_RGB_COLORS);
				GdiFlush();
				res = MakeGenericImage<unsigned char>(sv, info2.bmiHeader.biWidth, info2.bmiHeader.biHeight, 3);
				if (res) {
					GenericImage<unsigned char>* out = (GenericImage<unsigned char>*)sv.stringVal->value;
					memset(out->pixels, 0, out->memWidth*out->height);
					for (int y=info2.bmiHeader.biHeight; y--;) {
						unsigned char*out2 = &out->pixels[y*out->memWidth];
						//unsigned char*out3 = &temp2->pixels[y*out->memWidth];
						unsigned char*in = (unsigned char*) data2+(info2.bmiHeader.biHeight-1-y)*((out->width*3+3)&~3);
						for (int x=0; x<info2.bmiHeader.biWidth;x++) {
							out2[0] = in[0];
							out2[1] = in[1];
							out2[2] = in[2];
							//out2[3] = in[3];
							//if (((out2[2]&out2[1])&out2[0]) <255)
							//	out[2]=out[2];
							out2+=3;
							//out3+=3;
							in+=3;
						}
					}
				}

				SelectObject(hDC, hPrevBitMap);
			}
			DeleteObject(hBitMap);
		}
		DeleteDC(hDC);
	}
	//free(data);
	//free(test);
	return res;
}
/*
GenericImage<unsigned char> *LoadMemoryBMP(DataSource *in, unsigned int seekPos) {
	unsigned int size;
	tagBITMAPINFO info2;
	{
		tagBITMAPINFO *info;

		unsigned long temp;

		size = in->Read(&temp, 4);

		if (size<4 || temp < 24) return 0;
		info = (tagBITMAPINFO*) malloc(temp);
		if (!info) return 0;

		info->bmiHeader.biSize = temp;
		size = in->Read(&info->bmiHeader.biWidth, temp-4);

		int happy=0;
		if (size==temp-4 && info->bmiHeader.biPlanes==1 &&
			(!seekPos || seekPos >= temp)) {
				int types[] = {BI_RGB,BI_RLE8,BI_RLE4,BI_BITFIELDS,BI_JPEG,BI_PNG};
			if (info->bmiHeader.biCompression==BI_RGB) {
				if (info->bmiHeader.biBitCount==32 ||
					info->bmiHeader.biBitCount==24 ||
					info->bmiHeader.biBitCount==8 ||
					info->bmiHeader.biBitCount==4 ||
					info->bmiHeader.biBitCount==1)
					happy = 1;
			}
			else if ((info->bmiHeader.biCompression==BI_RLE8) &&
				info->bmiHeader.biBitCount==8) {
					happy = 1;
			}
			else if (info->bmiHeader.biCompression==BI_RLE4 &&
				info->bmiHeader.biBitCount==4) {
					happy = 1;
			}
			if (info->bmiHeader.biBitCount == 8 && (unsigned)info->bmiHeader.biClrUsed > 256)
				happy = 0;
			if (info->bmiHeader.biBitCount == 4 && (unsigned)info->bmiHeader.biClrUsed > 16)
				happy = 0;
		}
		info2 = *info;
		free(info);
		if (!happy) {
			return 0;
		}
	}

	unsigned long width = info2.bmiHeader.biWidth;
	unsigned long height = info2.bmiHeader.biHeight;
	if (width > 10000 || height > 10000 || width*height > 50000000) return 0;
	GenericImage<unsigned char> *image = MakeGenericImage<unsigned char>(width, height, 3 + (info2.bmiHeader.biBitCount==32));
	if (!image) return 0;
	unsigned long i;
	unsigned long j;
	if (info2.bmiHeader.biBitCount==24 || info2.bmiHeader.biBitCount==32) {
		if (seekPos) {
			seekPos -= info2.bmiHeader.biSize;
			char junk;
			while (seekPos--) {
				in->Read(&junk, 1);
			}
		}
		int orMask = 0xFF000000;
		unsigned int mul = image->spp;
		unsigned int lineLen = width*mul;
		unsigned char *pos = image->pixels + (height-1) * image->memWidth;
		while(pos >= image->pixels) {
			in->Read(pos, lineLen);
			for (unsigned int x = 0; x<lineLen; x+=mul) {
				unsigned char t = pos[x];
				pos[x] = pos[x+2];
				pos[x+2] = t;
			}
			if (lineLen&3) {
				in->Read(pos+lineLen, 4-(lineLen&3));
			}
			pos -= image->memWidth;
		}
	}
	else {
		unsigned char smask = 0x80, mask = 0x1;
		unsigned int numColors=2;
		int depth = info2.bmiHeader.biBitCount;
		if (depth==8) {
			mask = smask = 0xFF;
			numColors = info2.bmiHeader.biClrUsed;
		}
		else if (depth==4) {
			smask = 0xF0;
			mask = 0xF;
			numColors = info2.bmiHeader.biClrUsed;
		}
		int inv = 8/depth;
		RGBQUAD quadColors[256];
		Color3 color[256];
		memset(quadColors, 0, sizeof(quadColors));
		//info = (tagBITMAPINFO *)realloc(info, info->bmiHeader.biSize + numColors * sizeof(tagRGBQUAD));
		//ReadFile(hFile, info->bmiColors, numColors * sizeof(tagRGBQUAD), (unsigned long*)&size, 0);
		size = in->Read(quadColors, numColors * sizeof(RGBQUAD));

		//if (size<numColors * (int)sizeof(tagRGBQUAD)) {
		if (seekPos)
			seekPos -= size + info2.bmiHeader.biSize;
		if (size<numColors * sizeof(RGBQUAD) ||
			seekPos < 0) {
			image->Cleanup();
			return 0;
		}
		char junk;
		while (seekPos--) {
			in->Read(&junk, 1);
		}
		unsigned char byte;
		//Color3 color[256];
		for (j=0; j<numColors; j++) {
			color[j].r = quadColors[j].rgbRed;
			color[j].g = quadColors[j].rgbGreen;
			color[j].b = quadColors[j].rgbBlue;
			//color[j].a = 255;
		}
		//SetFilePointer(hFile, fileHeader.bfOffBits, 0, 0);
		unsigned int bit=0;
		if (info2.bmiHeader.biCompression == BI_RLE8 || info2.bmiHeader.biCompression == BI_RLE4) {
			memset(image->pixels, 0, height * image->memWidth);
			//line = image->pixels[new unsigned char[fileHeader.bfSize-fileHeader.bfOffBits];
			//ReadFile(hFile, line, fileHeader.bfSize-fileHeader.bfOffBits, (unsigned long*)&size, 0);
			//fread(line, fileHeader.bfSize-fileHeader.bfOffBits, 1, in);
			//unsigned char *data = line;
			//unsigned long pos=2;
			i=height-1;
			unsigned long j=0;
			unsigned char bits[10];
			while (in->Read(bits, 2) == 2) {
				if (bits[0]) {
					if (info2.bmiHeader.biCompression == BI_RLE8) {
						for (int t=0; t<bits[0]; t++) {
							if (j<width && i<height && j>=0 && i>=0) {
								((Color3*)&image->pixels)[i*image->memWidth+j] = color[bits[1]];
							}
							j++;
						}
					}
					else {
						for (int t=0; t<bits[0]; t++) {
							if (j<width && i<height && j>=0 && i>=0) {
								((Color3*)&image->pixels)[i*image->memWidth+j] = color[bits[1]>>4];
							}
							j++;
							t++;
							if (t<bits[0]) {
								if (j<width && i<height && j>=0 && i>=0) {
									((Color3*)&image->pixels)[i*image->memWidth+j] = color[bits[1]&0xF];
								}
								j++;
							}
						}
					}
				}
				else if (bits[1]>2) {
					if (info2.bmiHeader.biCompression == BI_RLE8) {
						for (int t=0; t<bits[1]; t++) {
							if (in->Read(bits+2, 1) != 1) break;
							if (j<width && i<height && j>=0 && i>=0) {
								((Color3*)&image->pixels)[i*image->memWidth+j] = color[bits[2]];
							}
							j++;
						}
					}
					else {
						for (int t=0; t<bits[1]; t++) {
							if (in->Read(bits+2, 1) != 1) break;
							if (j<width && i<height && j>=0 && i>=0) {
								((Color3*)&image->pixels)[i*image->memWidth+j] = color[bits[2]>>4];
							}
							j++;
							t++;
							if (t<bits[0]) {
								if (j<width && i<height && j>=0 && i>=0) {
									((Color3*)&image->pixels)[i*image->memWidth+j] = color[bits[2]&0xF];
								}
								j++;
							}
						}
					}
				}
				else if (bits[1]==2) {
					if (in->Read(bits+2, 2) == 2) {
						j+=bits[2];
						i-=bits[3];
					}
				}
				else if (bits[1]==0) {
					i--;
					j=0;
				}
				else if (bits[1]==1) {
					break;
				}
			}
		}
		else {
			unsigned int lineLen = (image->width+(inv-1))/inv;
			unsigned int memLineLen = (lineLen+3)&0xFFFFFFFC;
			unsigned char *line = &image->pixels[height*image->memWidth - memLineLen];
			for (i=height-1; i>=0; i--) {
				//ReadFile(hFile, line, ((image->width+(inv-1))/inv+3)&0xFFFFFFFCL, (unsigned long*)&size, 0);
				//fread(line, lineLen, 1, in);
				in->Read(line, lineLen);
				//unsigned int temp = size*8/depth;
				bit=0;
				unsigned char *pos = image->pixels+i*image->memWidth;
				for (j=0; j<lineLen; j++) {
					//fread(&byte,1,1,in);
					unsigned int k=smask;
					byte = line[j];
					int shift = 8-depth;
					while (k && bit<width) {
						//if (bit<temp)
						((Color3*)pos)[bit] = color[((k&byte)>>shift)&mask];
						//else {
						//	((Color3*)pos)[bit].r=0;
						//	((Color3*)pos)[bit].g=0;
						//	((Color3*)pos)[bit].b=0;
							//image->pixels[i*width+bit].a=0;
						//}
						bit++;
						k>>=depth;
						shift-=depth;
					}
				}
			}
			//padRead(hFile, &j);
		}
	}
	return image;
}
//*/
int LoadFileBMP(ScriptValue &sv, unsigned char *in, int len) {
	//Header* head = LoadHeader(hFile, &fileHeader, &info);
	if (len < sizeof(tagBITMAPFILEHEADER)) return 0;
	tagBITMAPFILEHEADER fileHeader = *(tagBITMAPFILEHEADER*)in;
	in += sizeof(tagBITMAPFILEHEADER);
	len -= sizeof(tagBITMAPFILEHEADER);
	//long size = in->Read(&fileHeader, sizeof(tagBITMAPFILEHEADER));
	//ReadFile(hFile, fileHeader, sizeof(tagBITMAPFILEHEADER), (unsigned long*)&size, 0);
	if (fileHeader.bfType!=((short int *)"BM")[0] ||
		fileHeader.bfReserved1!=0 ||
		fileHeader.bfReserved2!=0) {
		return 0;
	}
	return LoadMemoryBMP(sv, in, len, fileHeader.bfOffBits-sizeof(fileHeader));
}

//#ifdef CHICKEN
//char BMP::extension[4]="BMP";
int SaveBMP(FILE *file, GenericImage<unsigned char> *in) {
	if (!file) return 0;
	unsigned int i, j;
	tagBITMAPINFO info;
	tagBITMAPFILEHEADER fileHeader;
	char z[4];
	*(long*)z = 0;

	info.bmiHeader.biWidth = in->width;
	info.bmiHeader.biHeight = in->height;


	fileHeader.bfType=((short int *)"BM")[0];
	//fileHeader.bfSize=258;
	fileHeader.bfReserved1=0;
	fileHeader.bfReserved2=0;

	int offset=sizeof(fileHeader);
	info.bmiHeader.biCompression=BI_RGB;
	info.bmiHeader.biBitCount=24;
	info.bmiHeader.biSizeImage=0;
	info.bmiHeader.biXPelsPerMeter=0;
	info.bmiHeader.biYPelsPerMeter=0;
	info.bmiHeader.biClrUsed=0;
	info.bmiHeader.biClrImportant=0;


	info.bmiHeader.biSize = sizeof(tagBITMAPINFOHEADER);
	info.bmiHeader.biPlanes=1;

	offset+=sizeof(info)-4;

	fileHeader.bfOffBits=offset;
	info.bmiHeader.biSizeImage = info.bmiHeader.biHeight * ((3*info.bmiHeader.biWidth+3)&~3);
	fileHeader.bfSize = offset + info.bmiHeader.biSizeImage;
	//unsigned long size;
	fwrite(&fileHeader, sizeof(fileHeader), 1, file);
	//if (size != sizeof(fileHeader)) return BAD_FILE;
	fwrite(&info, sizeof(info)-4, 1, file);
	//if (size != sizeof(info)-4) return BAD_FILE;
	/*fwrite(&fileHeader, sizeof(fileHeader), 1, out);
	fwrite(&info, sizeof(info)-4, 1, out);*/

	long lineSize = (3*in->width+3)&~3;
	unsigned char*line = (unsigned char*) alloca(lineSize);
	if (!line) return 0;
	memset(line, 0, lineSize);
	if (in->spp >= 3) {
		for (i=in->height; i--;) {
			int p = 0;
			for (j=0; j<in->width; j++) {
				line[p++] = in->pixels[i*in->memWidth + j*in->spp]; // b
				line[p++] = in->pixels[i*in->memWidth + j*in->spp+1]; // g
				line[p++] = in->pixels[i*in->memWidth + j*in->spp+2];   // r
				//alpha |= 255 - in->pixels[i*in->width + j].a;
			}
			fwrite(line, 1, lineSize, file);
			//if (size != lineSize) return BAD_FILE;
		}
	}
	else {
		for (i=in->height; i--;) {
			int p = 0;
			for (j=0; j<in->width; j++) {
				line[p] =
					line[p+1] =
					line[p+2] = in->pixels[i*in->memWidth + j*in->spp];
				p+=3;
				//alpha |= 255 - in->pixels[i*in->width + j].a;
			}
			fwrite(line, 1, lineSize, file);
			//if (size != lineSize) return BAD_FILE;
		}
	}
	fclose(file);
	//if (alpha)
	//	return IMAGE_COLOR;
	//else return IMAGE_COLOR|IMAGE_ALPHA;
	return 1;
}
//#endif