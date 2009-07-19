// Feel free to use this code however you wish.

#include "DllTypes.h"

void MakeStaticStringValue(ScriptVal *sv, wchar_t *value) {
	memset(sv, 0, sizeof(ScriptVal));
	sv->wideStringValue.value = value;
	sv->wideStringValue.len = (int)wcslen(value);
	sv->type = SCRIPT_WIDE_STRING;
}

void MakeStaticStringValue(ScriptVal *sv, char *value) {
	memset(sv, 0, sizeof(ScriptVal));
	sv->asciiStringValue.value = value;
	sv->asciiStringValue.len = (int)strlen(value);
	sv->type = SCRIPT_ASCII_STRING;
}

void MakeStaticStringValue(ScriptVal *sv, unsigned char *value) {
	memset(sv, 0, sizeof(ScriptVal));
	sv->stringValue.value = value;
	sv->stringValue.len = (int)strlen((char*)value);
	sv->type = SCRIPT_STRING;
}



int MakeStringValue(ScriptVal *sv, wchar_t *value) {
	memset(sv, 0, sizeof(ScriptVal));
	if (!(sv->wideStringValue.value = wcsdup(value))) return 0;
	sv->wideStringValue.len = (int)wcslen(value);
	sv->flags = FLAG_NEED_FREE_DLL;
	sv->type = SCRIPT_WIDE_STRING;
	return 1;
}

int MakeStringValue(ScriptVal *sv, char *value) {
	memset(sv, 0, sizeof(ScriptVal));
	if (!(sv->asciiStringValue.value = strdup(value))) return 0;
	sv->asciiStringValue.len = (int)strlen(value);
	sv->flags = FLAG_NEED_FREE_DLL;
	sv->type = SCRIPT_ASCII_STRING;
	return 1;
}

int MakeStringValue(ScriptVal *sv, unsigned char *value) {
	memset(sv, 0, sizeof(ScriptVal));
	if (!(sv->stringValue.value = (unsigned char*)strdup((char*)value))) return 0;
	sv->stringValue.len = (int)strlen((char*)value);
	sv->flags = FLAG_NEED_FREE_DLL;
	sv->type = SCRIPT_STRING;
	return 1;
}



void MakeIntValue(ScriptVal *sv, __int64 value) {
	memset(sv, 0, sizeof(ScriptVal));
	sv->intValue = value;
	sv->type = SCRIPT_INT;
}

void MakeDoubleValue(ScriptVal *sv, double value) {
	memset(sv, 0, sizeof(ScriptVal));
	sv->doubleValue = value;
	sv->type = SCRIPT_DOUBLE;
}

void MakeDictValue(ScriptVal *sv) {
	memset(sv, 0, sizeof(ScriptVal));
	sv->flags = FLAG_NEED_FREE_DLL;
	sv->dictValue.numEntries = 0;
	sv->dictValue.entries = 0;
	sv->type = SCRIPT_DICT;
}

int ScriptDict::Add(ScriptVal *key, ScriptVal *val) {
	ScriptDictEntry *temp = (ScriptDictEntry*) realloc(entries, sizeof(ScriptDictEntry)*(numEntries+1));
	if (!temp) return 0;
	entries = temp;
	entries[numEntries].key = *key;
	entries[numEntries].value = *val;
	numEntries++;
	return 1;
}


void MakeListValue(ScriptVal *sv) {
	memset(sv, 0, sizeof(ScriptVal));
	sv->flags = FLAG_NEED_FREE_DLL;
	sv->listValue.numVals = 0;
	sv->listValue.vals = 0;
	sv->type = SCRIPT_LIST;
}

int ScriptList::PushBack(ScriptVal *value) {
	ScriptVal *temp = (ScriptVal*) realloc(vals, sizeof(ScriptVal)*(numVals+1));
	if (!temp) return 0;
	vals = temp;
	vals[numVals] = *value;
	numVals++;
	return 1;
}

int ScriptList::PushBack(__int64 i) {
	ScriptVal val;
	MakeIntValue(&val, i);
	return PushBack(&val);
}

void MakeNullValue(ScriptVal *sv) {
	sv->type = SCRIPT_NULL;
	sv->intValue = 0;
}
