#ifndef HASHTABLE_H
#define HASHTABLE_H
#include "ScriptValue.h"
struct HashTableEntry {
	HashTableEntry *next;
	void *value;
	StringValue *key;
	unsigned int hash;
};

// Simply hash table.  Meant merely to index data, rather than take care of memory allocation.
// Cannot replace stored data.

struct HashTable {
	HashTableEntry *entries;
	int size;
	int numEntries;
	int insertPos;
	inline void Init() {
		entries = 0;
		size = 0;
		numEntries = 0;
	}
	int FindIndex(StringValue *key);
	int FindIndex(unsigned char *key);
	int Find(StringValue *key, void * &v);
	int Find(unsigned char *key, void * &v);
	int Add(StringValue *key, void *value);
	void QuickAdd(StringValue *key, void *value, int hash);
	void Cleanup();
};
#endif
