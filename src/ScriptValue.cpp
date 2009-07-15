#include "ScriptValue.h"
#include "malloc.h"
#include "stringUtil.h"
#include "script.h"
#include "unicode.h"
#include "ScriptObjects/ScriptObjects.h"
#include "vm.h"

/*int scriptstrcmp(const StringValue *s1, const StringValue *s2) {
	int minLen = s1->len;
	if (s2->len < minLen)
		minLen = s2->len;
	for (int i = 0; i<minLen; i++) {
		if (s1->value[i] != s2->value[i]) {
			return (s1->value[i] > s2->value[i])*2 - 1;
		}
	}
	return (minLen==s2->len) - (minLen==s1->len);
}

/*/

int scriptstrcmp(const unsigned char *s1, int len1,  const unsigned char *s2, int len2) {
	int minLen = len1;
	if (len2 < minLen)
		minLen = len2;
	for (int i = 0; i<minLen; i++) {
		if (s1[i] != s2[i]) {
			return (s1[i] > s2[i])*2 - 1;
		}
	}
	return (minLen==len2) - (minLen==len1);
}
//*/

int scriptstricmp(const unsigned char *s1, int len1,  const unsigned char *s2, int len2) {
	int minLen = len1;
	if (len2 < minLen)
		minLen = len2;
	for (int i = 0; i<minLen; i++) {
		if (LCASE(s1[i]) != LCASE(s2[i])) {
			return (LCASE(s1[i]) > LCASE(s2[i]))*2 - 1;
		}
	}
	return (minLen==len2) - (minLen==len1);
}

int ResizeStringValue(ScriptValue &s, int len) {
	if (!srealloc(s.stringVal, sizeof(StringValue) + sizeof(unsigned char) * (len-3))) return 0;
	s.stringVal->len = len;
	s.stringVal->value[len] = 0;
	return 1;
}


int CreateStringValue(ScriptValue &s, const unsigned char *st, int len) {
/*	if (st)
	if (!strcmp(st, (unsigned char*)"i") || !strcmp(st, (unsigned char*) "cpuData") || !strcmp(st, (unsigned char*)"sf") || !strcmp(st, (unsigned char*)"data")
		|| !stricmp(st, (unsigned char*)"rawdata") || !stricmp(st, (unsigned char*)"enum")) {
		st = st;
	}
	//*/
	s.stringVal = (StringValue*) malloc(sizeof(StringValue) + sizeof(unsigned char) * (len-3));
	if (!s.stringVal) {
		CreateNullValue(s);
		return 0;
	}
	s.type = SCRIPT_STRING;
	s.stringVal->len = len;
	s.stringVal->value[len] = 0;
	s.stringVal->refCount = 1;
	if (st)
		memcpy(s.stringVal->value, st, len);
	return 1;
}

int CreateStringValue(ScriptValue &s, const wchar_t *st, int len) {
	s.stringVal = (StringValue*) malloc(sizeof(StringValue) + sizeof(unsigned char) * (3*len-3));
	if (!s.stringVal) {
		CreateNullValue(s);
		return 0;
	}
	len = UTF16toUTF8(s.stringVal->value, st, len);
	ResizeStringValue(s, len);
	s.type = SCRIPT_STRING;
	s.stringVal->refCount = 1;
	return 1;
}

int CreateStringValue(ScriptValue &s, const unsigned char *st) {
	return CreateStringValue(s, st, (int)strlen(st));
}

int CreateStringValue(ScriptValue &s, const wchar_t *st) {
	return CreateStringValue(s, st, (int)wcslen(st));
}

void CreateObjectValue(ScriptValue &s, ObjectValue *st) {
	s.type = SCRIPT_OBJECT;
	s.objectVal = st;
}


void CreateStringValue(ScriptValue &s) {
	s = VM.StringTable.table[0];
	s.stringVal->AddRef();
}

void StringValue::Release() {
	if (--refCount == 0) {
		free(this);
	}
}

void ListValue::Release() {
	if (--refCount == 0) {
		for (int i=0; i<numVals; i++) {
			vals[i].Release();
		}
		free(vals);
		free(this);
	}
}

int ListValue::Resize(int size, int init) {
	if (init < numVals) {
		for (int j=init; j<numVals; j++) {
			vals[j].Release();
		}
		numVals = init;
	}
	else {
		if (this->size < size && !srealloc(vals, sizeof(ScriptValue) * size)) {
			return 0;
		}
		this->size = size;
		for (int j=numVals; j<init; j++) {
			CreateNullValue(vals[j]);
		}
		numVals = init;
	}
	return 1;
}

