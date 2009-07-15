#ifndef SCRIPT_OBJECTS_H
#define SCRIPT_OBJECTS_H

#include "../ScriptValue.h"

struct ObjectTypeFunction {
	int stringID;
	int functionID;
};

struct ObjectTypeValue {
	int stringID;
};

struct ObjectType {
	ObjectTypeFunction *functs;
	ObjectTypeValue *values;

	int CleanupFunction;

	int numFuncts;
	int numValues;

	int type;
	int nameID;

	int canSubclass;

	void Cleanup();
	int AddValue(unsigned char *name, int len);
	int AddFunction(unsigned char *name, int len, int id);
	int AddFunction(int stringID, int functionID);

	inline int FindFunction(int id) {
		int min = 0; int max = numFuncts;
		while (min < max - 4) {
			int mid = (1+min+max)/2;
			if (functs[mid].stringID <= id) min = mid;
			else max = mid;
		}
		for (int i=min; i<max; i++) {
			if (id == functs[i].stringID) return i;
		}
		return -1;
	}

	int FindFunction(StringValue *s);

	inline int FindValue(int id) {
		int min = 0; int max = numValues;
		while (min < max - 4) {
			int mid = (1+min+max)/2;
			if (values[mid].stringID <= id) min = mid;
			else max = mid;
		}
		for (int i=0; i<max; i++) {
			if (id == values[i].stringID) return i;
		}
		return -1;
	}
	int FindValue(StringValue *s);
};

int RegisterObjectTypes();
void CleanupObjectTypes();

ObjectType *CreateObjectType(unsigned char *type, int canSubclass);
ObjectType *FindObjectType(unsigned char *type, int canSubclass);

int CreateObjectValue(ScriptValue &s, int type);

/*
void CallObjectFunction(ScriptValue &obj, int funct, ScriptValue &params);
inline void ReleaseObject (ObjectValue *obj) {
	ScriptValue sv1, sv2;
	CreateNullValue(sv2);
	CreateObjectValue(sv1, obj);
	CallObjectFunction(sv1, 1, sv2);
}
//*/

extern ObjectType *types;
extern int numTypes;
/*
__forceinline int FindObjectFunction(int t, int stringID) {
	if (t == SCRIPT_OBJECT) {
		ObjectType *type = &types[t];
		int i = type->FindFunction(stringID);
		if (i >= 0) {
			return functs[i].functionID;
		}
	}
	// null();
	return 0;
}
//*/
__forceinline void FindObjectValue(ScriptValue &out, ScriptValue &in, int stringID) {
	if (in.type == SCRIPT_OBJECT) {
		ObjectType *type = &types[in.objectVal->type];
		int i = type->FindValue(stringID);
		if (i >= 0) {
			out = in.objectVal->values[i];
			out.AddRef();
			return;
		}
	}
	CreateNullValue(out);
}

#endif
