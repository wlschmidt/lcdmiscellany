#include "../../ScriptValue.h"
void ScriptGetKeyState(ScriptValue &s, ScriptValue *args);
void KeyDown(ScriptValue &s, ScriptValue *args);
void KeyUp(ScriptValue &s, ScriptValue *args);

void RegisterKeyEvent(ScriptValue &s, ScriptValue *args);
void UnregisterKeyEvent(ScriptValue &s, ScriptValue *args);
void CleanKeyEvents();
