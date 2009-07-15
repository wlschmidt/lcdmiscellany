#pragma once
#include <stdio.h>
//#include <stdlib.h>
//#include "..\DataSource.h"
//#include "FileHandler.h"

//#include "util.h"
#include "image.h"
#include "../ScriptValue.h"


int LoadFileBMP(ScriptValue &sv, unsigned char *in, int size);
int LoadMemoryBMP(ScriptValue &sv, unsigned char *in, int size, unsigned int seekPos = 0);
int SaveBMP(FILE *in, GenericImage<unsigned char> *image);

/*class BMP {
	//Header head;
	//static char extension[4];
	static Header * LoadHeader(HANDLE hFile, tagBITMAPFILEHEADER *fileHeader, tagBITMAPINFO **info);
public:
	static int SaveAs(FILE * file, Image *image, int mask);
	static Header * LoadHeader(HANDLE hFile);
	static FileData * LoadFile (HANDLE hFile);
	static int CheckExtension(unsigned short *extension) {
		return !noCaseCmp(extension, L"BMP");
	}
};
*/