int ListValue::Set(ScriptValue &s, int index) {
	if (index < 0) return 0;
	if (numVals <= index) {
		if (index >= size) {
			if (!srealloc(vals, sizeof(ScriptValue) * (index+(size>>2)+1))) {
				s.Release();
				return 0;
			}
			size = index+(size>>2)+1;
		}
		while (numVals < index) {
			CreateNullValue(vals[numVals++]);
		}
		vals[numVals++] = s;
	}
	else {
		vals[index].Release();
		vals[index] = s;
	}
	return 1;
}

int ListValue::Insert(ScriptValue &s, int index) {
	if (index < 0) return 0;
	if (numVals <= index) {
		return Set(s, index);
	}
	else {
		if (numVals == size) {
			if (!srealloc(vals, sizeof(ScriptValue) * (size+(size>>2)+1))) {
				s.Release();
				return 0;
			}
			size = size+(size>>2)+1;
		}
		for (int i=numVals; i>index; i--) {
			vals[i] = vals[i-1];
		}
		numVals ++;
		vals[index] = s;
	}
	return 1;
}

int ListValue::Dupe(ScriptValue &s) {
	if (!this->numVals) return CreateListValue(s, 2);
	if (!CreateListValue(s, size)) return 0;
	for (int i=numVals-1; i>=0; i--) {
		s.listVal->vals[i] = vals[i];
		vals[i].AddRef();
	}
	s.listVal->numVals = numVals;
	return 1;
}

int CreateListValue(ScriptValue &s, int size) {
	s.listVal = (ListValue*) malloc(sizeof(ListValue));
	if (!s.listVal) return 0;
	s.listVal->vals = (ScriptValue *)malloc(sizeof(ScriptValue) * size);
	if (!s.listVal->vals) {
		free(s.listVal);
		return 0;
	}
	s.type = SCRIPT_LIST;
	s.listVal->refCount = 1;
	s.listVal->numVals = 0;
	s.listVal->size = size;
	return 1;
}

int CreateDictValue(ScriptValue &s, int size) {
	s.dictVal = (DictValue*) malloc(sizeof(DictValue));
	if (!s.dictVal) return 0;
	s.dictVal->entries = (DictEntry *)malloc(sizeof(DictEntry) * size);
	if (!s.dictVal->entries) {
		free(s.dictVal);
		return 0;
	}
	s.type = SCRIPT_DICT;
	s.dictVal->refCount = 1;
	s.dictVal->numEntries = 0;
	s.dictVal->size = size;
	return 1;
}

int DictValue::Dupe(ScriptValue &s) {
	if (!this) return CreateDictValue(s, 2);
	if (!CreateDictValue(s, size)) return 0;
	for (int i=numEntries-1; i>=0; i--) {
		s.dictVal->entries[i] = entries[i];
		entries[i].val.AddRef();
		entries[i].key.AddRef();
	}
	s.dictVal->numEntries = numEntries;
	return 1;
}

int DictValue::Add(ScriptValue &key, ScriptValue &val) {
	for (int i=0; i<numEntries; i++) {
		if (scriptstrcmp(key, entries[i].key) == 0) {
			entries[i].key.Release();
			entries[i].val.Release();
			entries[i].key = key;
			entries[i].val = val;
			return 1;
		}
	}
	if (size == numEntries) {
		if (!srealloc(entries, sizeof(DictEntry) * (numEntries *2+1))) return 0;
		size += numEntries+1;
	}
	entries[numEntries].key = key;
	entries[numEntries++].val = val;
	return 1;
}

void DictValue::Release() {
	/*
#ifdef _DEBUG
	for (i=0; i<Functions.numValues; i++) {
		Functions.table[i].sv->refCount++;
	}
	for (i=0; i<Functions.numValues; i++) {
		Functions.table[i].sv->refCount--;
	}
#endif
	//*/
	if (0 == --refCount) {
		for (int i=0; i<numEntries; i++) {
			entries[i].key.Release();
			entries[i].val.Release();
		}
		free(entries);
		free(this);
	}
}

int DictValue::Find(const ScriptValue &key, ScriptValue &val) {
	for (int i=0; i<numEntries; i++) {
		if (scriptstrcmp(key, entries[i].key) == 0) {
			val = entries[i].val;
			return 1;
		}
	}
	return 0;
}

