#include "../../ScriptValue.h"

struct Stack;

void ScriptSetCursorPos(ScriptValue &s, ScriptValue *args);
void ScriptGetCursorPos(ScriptValue &s, ScriptValue *args);
void ScriptClipCursor(ScriptValue &s, ScriptValue *args);
void MouseMove(ScriptValue &s, ScriptValue *args);
void MouseDown(ScriptValue &s, ScriptValue *args);
void MouseUp(ScriptValue &s, ScriptValue *args);
void MouseScroll(ScriptValue &s, ScriptValue *args);

int ContextMenuWait(ScriptValue *args, Stack *stack);
