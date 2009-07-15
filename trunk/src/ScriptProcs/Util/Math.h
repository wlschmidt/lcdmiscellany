#ifndef MATH_H
#define MATH_H
#include "../../ScriptValue.h"

void abs(ScriptValue &s, ScriptValue *args);
void log(ScriptValue &s, ScriptValue *args);
void log10(ScriptValue &s, ScriptValue *args);
void cos(ScriptValue &s, ScriptValue *args);
void sin(ScriptValue &s, ScriptValue *args);
void acos(ScriptValue &s, ScriptValue *args);
void asin(ScriptValue &s, ScriptValue *args);
void tan(ScriptValue &s, ScriptValue *args);
void atan(ScriptValue &s, ScriptValue *args);
void sqrt(ScriptValue &s, ScriptValue *args);
void exp(ScriptValue &s, ScriptValue *args);

void pow(ScriptValue &s, ScriptValue *args);

void rand(ScriptValue &s, ScriptValue *args);
void randf(ScriptValue &s, ScriptValue *args);
#endif
