#ifndef SCRIPTYPES_H
#define SCRIPTYPES_H
#include "../../ScriptValue.h"

void IsNull(ScriptValue &s, ScriptValue *args);
void IsString(ScriptValue &s, ScriptValue *args);
void IsInt(ScriptValue &s, ScriptValue *args);
void IsFloat(ScriptValue &s, ScriptValue *args);
void IsList(ScriptValue &s, ScriptValue *args);
void IsDict(ScriptValue &s, ScriptValue *args);
void IsObject(ScriptValue &s, ScriptValue *args);
void IsNum(ScriptValue &s, ScriptValue *args);
void IsSimple(ScriptValue &s, ScriptValue *args);
void IsCompound(ScriptValue &s, ScriptValue *args);
void GetType(ScriptValue &s, ScriptValue *args);
void StackCount(ScriptValue &s, ScriptValue *args);
void Equals(ScriptValue &s, ScriptValue *args);
#endif
