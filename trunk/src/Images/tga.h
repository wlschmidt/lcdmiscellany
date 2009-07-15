#include "Image.h"
#include "../ScriptValue.h"
#include <stdio.h>

int LoadTGA(ScriptValue &sv, unsigned char *buffer, int len);
int SaveTGA(FILE *file, GenericImage<unsigned char> *in);