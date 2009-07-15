#ifndef JPEG_H
#define JPEG_H


//#include "..\global.h"
//#include "..\DataSource.h"

#include "image.h"
#include "../ScriptValue.h"

int LoadJpeg(ScriptValue &sv, unsigned char *buffer, int len);

#endif