#pragma once

#include "..\global.h"
#include <malloc.h>

unsigned char * Inflate(unsigned char* compressed, unsigned int compressedBytes, unsigned int size, unsigned int &uncompressedBytes, int gzip);
char *InflateGzip(char *compressed, unsigned int compressedBytes, unsigned int &uncompressedBytes);