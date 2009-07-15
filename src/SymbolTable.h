#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include "ScriptValue.h"
#include "ScriptEnums.h"
#include "HashTable.h"
#include "malloc.h"
#include "stringUtil.h"

template <class Type>
struct TableEntry {
	StringValue *sv;
	//unsigned char *val;
	Type data;
	int index;
};

template <class Type>
struct SymbolTable {
	HashTable hashTable;
	int numValues;
	int maxValues;
	TableEntry<Type> *table;
	TableEntry<Type> *Find(StringValue *v);
	TableEntry<Type> *Find(unsigned char *v);
	TableEntry<Type> *Add(StringValue *v);
	inline int Preallocate(int num) {
		if (num+numValues > maxValues) {
			if (!srealloc(table, (numValues+num)*sizeof(TableEntry<Type>))) return 0;
			memset(table+numValues, 0, sizeof(TableEntry<Type>)*num);
			maxValues = numValues+num;
		}
		return 1;
	}
	void Cleanup();
};

template <class Type>
inline void SymbolTable<Type>::Cleanup() {
	for (int i=0; i<numValues; i++) {
		table[i].sv->Release();
	}
	hashTable.Cleanup();
	free(table);
	memset(this,0, sizeof(*this));
}

template <class Type>
inline TableEntry<Type> *SymbolTable<Type>::Find(unsigned char *v) {
	void *val;
	if (!hashTable.Find(v, val)) return 0;
	return table + (size_t)val;
	/*
	for (int i=0; i<numValues; i++) {
		if (strcmp((char*)v, (char*)table[i].val) == 0) return  table+i;
	}
	return 0;
	//*/
}


template <class Type>
inline TableEntry<Type> *SymbolTable<Type>::Find(StringValue *v) {
	void *val;
	if (!hashTable.Find(v, val)) return 0;
	return table + (size_t)val;
	/*
	for (int i=0; i<numValues; i++) {
		if (strcmp((char*)v, (char*)table[i].val) == 0) return  table+i;
	}
	return 0;
	//*/
}

template <class Type>
inline TableEntry<Type> *SymbolTable<Type>::Add(StringValue *sv) {
	TableEntry<Type> *e = Find(sv);
	if (e) {
		sv->Release();
		return e;
	}
	if (numValues == maxValues) {
		if (!srealloc(table, (numValues+10)*sizeof(TableEntry<Type>))) return 0;
		memset(table+numValues, 0, sizeof(TableEntry<Type>)*10);
		maxValues += 10;
	}
	table[numValues].index = numValues;
	table[numValues].sv = sv;
	if (!hashTable.Add(table[numValues].sv, (void*) (size_t) numValues)) {
		sv->Release();
		return 0;
	}
	return &table[numValues++];
}

/*
template <class Type>
inline TableEntry<Type> *SymbolTable<Type>::Add(unsigned char *v) {
	TableEntry<Type> *e = Find(v);
	if (e) {
		return e;
	}
	ScriptValue sv;
	if (!CreateStringValue(sv, v)) return 0;
	return Add(sv.stringVal);
}

/*
template <class Type>
inline TableEntry<Type> *SymbolTable<Type>::Add(unsigned char *v) {
	TableEntry<Type> *e = Find(v);
	if (e) return e;
	if (numValues % 10 == 0) {
		if (!srealloc(table, (numValues+10)*sizeof(TableEntry<Type>))) return 0;
	}
	table[numValues].index = numValues;
	if (!(table[numValues].val = (unsigned char*)strdup((char*)v))) return 0;
	if (!hashTable.Add(table[numValues].val, (void*) (size_t) numValues)) {
		free(table[numValues].val);
		return 0;
	}
	return &table[numValues++];
}
//*/
struct StringTableStruct {
	HashTable hashTable;
	int numValues;
	ScriptValue *table;
	int Add(unsigned char *v, int len);
	int Add(StringValue *v);
	int Add(unsigned char *v);
	int Find(StringValue *v);
	void Cleanup();
};


/*
#define CREATE_STATIC_STRING(OUT, VAL) {	\
	static int val = -1;					\
	if (val >= 0) {							\
		OUT = StringTable.table[val];		\
	}										\
	else {									\
		val = StringTable.Add((unsigned char*)VAL);	\
		if (val == -1) {					\
			OUT = StringTable.table[0];		\
		}									\
		else {								\
			OUT = StringTable.table[val];	\
		}									\
	}										\
	OUT.stringVal->refCount++;				\
}
//*/


// note:  Length must be set.  For single use when used alone.
#define STATIC_SINGLE_USE_STRING_VALUE(string) ("\2\0\0\0\0\0\0\0" string)

#define CREATE_STATIC_STRING(out, val) {						\
	static char junk[] =  STATIC_SINGLE_USE_STRING_VALUE(val);	\
	out.type = SCRIPT_STRING;									\
	out.stringVal = (StringValue*) junk;						\
	out.stringVal->refCount++;									\
	out.stringVal->len = sizeof(junk)-9;						\
	/*if (!out.stringVal->len) out.stringVal->len = (int)strlen(out.stringVal->value);*/	\
}

#endif

