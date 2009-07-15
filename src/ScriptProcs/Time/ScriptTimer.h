#ifndef SCRIPTTIMER_H
#define SCRIPTTIMER_H
#include "..\\ScriptValue.h"

void CleanupTimers();
void CreateFastTimer(ScriptValue &s, ScriptValue *args);
void CreateTimer(ScriptValue &s, ScriptValue *args);
void ModifyFastTimer(ScriptValue &s, ScriptValue *args);
void ModifyTimer(ScriptValue &s, ScriptValue *args);
void StartTimer(ScriptValue &s, ScriptValue *args);
void StopTimer(ScriptValue &s, ScriptValue *args);
void KillTimer(ScriptValue &s, ScriptValue *args);

#endif
