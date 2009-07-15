#include <stdlib.h>
#include "ScriptTypes.h"
#include "../../vm.h"
#include "../../ScriptObjects/ScriptObjects.h"
void GetType(ScriptValue &s, ScriptValue *args) {
	if (args->type == SCRIPT_LIST && args->listVal->numVals) {
		switch (args->listVal->vals[0].type) {
			case -1:
				s = VM.StringTable.table[1];
				break;
			case SCRIPT_INT:
				s = VM.StringTable.table[2];
				break;
			case SCRIPT_DOUBLE:
				s = VM.StringTable.table[3];
				break;
			case SCRIPT_STRING:
				s = VM.StringTable.table[4];
				break;
			case SCRIPT_LIST:
				s = VM.StringTable.table[5];
				break;
			case SCRIPT_DICT:
				s = VM.StringTable.table[6];
				break;
			case SCRIPT_OBJECT:
				s = VM.StringTable.table[types[args->listVal->vals[0].objectVal->type].nameID];
				break;
			default:
				return;
		}
		s.stringVal->AddRef();
	}
}

void IsNull(ScriptValue &s, ScriptValue *args) {
	CreateIntValue(s, 0);
	if (args->type == SCRIPT_LIST && args->listVal->numVals > 0) {
		for (int i=0; i<args->listVal->numVals; i++) {
			if (args->listVal->vals[i].type != -1) return;
		}
	}
	s.i32 = 1;
}

void IsString(ScriptValue &s, ScriptValue *args) {
	CreateIntValue(s, 0);
	if (args->type == SCRIPT_LIST && args->listVal->numVals > 0) {
		for (int i=0; i<args->listVal->numVals; i++) {
			if (!(args->listVal->vals[i].type == SCRIPT_STRING)) return;
		}
	}
	s.i32 = 1;
}

void IsInt(ScriptValue &s, ScriptValue *args) {
	CreateIntValue(s, 0);
	if (args->type == SCRIPT_LIST && args->listVal->numVals > 0) {
		for (int i=0; i<args->listVal->numVals; i++) {
			if (!(args->listVal->vals[i].type == SCRIPT_INT)) return;
		}
	}
	s.i32 = 1;
}

void IsFloat(ScriptValue &s, ScriptValue *args) {
	CreateIntValue(s, 0);
	if (args->type == SCRIPT_LIST && args->listVal->numVals > 0) {
		for (int i=0; i<args->listVal->numVals; i++) {
			if (!(args->listVal->vals[i].type == SCRIPT_DOUBLE)) return;
		}
	}
	s.i32 = 1;
}

void IsList(ScriptValue &s, ScriptValue *args) {
	CreateIntValue(s, 0);
	if (args->type == SCRIPT_LIST && args->listVal->numVals > 0) {
		for (int i=0; i<args->listVal->numVals; i++) {
			if (!(args->listVal->vals[i].type == SCRIPT_LIST)) return;
		}
	}
	s.i32 = 1;
}

void IsDict(ScriptValue &s, ScriptValue *args) {
	CreateIntValue(s, 0);
	if (args->type == SCRIPT_LIST && args->listVal->numVals > 0) {
		for (int i=0; i<args->listVal->numVals; i++) {
			if (!(args->listVal->vals[i].type == SCRIPT_DICT)) return;
		}
	}
	s.i32 = 1;
}

void IsObject(ScriptValue &s, ScriptValue *args) {
	CreateIntValue(s, 0);
	if (args->type == SCRIPT_LIST && args->listVal->numVals > 0) {
		for (int i=0; i<args->listVal->numVals; i++) {
			if (!(args->listVal->vals[i].type == SCRIPT_OBJECT)) return;
		}
	}
	s.i32 = 1;
}

void IsNum(ScriptValue &s, ScriptValue *args) {
	CreateIntValue(s, 0);
	if (args->type == SCRIPT_LIST && args->listVal->numVals > 0) {
		for (int i=0; i<args->listVal->numVals; i++) {
			if (!(args->listVal->vals[i].type & (SCRIPT_INT | SCRIPT_DOUBLE)) || args->listVal->vals[i].type == SCRIPT_NULL) return;
		}
	}
	s.i32 = 1;
}

void IsSimple(ScriptValue &s, ScriptValue *args) {
	CreateIntValue(s, 0);
	if (args->type == SCRIPT_LIST && args->listVal->numVals > 0) {
		for (int i=0; i<args->listVal->numVals; i++) {
			if (!(args->listVal->vals[i].type & (SCRIPT_INT | SCRIPT_DOUBLE | SCRIPT_STRING))) return;
		}
	}
	s.i32 = 1;
}

void IsCompound(ScriptValue &s, ScriptValue *args) {
	CreateIntValue(s, 0);
	if (args->type == SCRIPT_LIST && args->listVal->numVals > 0) {
		for (int i=0; i<args->listVal->numVals; i++) {
			if (!((args->listVal->vals[i].type & (SCRIPT_DICT | SCRIPT_LIST | SCRIPT_OBJECT))==(SCRIPT_DICT | SCRIPT_LIST | SCRIPT_OBJECT))) return;
		}
	}
	s.i32 = 1;
}

void StackCount(ScriptValue &s, ScriptValue *args) {
	CreateIntValue(s, VM.numStacks);
}

void Equals(ScriptValue &s, ScriptValue *args) {
	CreateIntValue(s, 0);
	if (args[0].type == args[1].type) {
		if (args[0].type & (SCRIPT_DOUBLE | SCRIPT_INT)) {
			s.i32 = args[0].intVal == args[1].intVal;
		}
		else {
			if (args[0].stringVal == args[1].stringVal) {
				s.i32 = 1;
			}
			else if (args[0].type == SCRIPT_STRING) {
				s.i32 = !scriptstrcmp(args[0], args[1]);
			}
		}
	}
	else if (args[0].type == SCRIPT_NULL) {
		if (args[1].type & SCRIPT_INT) {
			s.i32 = (args[1].intVal == 0);
		}
		else if (args[1].type & SCRIPT_DOUBLE) {
			s.i32 = (args[1].doubleVal == 0);
		}
		else if (args[1].type & SCRIPT_STRING) {
			s.i32 = args[1].stringVal->len == 0;
		}
	}
	else if (args[1].type == SCRIPT_NULL) {
		if (args[0].type & SCRIPT_INT) {
			s.i32 = (args[0].intVal == 0);
		}
		else if (args[0].type & SCRIPT_DOUBLE) {
			s.i32 = (args[0].doubleVal == 0);
		}
		else if (args[0].type & SCRIPT_STRING) {
			s.i32 = args[0].stringVal->len == 0;
		}
	}
	else if ((args[0].type | args[1].type) == (SCRIPT_DOUBLE | SCRIPT_INT)) {
		ScriptValue d1, d2;
		CoerceDoubleNoRelease(args[0], d1);
		CoerceDoubleNoRelease(args[1], d2);
		s.i32 = d1.doubleVal == d2.doubleVal;
	}
}
