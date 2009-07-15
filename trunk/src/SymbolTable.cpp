#include "SymbolTable.h"
#include "stringUtil.h"
#include "malloc.h"

struct ScriptValue;
struct DictValue;


int scriptstrcmp(const ScriptValue &s1, const ScriptValue &s2);
int scriptstricmp(const ScriptValue &s1, const ScriptValue &s2);

int CreateListValue(ScriptValue &s, int size);

SymbolTable<bool> Globals = {0,0};

void StringTableStruct::Cleanup() {
	for (int i=numValues-1; i>=0; i--) {
		table[i].stringVal->Release();
	}
	free(table);
	hashTable.Cleanup();
}

int StringTableStruct::Add(unsigned char *v) {
	return Add(v, (int)strlen(v));
}

int StringTableStruct::Add(unsigned char *v, int len) {
	for (int i=0; i<numValues; i++) {
		if (len != table[i].stringVal->len) continue;
		if (memcmp((char*)v, (char*)table[i].stringVal->value, len) == 0) {
			//table[i].stringVal->refCount++;
			return  i;
		}
	}
	if (numValues % 10 == 0) {
		if (!srealloc(table, (numValues+10)*sizeof(ScriptValue))) return -1;
	}
	if (!CreateStringValue(table[numValues], v, len)) return -1;
	if (!hashTable.Add(table[numValues].stringVal, (void*)(size_t)numValues)) {
		table[numValues].stringVal->Release();
		return -1;
	}
	//sv.AddRef();
	return numValues++;
}

int StringTableStruct::Find(StringValue *key) {
	void *v;
	if (!hashTable.Find(key, v)) return -1;
	return (int)(size_t) v;
}

int StringTableStruct::Add(StringValue *v) {
	for (int i=0; i<numValues; i++) {
		if (v->len != table[i].stringVal->len) continue;
		if (memcmp(v->value, (char*)table[i].stringVal->value, v->len) == 0) {
			//table[i].stringVal->refCount++;
			v->Release();
			return  i;
		}
	}
	if (numValues % 10 == 0) {
		if (!srealloc(table, (numValues+10)*sizeof(ScriptValue))) return -1;
	}
	if (!hashTable.Add(v, (void*)(size_t)numValues)) return -1;
	CreateStringValue(table[numValues], v);
	//v->AddRef();
	return numValues++;
}

