#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif

#ifndef SCRIPT_TYPES_H
#define SCRIPT_TYPES_H

struct ScriptValue;
struct DictValue;

#define SCRIPT_NULL -1
#define SCRIPT_INT 1
#define SCRIPT_DOUBLE 2
#define SCRIPT_STRING 4
#define SCRIPT_LIST 8
#define SCRIPT_DICT 16
#define SCRIPT_WIDE_STRING 32
#define SCRIPT_ASCII_STRING 64
#define SCRIPT_CUSTOM_TYPE 64
#define SCRIPT_OBJECT 128
//#define SCRIPT_NAMESPACE 16
//#define SCRIPT_REFERENCE 64

#define SCRIPT_POINTER (SCRIPT_CUSTOM_TYPE | SCRIPT_STRING | SCRIPT_LIST | SCRIPT_DICT | SCRIPT_OBJECT)

struct PointedToValue {
	unsigned int refCount;
	inline void AddRef() {
		refCount++;
	}
};

struct ObjectValue;

struct ListValue : public PointedToValue {
	int size;
	ScriptValue *vals;
	int numVals;
	void Release();
	int Set(ScriptValue &s, int index);
	int Insert(ScriptValue &s, int index);
	int Resize(int size, int init);
	int Dupe(ScriptValue &s);
	inline int PushBack(ScriptValue &s) {
		return Set(s, numVals);
	}
	int PushBack(const __int64 &i);

	// Pushes a null.
	int PushBack();
};
int CreateListValue(ScriptValue &s, int size);

struct StringValue : public PointedToValue {
	int len;
	unsigned char value[4];
	void Release();
};

struct ScriptValue {
	int type;
	//int refs;
	union {
		__int64 intVal;
		double doubleVal;
		// For simple access.
		StringValue *stringVal;
		ListValue *listVal;
		ScriptValue *refVal;
		DictValue *dictVal;
		ObjectValue *objectVal;
		int i32;
	};
	void Release();
	inline void AddRef() {
		if (!(type & ~SCRIPT_POINTER)) {
			// Note:  All inherit from the same single class, so works for all types.
			listVal->refCount++;
		}
	}
};

// Doesn't add a reference, duplicate string, or do anything other than init s.
inline void CreateStringValue(ScriptValue &s, StringValue *sv) {
	s.type = SCRIPT_STRING;
	s.stringVal = sv;
}


struct DictEntry {
	ScriptValue val;
	ScriptValue key;
	//DictionaryEntry *next;
};

inline void CreateTypeValue(ScriptValue &s, int type, const __int64 &i) {
	s.type = type;
	s.intVal = i;
}

inline void CreateTypeValue(ScriptValue &s, int type, void *v) {
	s.type = type;
	s.stringVal = (StringValue*)v;
}

inline void CreateIntValue(ScriptValue &s, const __int64 &i) {
	s.type = SCRIPT_INT;
	s.intVal = i;
}

/*inline void CreateIntValue(ScriptValue &s, int i) {
	s.type = SCRIPT_INT;
	s.intVal = i;
}//*/

inline int ListValue::PushBack(const __int64 &i) {
	ScriptValue sv;
	CreateIntValue(sv, i);
	return PushBack(sv);
}

inline void CreateNullValue(ScriptValue &s) {
	s.type = -1;
	s.intVal = 0;
}

inline int ListValue::PushBack() {
	ScriptValue sv;
	CreateNullValue(sv);
	return PushBack(sv);
}

inline void CreateDoubleValue(ScriptValue &s, double d) {
	s.type = SCRIPT_DOUBLE;
	s.doubleVal = d;
}

struct DictValue  : public PointedToValue {
	int size;
	DictEntry *entries;
	int numEntries;
	void Cleanup();
	void Release();
	int Add(ScriptValue &key, ScriptValue &value);
	int Find(const ScriptValue &key, ScriptValue &val);
	int Dupe(ScriptValue &s);
};
int CreateDictValue(ScriptValue &s, int size);
void CreateStringValue(ScriptValue &s);

int CreateStringValue(ScriptValue &s, const unsigned char *st);
int CreateStringValue(ScriptValue &s, const unsigned char *st, int len);
int CreateStringValue(ScriptValue &s, const wchar_t *st);
int CreateStringValue(ScriptValue &s, const wchar_t *st, int len);

inline int CreateStringValue(ScriptValue &s, const char *st) {
	return CreateStringValue(s, (const unsigned char *)st);
}


inline int AllocateStringValue(ScriptValue &s, int len) {
	return CreateStringValue(s, (unsigned char*)0, len);
}

inline int AllocateCustomValue(ScriptValue &s, int len) {
	if (!CreateStringValue(s, (unsigned char*)0, len-1)) return 0;
	s.type = SCRIPT_CUSTOM_TYPE;
	return 1;
}

void CreateObjectValue(ScriptValue &s, ObjectValue *st);

// Should only be called on string creation.
int ResizeStringValue(ScriptValue &s, int len);

inline int ResizeCustomValue(ScriptValue &s, int len) {
	return ResizeStringValue(s, len);
}

//void CreateObjectValue(ScriptValue &s, ObjectValue *st);

struct ObjectValue : public PointedToValue {
	unsigned int type;
	ScriptValue values[1];
	void Release();
};

inline void ScriptValue ::Release() {
	if (type & (SCRIPT_INT | SCRIPT_DOUBLE)) return;
	if (stringVal->refCount > 1) {
		stringVal->refCount--;
	}
	else {
		if (type & (SCRIPT_STRING | SCRIPT_CUSTOM_TYPE)) {
			stringVal->Release();
		}
		else if (type & SCRIPT_LIST) {
			listVal->Release();
		}
		else if (type & SCRIPT_OBJECT) {
			objectVal->Release();
		}
		else if (type & SCRIPT_DICT) {
			dictVal->Release();
		}
	}
}




/*
inline void Dictionary::Cleanup() {
	for (int i=0; i<size; i++) {
		while (entries[i]) {
			DictionaryEntry *temp = entries[i];
			entries[i] = entries[i]->next;
			temp->val->Release();
			temp->key->Release();
			free(temp);
		}
	}
	free(this);
}
extern ScriptValue EmptyString;
//*/


int scriptstrcmp(const unsigned char *s1, int len1,  const unsigned char *s2, int len2);
inline int scriptstrcmp(const StringValue *s1, const StringValue *s2) {
	return scriptstrcmp(s1->value, s1->len,  s2->value, s2->len);
}
inline int scriptstrcmp(const ScriptValue &s1, const ScriptValue &s2) {
	return scriptstrcmp(s1.stringVal, s2.stringVal);
}

int scriptstricmp(const unsigned char *s1, int len1, const unsigned char *s2, int len2);

inline int scriptstricmp(const StringValue *s1, const StringValue *s2) {
	return scriptstricmp(s1->value, s1->len,  s2->value, s2->len);
}

inline int scriptstricmp(const ScriptValue &s1, const ScriptValue &s2) {
	return scriptstricmp(s1.stringVal, s2.stringVal);
}
#endif
