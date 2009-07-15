#ifndef SCRIPTSTRINGPARSER_H
#define SCRIPTSTRINGPARSER_H
#include "../../ScriptValue.h"

void ScriptToUpper(ScriptValue &s, ScriptValue *args);
void ScriptToLower(ScriptValue &s, ScriptValue *args);

void ParseBinaryInt(ScriptValue &s, ScriptValue *args);
void ParseBinaryInts(ScriptValue &s, ScriptValue *args);
void ParseBinaryIntReverse(ScriptValue &s, ScriptValue *args);

//void ParseBinaryDouble(ScriptValue &s, ScriptValue *args);
void ParseBinaryFloat(ScriptValue &s, ScriptValue *args);

void ParseBinaryStringASCII(ScriptValue &s, ScriptValue *args);
void ParseBinaryStringUTF8(ScriptValue &s, ScriptValue *args);
void ParseBinaryStringUTF16(ScriptValue &s, ScriptValue *args);

void ParseBinaryBinary(ScriptValue &s, ScriptValue *args);

void UTF8toASCII(ScriptValue &s, ScriptValue *args);
void UTF8toUTF16(ScriptValue &s, ScriptValue *args);
void UTF16toUTF8(ScriptValue &s, ScriptValue *args);
void ASCIItoUTF8(ScriptValue &s, ScriptValue *args);

void SaveString(ScriptValue &s, ScriptValue *args);
void GetString(ScriptValue &s, ScriptValue *args);

void MyStrstr(ScriptValue &s, ScriptValue *args);
void MyStristr(ScriptValue &s, ScriptValue *args);
void strsplit(ScriptValue &s, ScriptValue *args);

void strreplace(ScriptValue &s, ScriptValue *args);
void substring(ScriptValue &s, ScriptValue *args);

void strinsert(ScriptValue &s, ScriptValue *args);
void strswap(ScriptValue &s, ScriptValue *args);


void GetChar(ScriptValue &s, ScriptValue *args);

#endif
