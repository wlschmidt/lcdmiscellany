#ifndef LIST_H
#define LIST_H
#include "../../ScriptValue.h"

void insert(ScriptValue &s, ScriptValue *args);
void clear(ScriptValue &s, ScriptValue *args);
void resize(ScriptValue &s, ScriptValue *args);
void realloc(ScriptValue &s, ScriptValue *args);
void pop(ScriptValue &s, ScriptValue *args);
void pop_range(ScriptValue &s, ScriptValue *args);
void push_back(ScriptValue &s, ScriptValue *args);
void range(ScriptValue &s, ScriptValue *args);
void dictlist(ScriptValue &s, ScriptValue *args);
void dictvalues(ScriptValue &s, ScriptValue *args);
void dictkeys(ScriptValue &s, ScriptValue *args);
void indexsort(ScriptValue &s, ScriptValue *args);
void indexstablesort(ScriptValue &s, ScriptValue *args);
void sort(ScriptValue &s, ScriptValue *args);
#endif
