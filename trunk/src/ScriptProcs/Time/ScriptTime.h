#ifndef SCRIPTTIME_H
#define SCRIPTTIME_H
#include "../../ScriptValue.h"
void GetTime(ScriptValue &s, ScriptValue *args);
void ScriptGetTickCount(ScriptValue &s, ScriptValue *args);
void FormatTime(ScriptValue &s, ScriptValue *args);
void SetLocale(ScriptValue &s, ScriptValue *args);
#endif
