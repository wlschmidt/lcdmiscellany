#ifndef GENCODE_H
#define GENCODE_H

struct TempCode;
#include "ScriptEnums.h"

int GenCode (TempCode *t, unsigned char *input, int len, unsigned char *fileName);

#endif
