#ifndef MUTABLE_STRING_H
#define MUTABLE_STRING_H

#include "../ScriptValue.h"
extern unsigned int MutableStringType;

void FreeMutableString(ScriptValue &s, ScriptValue *args);
void MutableStringReadInt(ScriptValue &s, ScriptValue *args);
void MutableStringReadInts(ScriptValue &s, ScriptValue *args);
void MutableStringReadInts(ScriptValue &s, ScriptValue *args);

extern unsigned int FileMappingType;

void MyOpenFileMapping(ScriptValue &s, ScriptValue *args);
void MyFreeFileMapping(ScriptValue &s, ScriptValue *args);
void FileMappingMapViewOfFile(ScriptValue &s, ScriptValue *args);

extern unsigned int MutableImageType;

void MutableStringLoadImage(ScriptValue &s, ScriptValue *args);

#endif
