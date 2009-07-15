#include <assert.h>
#include <stdlib.h>
#include "vm.h"
#include "stringUtil.h"
#include "ScriptObjects/ScriptObjects.h"
#include "SymbolTable.h"
#include "ScriptProcs/InterProcess/ScriptDll.h"
#include "config.h"
//#include "script.h"
//#include <stdio.h>

Code code = {0,0};
VirtualMachine VM;

void CoerceString(ScriptValue &sv, ScriptValue& sv2) {
	if (sv.type == SCRIPT_STRING) {
		sv2 = sv;
	}
	else if (sv.type == -1) {
		// Switch from null to empty string.
		CreateStringValue(sv2);
	}
	else if (sv.type & (SCRIPT_INT | SCRIPT_DOUBLE)) {
		if (!AllocateStringValue(sv2, 20)) {
			CreateStringValue(sv2);
			return;
		}
		sv2.stringVal->refCount = 1;
		if (sv.type & SCRIPT_INT) {
			_i64toa(sv.intVal, (char*)sv2.stringVal->value, 10);
		}
		else {
			_gcvt(sv.doubleVal, 14, (char*)sv2.stringVal->value);
			int len = strlen(sv2.stringVal->value);
			if (sv2.stringVal->value[len-1] == '.') {
				sv2.stringVal->value[len-1] = 0;
			}
			//if (sv.doubleVal < 1000 && sv.doubleVal > -1000)
			//	sprintf((char*)sv2.stringVal->value, "%3lf", sv.doubleVal);
			//else
			//	sprintf((char*)sv2.stringVal->value, "%3e", sv.doubleVal);
		}
		ResizeStringValue(sv2, strlen(sv2.stringVal->value));
	}
	else {
		CreateStringValue(sv2);
		sv.Release();
	}
	//return 1;
}

void CoerceStrings(ScriptValue &sv, ScriptValue& sv2,ScriptValue &sv3, ScriptValue& sv4) {
	CoerceString(sv, sv3);
	CoerceString(sv2, sv4);
}

void CoerceDoubleNoRelease(ScriptValue &sv, ScriptValue& sv2) {
	if (sv.type & SCRIPT_DOUBLE) {
		sv2.intVal = sv.intVal;
		sv2.type = SCRIPT_DOUBLE;
		return;
	}
	double d = 0;
	if (sv.type == SCRIPT_INT) {
		d = (double) sv.intVal;
	}
	else if (sv.type == SCRIPT_STRING) {
		d = strtod((char*)sv.stringVal->value, 0);
	}
	CreateDoubleValue(sv2, d);
}

void CoerceDouble(ScriptValue &sv, ScriptValue& sv2) {
	if (sv.type & SCRIPT_DOUBLE) {
		sv2.doubleVal = sv.doubleVal;
		sv2.type = SCRIPT_DOUBLE;
		return;
	}
	double d = 0;
	if (sv.type == SCRIPT_INT) {
		d = (double) sv.intVal;
	}
	else {
		if (sv.type == SCRIPT_STRING) {
			d = strtod((char*)sv.stringVal->value, 0);
		}
		sv.Release();
	}
	CreateDoubleValue(sv2, d);
}

__int64 CoerceIntNoRelease(ScriptValue &sv) {
	if (sv.type & SCRIPT_INT) {
		return sv.intVal;
	}
	if (sv.type == SCRIPT_DOUBLE) {
		return (__int64) sv.doubleVal;
	}
	else if (sv.type == SCRIPT_STRING) {
		char *t1, *t2;
		__int64 i = strtoi64((char*)sv.stringVal->value, &t1, 0);
		double d = strtod((char*)sv.stringVal->value, &t2);
		if (t1 < t2) {
			i = (__int64) d;
		}
		return i;
	}
	return 0;
}

void CoerceIntNoRelease(ScriptValue &sv, ScriptValue& sv2) {
	if (sv.type & SCRIPT_INT) {
		sv2 = sv;
		return;
	}
	__int64 i = 0;
	if (sv.type == SCRIPT_DOUBLE) {
		i = (__int64) sv.doubleVal;
	}
	else if (sv.type == SCRIPT_STRING) {
		char *t1, *t2;
		i = strtoi64((char*)sv.stringVal->value, &t1, 0);
		double d = strtod((char*)sv.stringVal->value, &t2);
		if (t1 < t2) {
			i = (__int64) d;
		}
	}
	CreateIntValue(sv2, i);
}

void PopCoerceInt(ScriptValue &sv3, ScriptValue& sv4, Stack *s) {
	ScriptValue sv2 = s->Pop();
	ScriptValue sv = s->Pop();
	CoerceIntNoRelease(sv, sv3);
	CoerceIntNoRelease(sv2, sv4);
	sv.Release();
	sv2.Release();
}

void CoerceMath(ScriptValue &sv, ScriptValue& sv2) {
	if (sv.type & (SCRIPT_DOUBLE | SCRIPT_INT)) {
		sv2.intVal = sv.intVal;
		// Overkill optimization.
		sv2.type = (6+sv.type)>>2;
	}
	else if (sv.type == SCRIPT_STRING) {
		char *t1, *t2;
		__int64 i = strtoi64((char*)sv.stringVal->value, &t1, 0);
		double d = strtod((char*)sv.stringVal->value, &t2);
		if (t1 >= t2) {
			CreateIntValue(sv2, i);
		}
		else {
			CreateDoubleValue(sv2, d);
		}
		sv.stringVal->Release();
	}
	else {
		CreateIntValue(sv2, 0);
		sv.Release();
	}
}
//*
bool CoerceBoolKeep(ScriptValue &sv) {
	if (sv.type & SCRIPT_INT) return (0!=sv.intVal);
	if (sv.type == SCRIPT_DOUBLE) return (0!=sv.doubleVal);
	bool out;
	if (sv.type == SCRIPT_STRING) {
		char *t1, *t2;
		__int64 i = strtoi64((char*)sv.stringVal->value, &t1, 0);
		double d = strtod((char*)sv.stringVal->value, &t2);
		if (t1 >= t2) {
			return (i!=0);
		}
		else {
			return (d != 0);
		}
	}
	else if (sv.type == SCRIPT_LIST) {
		out = (0!=sv.listVal->numVals);
	}
	else if (sv.type == SCRIPT_DICT) {
		out = (0!=sv.dictVal->numEntries);
	}
	return out;
}//*/

bool CoerceBool(ScriptValue &sv) {
	if (sv.type & SCRIPT_INT) return (0!=sv.intVal);
	if (sv.type == SCRIPT_DOUBLE) return (0!=sv.doubleVal);
	bool out;
	if (sv.type == SCRIPT_STRING) {
		ScriptValue sv2;
		CoerceMath(sv, sv2);
		if (sv2.type & SCRIPT_INT) return (0!=sv2.intVal);
		return (0!=sv2.doubleVal);
	}
	else if (sv.type == SCRIPT_LIST) {
		out = (0!=sv.listVal->numVals);
		sv.listVal->Release();
	}
	else if (sv.type == SCRIPT_DICT) {
		out = (0!=sv.dictVal->numEntries);
		sv.dictVal->Release();
	}
	return out;
}

void CoerceMath(ScriptValue &sv, ScriptValue& sv2, ScriptValue &sv3, ScriptValue &sv4) {
	CoerceMath(sv, sv3);
	if (sv3.type == SCRIPT_INT) {
		CoerceMath(sv2, sv4);
		if (sv4.type == SCRIPT_DOUBLE) {
			CreateDoubleValue(sv3, (double)sv3.intVal);
		}
	}
	else {
		CoerceDouble(sv2, sv4);
	}
}

void CoerceMathPairPop(Stack *s, ScriptValue &sv3, ScriptValue &sv4) {
	//ScriptValue sv2 = s->Pop();
	//ScriptValue sv = s->Pop();
	s->top-=2;
	CoerceMath(s->vals[s->top], s->vals[s->top+1], sv3, sv4);
}

int PopCoerceNumCanCompare(ScriptValue &sv3, ScriptValue &sv4, Stack *s) {
	ScriptValue sv2 = s->Pop();
	ScriptValue sv = s->Pop();
	/*if (!(sv.type & (SCRIPT_INT|SCRIPT_DOUBLE|SCRIPT_STRING)) ||
		!(sv2.type & (SCRIPT_INT|SCRIPT_DOUBLE|SCRIPT_STRING))) {
			sv.Release();
			sv2.Release();
			return 0;
	}
	//*/
	CoerceMath(sv, sv2, sv3, sv4);
	//sv.Release();
	//sv2.Release();
	return 1;
}

int FindFunction(unsigned char *name) {
	if (!name) return -1;
	TableEntry<Function> *e = VM.Functions.Find(name);
	if (e) return e->index;
	/*e = Functions.Add(name);
	if (!e) return -1;
	e->data.offset = -1;
	e->data.type = UNDEFINED;
	return e->index;
	//*/
	return -1;
}

void RunNative(ScriptValue &s, int id, unsigned int flags, ScriptValue &out);

int RunFunction(ObjectValue *object, int id, ScriptMode mode) {
	ScriptValue s;
	s.type = -1;
	s.intVal = 0;
	return RunFunction(id, s, mode, object);
}

int RunFunction(int id, ScriptMode mode) {
	ScriptValue s;
	s.type = -1;
	s.intVal = 0;
	return RunFunction(id, s, mode);
}

int RunFunction(int id, __int64 value, ScriptMode mode) {
	if (id == -1) return 0;
	ScriptValue s;
	if (!CreateListValue(s, 1)) return 0;
	ScriptValue s2;
	CreateIntValue(s2, value);
	s.listVal->Set(s2, 0);
	int res = RunFunction(id, s, mode);
	//if (res != 2)
	//s.Release();
	return res;
}

//void Stack::Init(int pos) {
//}
//typedef void tMiscProc (void *);

//ScriptValue spareList = {0,0};

inline void Stack::SetTop(int top) {
	varTop = top;
	locals = vals + varTop;
}

const int typeTable[] = {SCRIPT_NULL, SCRIPT_INT, SCRIPT_DOUBLE};

void Stack::GetFirstArg(ScriptValue &sv, Op *&codePos) {
	if (!codePos->src.type) {
		sv = Pop();
		codePos ++;
	}
	else {
		GetPushValue(sv, codePos);
	}
}

void Stack::GetPushValue(ScriptValue &sv, Op *&codePos) {
	__int64 val;
	if (codePos->src.type >= OP_ELEMENT) {
		Op temp = *codePos;
		codePos->src.type -= OP_ELEMENT;
		Op *temp2 = codePos;

		// index
		ScriptValue sv2;// = Pop();
		GetPushValue(sv2, codePos);
		*temp2 = temp;
		// element
		ScriptValue sve = Pop();
		// out
		// CreateNullValue(sv);

		ScriptValue sv3;
		if (sve.type == SCRIPT_LIST) {
			CoerceIntNoRelease(sv2, sv3);
			if (sv3.intVal >= 0 && sve.listVal->numVals > sv3.intVal) {
				sv = sve.listVal->vals[sv3.intVal];
				sv.AddRef();
			}
			else {
				CreateNullValue(sv);
			}
		}
		else if (sve.type == SCRIPT_DICT) {
			sv2.AddRef();
			CoerceString(sv2, sv3);
			if (sve.dictVal->Find(sv3, sv)) {
				sv.AddRef();
			}
			else {
				CreateNullValue(sv);
			}
			sv3.Release();
		}
		else if (sve.type == SCRIPT_STRING) {
			CoerceIntNoRelease(sv2, sv3);
			if (sv3.intVal >= 0 && sv3.intVal < sve.stringVal->len) {
				CreateStringValue(sv, sve.stringVal->value + sv3.intVal, 1);
			}
			else {
				CreateNullValue(sv);
			}
		}
		else {
			CreateNullValue(sv);
		}
		sv2.Release();
		sve.Release();
		/*if (!codePos->src.size) {
			codePos++;
			return;
		}//*/
		return;
	}
	if (codePos->src.size == 1) {
		val = codePos->src.value;
	}
	else if (codePos->src.size == 3) {
		val = *(__int64*)(codePos+1);
	}
	else if (!codePos->src.size) {
		sv = Pop();
		codePos ++;
		return;
	}
	else if (codePos->src.size == 2) {
		val = *(int*)(codePos+1);
	}
	if (codePos->src.type <= OP_FLOAT) {
		CreateTypeValue(sv, typeTable[codePos->src.type], val);
	}
	else if (codePos->src.type <= OP_GLOBAL) {
		/*ScriptValue *temp = &vars[codePos->src.type-3][val];
		temp->AddRef();
		Push(*temp);
		//*/
		sv = vars[codePos->src.type-3][val];
		sv.AddRef();
	}
	else if (codePos->src.type == OP_THIS_OBJECT) {
		// object
		int i;
		if (locals[1].type == SCRIPT_OBJECT && (i = types[locals[1].objectVal->type].FindValue((int)val)) >= 0) {
			sv = locals[1].objectVal->values[i];
			sv.AddRef();
		}
		else {
			CreateNullValue(sv);
		}
	}
	else if (codePos->src.type == OP_OBJECT) {
		// object
		ScriptValue sv2 = Pop();
		int i;
		if (sv2.type == SCRIPT_OBJECT && (i = types[sv2.objectVal->type].FindValue((int)val)) >= 0) {
			sv = sv2.objectVal->values[i];
			sv.AddRef();
		}
		else {
			CreateNullValue(sv);
		}
		sv2.Release();
	}
	codePos += codePos->src.size;
}

#include "Config.h"

int Stack::Run() {
	//int done = 0;
	int int32Val;
	ScriptValue sv, sv2, sv3, sv4;
	//Op *codePos = code.code + pos;
	Op *codePos = code.code + pos;
	TableEntry<Function> *entry;
	//SetTop(varTop);
				static int ct = 0;
	while (1) {
		/*
		if (codePos - code.code == 0x1013) {
			codePos = codePos;
		}
		/*
		static int test=0;
		test ++;
		if (test == 327) {
			codePos = codePos;
		}
		if (names && (((size_t)names->locals)&1)) {
			codePos = codePos;
		}
		*/
		//errorPrintf("%08X\r\n", codePos-code.code);
		//if (top > 400) {
		//	top = top;
		//}
		switch(codePos->op) {
			case DISCARD:
				Discard();
				break;
			case PUSH:
				do {
					GetPushValue(sv, codePos);
					Push(sv);
						/*
					__int64 val;
					if (codePos->src.size == 1) {
						val = codePos->src.value;
					}
					else if (codePos->src.size == 3) {
						val = *(__int64*)(codePos+1);
					}
					else if (codePos->src.size == 2) {
						val = *(int*)(codePos+1);
					}
					if (codePos->src.type <= OP_FLOAT) {
						PushType(typeTable[codePos->src.type], val);
					}
					else if (codePos->src.type <= OP_GLOBAL) {
						ScriptValue *temp = &vars[codePos->src.type-3][val];
						temp->AddRef();
						Push(*temp);
					}
					else if (codePos->src.type == OP_OBJECT) {
						// object
						sv2 = Pop();
						int i;
						if (sv2.type == SCRIPT_OBJECT && (i = types[sv2.objectVal->type].FindValue((int)val)) >= 0) {
							sv2.objectVal->values[i].AddRef();
							Push(sv2.objectVal->values[i]);
						}
						else {
							PushNull();
						}
						sv2.Release();
					}
					else if (codePos->src.type == OP_ELEMENT) {
						// index
						sv2 = Pop();
						// element
						sv = Pop();
						// out
						CreateNullValue(sv4);
						if (sv.type == SCRIPT_LIST) {
							CoerceIntNoRelease(sv2, sv3);
							if (sv3.intVal >= 0 && sv.listVal->numVals > sv3.intVal) {
								sv4 = sv.listVal->vals[sv3.intVal];
								sv4.AddRef();
							}
						}
						else if (sv.type == SCRIPT_DICT) {
							sv2.AddRef();
							CoerceString(sv2, sv3);
							sv.dictVal->Find(sv3, sv4);
							sv4.AddRef();
							sv3.Release();
						}
						else if (sv.type == SCRIPT_STRING) {
							CoerceIntNoRelease(sv2, sv3);
							if (sv3.intVal >= 0 && sv3.intVal < sv.stringVal->len) {
								CreateStringValue(sv4, sv.stringVal->value+sv3.intVal, 1);
							}
						}
						Push(sv4);
						sv2.Release();
						sv.Release();
					}
					codePos += codePos->src.size;
					//*/
				}
				while (codePos->op == PUSH);
				continue;
			case POP:
				{
					__int64 val;
					if (codePos->src.size == 1) {
						val = codePos->src.value;
					}
					else if (codePos->src.size == 3) {
						val = *(__int64*)(codePos+1);
					}
					else if (codePos->src.size == 2) {
						val = *(int*)(codePos+1);
					}

					if (codePos->src.type <= OP_GLOBAL) {
						ScriptValue *temp = &vars[codePos->src.type-3][val];
						temp->Release();
						*temp = Pop();
					}
					else /*if (codePos->src.type == OP_OBJECT*/ {
						// val
						sv = Pop();
						// object
						sv2 = Pop();
						int i;
						if (sv2.type == SCRIPT_OBJECT && (i = types[sv2.objectVal->type].FindValue((int)val)) >= 0) {
							sv2.objectVal->values[i].Release();
							sv2.objectVal->values[i] = sv;
							sv2.objectVal->Release();
						}
						else {
							sv2.Release();
							sv.Release();
						}
					}

					codePos += codePos->src.size;
				}
				continue;
			/*case PUSH_THIS:
				{
					int i;
					int val;
					if (codePos->src.size == 1) {
						val = codePos->src.value;
					}
					/*else if (codePos->src.size == 3) {
						val = *(__int64*)(codePos+1);
					}//*/
			/*		else {//if (codePos->src.size == 2) {
						val = *(int*)(codePos+1);
					}
					if (locals[1].type == SCRIPT_OBJECT && (i = types[locals[1].objectVal->type].FindValue(val)) >= 0) {
						locals[1].objectVal->values[i].AddRef();
						Push(locals[1].objectVal->values[i]);
					}
					else {
						PushNull();
					}
					codePos += codePos->src.size;
				}
				continue;
				//*/
			case POP_THIS:
				{
					int i;
					int val;
					if (codePos->src.size == 1) {
						val = codePos->src.value;
					}
					/*else if (codePos->src.size == 3) {
						val = *(__int64*)(codePos+1);
					}//*/
					else { //if (codePos->src.size == 2) {
						val = *(int*)(codePos+1);
					}
					if (locals[1].type == SCRIPT_OBJECT && (i = types[locals[1].objectVal->type].FindValue(val)) >= 0) {
						ScriptValue *v = locals[1].objectVal->values+i;
						v->Release();
						*v = Pop();
					}
					else {
						Pop().Release();
					}
					codePos += codePos->src.size;
				}
				continue;
				/*
			case POPG:
				state.globals[codePos->data].Release();
				state.globals[codePos->data] = Pop();
				break;
			case POPL:
				vals[varTop + codePos->data].Release();
				vals[varTop + codePos->data] = Pop();
				break;
				/*
			case ELEMENT_PUSH:
				{
					sv2 = Pop();
					sv = Pop();
					int happy = 0;
					if (sv.type == SCRIPT_LIST) {
						CoerceIntNoRelease(sv2, sv3);
						if (sv3.intVal >= 0 && sv.listVal->numVals > sv3.intVal) {
							happy = 1;
							sv4 = sv.listVal->vals[sv3.intVal];
							sv4.AddRef();
							Push(sv4);
						}
					}
					else if (sv.type == SCRIPT_DICT) {
						sv2.AddRef();
						CoerceString(sv2, sv3);
						if (sv.dictVal->Find(sv3, sv4)) {
							happy = 1;
							sv4.AddRef();
							Push(sv4);
						}
						sv3.Release();
					}
					else if (sv.type == SCRIPT_STRING) {
						CoerceIntNoRelease(sv2, sv3);
						if (sv3.intVal >= 0 && sv3.intVal < sv.stringVal->len && CreateStringValue(sv3, sv.stringVal->value+sv3.intVal, 1)) {
							happy = 1;
							Push(sv3);
						}
					}
					if (!happy)
						PushNull();
					sv2.Release();
					sv.Release();
				}
				break;
			/*case PUSHG:
				state.globals[codePos->data].AddRef();
				Push(state.globals[codePos->data]);
				break;
			case PUSHL:
				vals[varTop + codePos->data].AddRef();
				Push(vals[varTop + codePos->data]);
				break;
			case PUSH_STRING:
				state.strings[codePos->data].AddRef();
				Push(state.strings[codePos->data]);
				break;
			case PUSH_INT24:
				do {
					PushInt(codePos->data);
					codePos++;
				}
				while (codePos->op == PUSH_INT24);
				continue;
			case PUSH_INT64:
				PushInt(((__int64*)(codePos+1))[0]);
				codePos += 2;
				break;
			case PUSH_DOUBLE64:
				PushDouble(((double*)(codePos+1))[0]);
				codePos += 2;
				break;
			case PUSH_NULL:
				{
					int i = codePos->data;
					while (i) {
						Discard();
						i--;
					}
					PushNull();
					break;
				}
				*/
			case CALL:
				int32Val = VM.Functions.table[codePos->data].data.offset;
				if (int32Val > 0) {
					sv = Pop();
					PushInt(codePos - code.code);
					PushInt(varTop);
					SetTop(top);
					Push(sv);
					PushNull();
					codePos = code.code + int32Val;
					// Next opcode is always FUNCTION64, so no need to go to the switch.
					//continue;
				}
				else {
					Pop().Release();
					PushNull();
					break;
				}
			case FUNCTION64:
				if (int32Val = codePos->data) {
					sv = vals[varTop];
					for (int i = 0; i < int32Val; i++) {
						if (sv.type == SCRIPT_LIST && sv.listVal->numVals > i) {
							sv.listVal->vals[i].AddRef();
							Push(sv.listVal->vals[i]);
						}
						else {
							PushNull();
						}
					}
				}
				if (int32Val = codePos[1].high) {
					if (!AddSize(int32Val)) {
						status = -1;
						return 0;
					}
					for (int i = int32Val; i > 0; i--) {
						PushNull();
					}
				}
				if (!AddSize(codePos[1].low)) {
					status = -1;
					return 0;
				}
				else codePos += 1;
				break;
			case CALL_WAIT:
				{
					// Wait functions do nothing if can't wait.
					// Need to do nothing if they return -1.  Return value already pushed on top of stack.
					// If return 0, just need to push null.
					// If return 1, running stack will be continued later.
					// Always have to clean up the list passed to the function.
					sv = Pop();
					Function *f = &VM.Functions.table[codePos->data].data;
					codePos ++;
					int res = 0;
					if (flags&CAN_WAIT) {
						res = ((ScriptWaitProc*)f->address)(&sv, this);
					}
					sv.Release();
					if (res < 0) {
						// don't need the null.
						continue;
					}
					else if (res) {
						pos = (int)(codePos - code.code);
						return 2;
					}
					PushNull();
					continue;
				}
			case CALL_C:
				ct++;
				if (codePos->call.list) {
					//if (!spareList.listVal) {
						if (!CreateListValue(sv, 255)) {
							int i = codePos->call.list;
							if (!CreateListValue(sv, codePos->call.list-1)) {
								sv.listVal = 0;
								while (--i > 0) {
									Pop();
								}
								PushNull();
								break;
							}
						}
					//}
					//sv = spareList;
					//spareList.listVal = 0;
					int i = codePos->call.list-1;
					while (i-- > 0) {
						sv.listVal->Set(Pop(), i);
					}
				}
				else {
					sv = Pop();
				}
				CreateNullValue(sv2);
				RunNative(sv, codePos->call.function, flags, sv2);
				if (!codePos->call.discard) {
					Push(sv2);
				}
				else {
					sv2.Release();
				}
				/*if (sv.listVal->size == 255 && sv.listVal->refCount == 1 && !spareList.listVal) {
					while (sv.listVal->numVals) {
						sv.listVal->vals[--sv.listVal->numVals].Release();
					}
					spareList.listVal = sv.listVal;
				}
				else {//*/
					if (sv.listVal->refCount>1) {
						// Currently never runs, but if I ever decide I want to start keeping lists around,
						// reduces memory usage.
						sv.listVal->size = sv.listVal->numVals;
						srealloc(sv.listVal->vals, sv.listVal->numVals * sizeof(ScriptValue));
					}
					sv.Release();
				//}
				break;
			case CALL_DLL:
				sv = Pop();
				// Call dll releases as needed.
				CallDll(sv2, sv, codePos->data);
				Push(sv2);
				break;
			case DSIZE:
				GetPushValue(sv, codePos);
				if (sv.type == SCRIPT_STRING) {
					PushInt(sv.stringVal->len);
				}
				else if (sv.type == SCRIPT_LIST) {
					PushInt(sv.listVal->numVals);
				}
				else if (sv.type == SCRIPT_DICT) {
					PushInt(sv.dictVal->numEntries);
				}
				else {
					PushInt(sv.type != SCRIPT_NULL);
				}
				sv.Release();
				continue;
			case ADD:
				GetPushValue(sv2, codePos);
				sv = Pop();
				CoerceMath(sv, sv2, sv3, sv4);
				if (sv3.type & SCRIPT_INT) {
					sv3.intVal += sv4.intVal;
				}
				else {
					sv3.doubleVal += sv4.doubleVal;
				}
				Push(sv3);
				continue;
			case SUB:
				GetPushValue(sv2, codePos);
				sv = Pop();
				CoerceMath(sv, sv2, sv3, sv4);
				if (sv3.type & SCRIPT_INT) {
					sv3.intVal -= sv4.intVal;
				}
				else {
					sv3.doubleVal -= sv4.doubleVal;
				}
				Push(sv3);
				continue;
			case MUL:
				GetPushValue(sv2, codePos);
				sv = Pop();
				CoerceMath(sv, sv2, sv3, sv4);
				if (sv3.type & SCRIPT_INT) {
					sv3.intVal *= sv4.intVal;
				}
				else {
					sv3.doubleVal *= sv4.doubleVal;
				}
				Push(sv3);
				continue;
			case DIV:
				GetPushValue(sv2, codePos);
				sv = Pop();
				CoerceMath(sv, sv2, sv3, sv4);
				if (sv3.type & SCRIPT_INT) {
					if (sv4.intVal != 0) {
						sv3.intVal /= sv4.intVal;
					}
					else {
						sv3.intVal = 0;
					}
				}
				else {
					/*if (sv2.doubleVal == 0) {
						sv2.doubleVal = sv2.doubleVal;
					}//*/
					sv3.doubleVal /= sv4.doubleVal;
				}
				Push(sv3);
				continue;
			case MAKE_COPY:
				sv = Pop();
				if (sv.type & (SCRIPT_DOUBLE | SCRIPT_INT | SCRIPT_STRING)) {
					Push(sv);
				}
				else {
					if ((sv.type == SCRIPT_LIST && sv.listVal->Dupe(sv2)) ||
						(sv.type == SCRIPT_DICT && sv.dictVal->Dupe(sv2))) {
						Push(sv2);
					}
					else
						PushNull();
					sv.Release();
				}
				break;
			case APPEND:
				sv2 = Pop();
				sv = Pop();
				if ((sv.type & sv2.type) == SCRIPT_LIST && sv.listVal->Dupe(sv3)) {
					if (sv2.listVal) {
						for (int i=0; i<sv2.listVal->numVals; i++) {
							sv2.listVal->vals[i].AddRef();
							sv3.listVal->PushBack(sv2.listVal->vals[i]);
						}
					}
					Push(sv3);
				}
				else if ((sv.type & sv2.type) == SCRIPT_DICT && sv.dictVal->Dupe(sv3)) {
					if (sv2.dictVal) {
						for (int i=0; i<sv2.dictVal->numEntries; i++) {
							sv2.dictVal->entries[i].key.AddRef();
							sv2.dictVal->entries[i].val.AddRef();
							sv3.dictVal->Add(sv2.dictVal->entries[i].key, sv2.dictVal->entries[i].val);
						}
					}
					Push(sv3);
				}
				else {
					PushNull();
				}
				sv.Release();
				sv2.Release();
				break;
			case CONCAT_STRING:
				sv2 = Pop();
				sv = Pop();
				CoerceStrings(sv, sv2, sv3, sv4);
				if (AllocateStringValue(sv, sv3.stringVal->len + sv4.stringVal->len)) {
					memcpy(sv.stringVal->value, sv3.stringVal->value, sv3.stringVal->len);
					memcpy(sv.stringVal->value + sv3.stringVal->len, sv4.stringVal->value, sv4.stringVal->len);
				}
				else {
					CreateStringValue(sv);
				}

				sv3.stringVal->Release();
				sv4.stringVal->Release();
				Push(sv);
				break;
			case LIST_ADD_ELEMENT:
				sv2 = Pop();
				sv = Pop();
				if (sv.type == SCRIPT_LIST) {
					if (sv.listVal->refCount == 1) {
						sv.listVal->PushBack(sv2);
						Push(sv);
					}
					else {
						if (!sv.listVal->Dupe(sv3)) {
							PushNull();
							sv2.Release();
						}
						else {
							sv3.listVal->PushBack(sv2);
							Push(sv3);
						}
						sv.Release();
					}
				}
				else {
					//if (sv.type & SCRIPT_LIST) {
						if (!CreateListValue(sv3, 2)) {
							sv2.Release();
							PushNull();
						}
						else {
							sv3.listVal->PushBack(sv2);
							Push(sv3);
						}
					/*}
					else {
						sv2.Release();
						PushNull();
					}
					//*/
					sv.Release();
				}

				break;
			case MAKE_LIST:
				{
					if (codePos->data <= 0) {
						if (CreateListValue(sv, 2)) {
							Push(sv);
						}
						else
							PushNull();
					}
					else {
						int i = codePos->data;
						if (CreateListValue(sv, codePos->data)) {
							while (i-- > 0) {
								sv.listVal->Set(Pop(), i);
							}
							Push(sv);
						}
						else {
							while (i-- > 0) {
								Pop();
							}
							PushNull();
						}
					} 
					break;
				}
			/*case LIST_CREATE:
				sv2 = Pop();
				sv = Pop();

				break;
				//*/
			/*
			case GLOBAL_REF:
				PushRef(&Globals.table[codePos->data].data);
				break;
			case LOCAL_REF:
				PushRef(vals + varTop + codePos->data);
				break;
				//*/
			case ELEMENT_POP:
			case ELEMENT_ASSIGN:
				{
					sv3 = Pop();
					sv2 = Pop();
					sv = Pop();
					int happy = 0;
					if (sv.type == SCRIPT_LIST) {
						CoerceIntNoRelease(sv2, sv4);
						if (sv4.intVal >= 0 && (sv.listVal->size+2 > sv4.intVal || sv4.intVal < 100000)) {
							if (codePos->op != ELEMENT_POP) {
								sv3.AddRef();
								Push(sv3);
							}
							sv.listVal->Set(sv3, (int)sv4.intVal);
							happy = 1;
						}
					}
					else if (sv.type == SCRIPT_DICT) {
						if (codePos->op != ELEMENT_POP) {
							sv3.AddRef();
							Push(sv3);
						}
						sv2.AddRef();
						CoerceString(sv2, sv4);
						sv.dictVal->Add(sv4, sv3);
						happy = 1;
					}
					if (!happy) {
						if (codePos->op != ELEMENT_POP) {
							PushNull();
							sv3.Release();
						}
					}
					sv2.Release();
					sv.Release();
				}
				break;
			case ELEMENT_PEEK:
				{
					sv2 = vals[top-1];
					sv = vals[top-2];
					int happy = 0;
					if (sv.type == SCRIPT_LIST) {
						CoerceIntNoRelease(sv2, sv3);
						if (sv3.intVal >= 0 && sv.listVal->numVals > sv3.intVal) {
							happy = 1;
							sv4 = sv.listVal->vals[sv3.intVal];
							sv4.AddRef();
							Push(sv4);
						}
					}
					else if (sv.type == SCRIPT_DICT) {
						sv2.AddRef();
						CoerceString(sv2, sv3);
						if (sv.dictVal->Find(sv3, sv4)) {
							happy = 1;
							sv4.AddRef();
							Push(sv4);
						}
						sv3.Release();
					}
					else if (sv.type == SCRIPT_STRING) {
						CoerceIntNoRelease(sv2, sv3);
						if (sv3.intVal >= 0 && sv3.intVal < sv.stringVal->len && CreateStringValue(sv3, sv.stringVal->value+sv.intVal, 1)) {
							Push(sv3);
							happy = 1;
						}
					}
					if (!happy)
						PushNull();
				}
				break;
			/*case ELEMENT_POKE:
				{
					sv4 = Pop();
					sv2 = vals[top-1];
					sv = vals[top-2];
					int happy = 0;
					if (sv.type == SCRIPT_LIST) {
						CoerceIntNoRelease(sv2, sv3);
						if (sv3.intVal >= 0 && (sv.listVal->size+2 > sv3.intVal || sv3.intVal < 100000)) {
							sv.listVal->Insert(sv4, (int)sv3.intVal);
							happy = 1;
						}
					}
					else if (sv.type == SCRIPT_DICT) {
						sv2.AddRef();
						CoerceString(sv2, sv3);
						sv.dictVal->Add(sv3, sv4);
						happy = 1;
					}
					if (!happy)
						sv4.Release();
				}
				break;//*/
			/*case DEREF:
				sv = Pop();
				Push(*sv.refVal);
				sv.refVal->AddRef();
				break;//*/
			case PUSH_EMPTY_DICT:
				if (CreateDictValue(sv, 2)) {
					Push(sv);
				}
				else
					PushNull();
				break;
			case DICT_CREATE:
				sv2 = Pop();
				sv = Pop();
				CoerceString(sv, sv3);
				if (CreateDictValue(sv4, 2)) {
					sv4.dictVal->Add(sv3, sv2);
					Push(sv4);
				}
				else {
					sv2.Release();
					sv3.Release();
					PushNull();
				}
				break;
			case COMPARE:
				GetPushValue(sv4, codePos);
				//sv4 = Pop();
				sv3 = Pop();
				CoerceMath(sv3, sv4, sv, sv2);
				//if (PopCoerceNumCanCompare(sv, sv2, this)) {
					if (sv.type == SCRIPT_INT) {
						compareResult = (sv.intVal > sv2.intVal) - (sv.intVal < sv2.intVal);
					} else {
						compareResult = (sv.doubleVal > sv2.doubleVal) - (sv.doubleVal < sv2.doubleVal);
					}
				//}
				//else compareResult = 0;
				// error.
				// else compareResult = -2;
				continue;
			case COMPARE_SCI:
				sv2 = Pop();
				sv = Pop();
				CoerceString(sv, sv3);
				CoerceString(sv2, sv4);
				if (sv3.type == sv4.type && sv4.type == SCRIPT_STRING) {
					compareResult = scriptstricmp(sv3, sv4);
				}
				else compareResult = 0;
				sv3.Release();
				sv4.Release();
				break;
			case COMPARE_S:
				sv2 = Pop();
				sv = Pop();
				CoerceString(sv, sv3);
				CoerceString(sv2, sv4);
				if (sv3.type == sv4.type && sv4.type == SCRIPT_STRING) {
					compareResult = scriptstrcmp(sv3, sv4);
				}
				else compareResult = 0;
				sv3.Release();
				sv4.Release();
				break;
			case SET_E:
				PushInt(compareResult == 0);
				break;
			case SET_GE:
				PushInt(compareResult >= 0);
				break;
			case SET_G:
				PushInt(compareResult > 0);
				break;
			case SET_LE:
				PushInt(compareResult <= 0);
				break;
			case SET_L:
				PushInt(compareResult < 0);
				break;
			case SET_NE:
				PushInt(compareResult != 0);
				break;
			case RSHIFT:
				PopCoerceInt(sv, sv2, this);
				PushInt(sv.intVal >> sv2.intVal);
				break;
			case LSHIFT:
				PopCoerceInt(sv, sv2, this);
				PushInt(sv.intVal << sv2.intVal);
				break;
			case MOD:
				PopCoerceInt(sv, sv2, this);
				if (sv2.intVal)
					PushInt(sv.intVal % sv2.intVal);
				else
					PushInt(0);
				break;
			case AND:
				PopCoerceInt(sv, sv2, this);
				PushInt(sv.intVal & sv2.intVal);
				break;
			case OR:
				PopCoerceInt(sv, sv2, this);
				PushInt(sv.intVal | sv2.intVal);
				break;
			case XOR:
				PopCoerceInt(sv, sv2, this);
				PushInt(sv.intVal ^ sv2.intVal);
				break;
			case NOT:
				PushInt(!CoerceBool(Pop()));
				break;
			case NEG:
				CoerceMath(Pop(), sv);
				if (sv.type == SCRIPT_INT) {
					PushInt(-sv.intVal);
				}
				else {
					PushDouble(-sv.doubleVal);
				}
				break;
			case JMP:
				codePos += codePos->data;
				continue;
			case JMP_E:
				if (compareResult == 0) {
					codePos += codePos->data;
					continue;
				}
				break;
			case JMP_G:
				if (compareResult > 0) {
					codePos += codePos->data;
					continue;
				}
				break;
			case JMP_GE:
				if (compareResult >= 0) {
					codePos += codePos->data;
					continue;
				}
				break;
			case JMP_L:
				if (compareResult < 0) {
					codePos += codePos->data;
					continue;
				}
				break;
			case JMP_LE:
				if (compareResult <= 0) {
					codePos += codePos->data;
					continue;
				}
				break;
			case JMP_NE:
				if (compareResult != 0) {
					codePos += codePos->data;
					continue;
				}
				break;
			case TEST:
				CoerceMath(Pop(), sv);
				if (sv.type == SCRIPT_INT) {
					compareResult = (sv.intVal>0) - (sv.intVal<0);
				}
				else {
					compareResult = (sv.doubleVal>0) - (sv.doubleVal<0);
				}
				break;
			case NTEST:
				CoerceMath(Pop(), sv);
				if (sv.type == SCRIPT_INT) {
					compareResult = sv.intVal == 0;
				}
				else {
					compareResult = sv.doubleVal == 0;
				}
				break;
			case RET:
				if (codePos->data) {
					CreateNullValue(sv);
				}
				else {
					sv = Pop();
				}
				while (top > varTop) {
					Discard();
				}
				sv2 = Pop();
				SetTop((int) sv2.intVal);
				sv2 = Pop();
				pos = (int) sv2.intVal;
				if (pos < 0) {
					status = 1;
					out = sv;
					return 0;
				}
				codePos = code.code + pos;
				Push(sv);
				break;
			case DUPE:
				vals[top-1].AddRef();
				Push(vals[top-1]);
				break;
			case SWAP:
				sv = vals[top-1];
				vals[top-1] = vals[top-2];
				vals[top-2] = sv;
				break;
			case OBJECT_ASSIGN:
				// val
				sv = Pop();
				// object
				sv2 = Pop();
				if (sv2.type == SCRIPT_OBJECT) {
					ObjectType * type = &types[sv2.objectVal->type];
					int i = type->FindValue(codePos->data);
					if (i >= 0) {
						sv2.objectVal->values[i].Release();
						sv2.objectVal->values[i] = sv;
						sv.AddRef();
					}
				}
				sv2.Release();
				Push(sv);
				break;
			case OBJECT_PEEK:
				sv2 = vals[top-1];
				if (sv2.type == SCRIPT_OBJECT) {
					ObjectType * type = &types[sv2.objectVal->type];
					int i = type->FindValue(codePos->data);
					if (i >= 0) {
						sv2.objectVal->values[i].AddRef();
						Push(sv2.objectVal->values[i]);
						break;
					}
				}
				PushNull();
				break;
				/*
			case OBJECT_PUSH:
				// object
				sv2 = Pop();
				if (sv2.type == SCRIPT_OBJECT) {
					ObjectType * type = &types[sv2.objectVal->type];
					int i = type->FindValue(codePos->data);
					if (i >= 0) {
						sv2.objectVal->values[i].AddRef();
						Push(sv2.objectVal->values[i]);
						sv2.Release();
						break;
					}
				}
				sv2.Release();
				PushNull();
				break;
				//*/
			case MAKE_OBJECT:
				if (CreateObjectValue(sv, codePos->data)) {
					Push(sv);
				}
				else
					PushNull();
				break;
			case REF_OBJECT_CALL:
				// args
				sv = Pop();
				// fxn
				sv2 = Pop();
				// object
				sv4 = Pop();
				if (sv4.type == SCRIPT_OBJECT) {
					int i;
					if (sv2.type == SCRIPT_STRING && (i = VM.StringTable.Find(sv2.stringVal)) >= 0) {
						ObjectType * type = &types[sv4.objectVal->type];
						if ((i = type->FindFunction(i)) >= 0) {
							sv2.Release();
							pos = (int)(codePos - code.code);
							entry = &VM.Functions.table[type->functs[i].functionID];
							int res = Call(*entry, sv, sv4);
							if (res > 0) return res;
							codePos = code.code + pos;
							continue;
						}
					}
				}
				sv4.Release();
				sv2.Release();
				sv.Release();
				PushNull();
				break;
			case REF_CALL:
				// args
				sv = Pop();
				// fxn
				sv2 = Pop();
				if (sv2.type == SCRIPT_STRING && !strchr(sv2.stringVal->value, ':') && (entry = VM.Functions.Find(sv2.stringVal->value))) {
					pos = (int)(codePos - code.code);
					CreateNullValue(sv4);
					int res = Call(*entry, sv, sv4);
					sv2.Release();
					if (res > 0) return res;
					codePos = code.code + pos;
					//SetTop(varTop);
					continue;
				}
				sv2.Release();
				sv.Release();
				PushNull();
				break;
			case CALL_OBJECT:
				// args
				sv = Pop();
				// object
				sv2 = Pop();
				if (sv2.type == SCRIPT_OBJECT) {
					ObjectType * type = &types[sv2.objectVal->type];
					int i = type->FindFunction(codePos->data);
					if (i >= 0) {
						pos = (int)(codePos - code.code);
						entry = &VM.Functions.table[type->functs[i].functionID];
						int res = Call(*entry, sv, sv2);
						if (res > 0) return res;
						codePos = code.code + pos;
						//SetTop(varTop);
						continue;
					}
				}
				sv.Release();
				sv2.Release();
				PushNull();
				break;
			case NOP:
				break;

		}
		//if (done) break;
		codePos ++;//= OpSize(codePos->op);
	}
	//sv.Release();
	//if (done == -1) return 0;
	return 1;
}

int Stack::Call(const TableEntry<Function> &f, ScriptValue &argList, ScriptValue &obj) {
	ScriptValue sv;
	if (f.data.type == SCRIPT) {
		if (f.data.offset >= 0) {
			PushInt(pos);
			PushInt(varTop);
			SetTop(top);
			Push(argList);
			Push(obj);
			pos = f.data.offset;
			return 0;
		}
	}
	else if (f.data.type == DLL_CALL) {
		CallDll(sv, argList, f.index);
		Push(sv);
		obj.Release();
		pos++;
		return 0;
	}
	else if (f.data.type != UNDEFINED && f.data.type != ALIAS) {
		if (f.data.type != C_directwait) {
			pos ++;
			if (f.data.needThis)
				sv = obj;
			else
				CreateNullValue(sv);
			RunNative(argList, f.index, flags, sv);
			Push(sv);
			obj.Release();
			argList.Release();
			return 0;
		}
		else {
			if (flags&CAN_WAIT) {
				pos ++;
				// Should be locals[1], but doesn't matter, as long as I can access it.
				ScriptValue oldObj = this->vals[1];
				this->vals[1] = obj;
				int res = ((ScriptWaitProc*)f.data.address)(&argList, this);
				argList.Release();
				obj.Release();
				this->vals[1] = oldObj;
				if (res) {
					return 2;
				}
				if (res == 0) {
					PushNull();
				}
				return 0;
			}
		}
	}
	argList.Release();
	obj.Release();
	PushNull();
	return -1;
}


int RunFunction(int id, ScriptValue &s, ScriptMode mode, ObjectValue *obj) {
	if (id < 0) {
		s.Release();
		if (obj) obj->Release();
		return 0;
	}
	ScriptValue sv2;
	if (!obj) {
		CreateNullValue(sv2);
	}
	else {
		CreateObjectValue(sv2, obj);
	}
	if (VM.Functions.table[id].data.type != SCRIPT) {
		if (VM.Functions.table[id].data.type == UNDEFINED) {
			s.Release();
			sv2.Release();
			return 0;
		}
		if (VM.Functions.table[id].data.type == CALL_DLL) {
			// Auto-releases.
			CallDll(sv2, s, VM.Functions.table[id].data.offset);
		}
		else {
			// Doesn't auto-release.
			RunNative(s, id, mode, sv2);
			s.Release();
		}
		sv2.Release();
		//if (obj) obj->Release();
		return 1;
	}
	int pos = VM.Functions.table[id].data.offset;
	Stack *stack = VM.CreateStack(pos, s, mode, sv2);
	if (!stack) {
		s.Release();
		sv2.Release();
		return 0;
	}

	int res = VM.RunStack(stack, sv2);
	if (res == 2) return 2;
	return 1;
}

void RunNative(ScriptValue &s, int id, unsigned int flags, ScriptValue &out) {
	//ScriptValue temp[3];
	if (!(s.type & SCRIPT_LIST)) return;
	Function *f = &VM.Functions.table[id].data;
	//FunctionType t = f.type;
	ScriptValue args[50];
	int i = 0;
	//if (f->type == C_directwait) return out;
	if (f->type == C_direct) {
		f->address(out, &s);
		return;
	}
	if (f->type <= C_noargsDraw && !(flags & CAN_DRAW)) {
		return;
	}
	ArgListElement *a = argLists.list + argLists.argLists[f->type];
	int pos = 0;
	int numVals = 0;
	if (s.type == SCRIPT_LIST) {
		numVals = s.listVal->numVals;
	}
	while (a[i].count) {
		if (a[i].type == SCRIPT_STRING) {
			int j;
			for (j=0; j<a[i].count; j++) {
				if (pos >= numVals) {
					CreateStringValue(args[pos]);
				}
				else {
					s.listVal->vals[pos].AddRef();
					CoerceString(s.listVal->vals[pos], args[pos]);
				}
				pos++;
			}
			if (j<a[i].count) {
				break;
			}
			i++;
		}
		else if (a[i].type == SCRIPT_DOUBLE) {
			for (int j=0; j<a[i].count; j++) {
				if (pos >= numVals) {
					CreateDoubleValue(args[pos], 0);
				}
				else {
					CoerceDoubleNoRelease(s.listVal->vals[pos], args[pos]);
				}
				pos++;
			}
			i++;
		}
		else if (a[i].type == SCRIPT_INT) {
			for (int j=0; j<a[i].count; j++) {
				if (pos >= numVals) {
					CreateIntValue(args[pos], 0);
				}
				else {
					CoerceIntNoRelease(s.listVal->vals[pos], args[pos]);
				}
				pos++;
			}
			i++;
		}
		else if (a[i].type == SCRIPT_OBJECT) {
			for (int j=0; j<a[i].count; j++) {
				if (pos >= numVals) {
					CreateNullValue(args[pos]);
				}
				else {
					args[pos] = s.listVal->vals[pos];
					args[pos].AddRef();
				}
				pos++;
			}
			i++;
		}
	}
	if (a[i].count == 0) {
		f->address(out, args);
	}
	for (i=0; i<pos; i++) {
		args[i].Release();
	}
	return;
}

void Stack::Cleanup() {
	for (int i=0; i<top; i++) {
		vals[i].Release();
	}
	free(vals);
	//out.Release();
	//free(this);
}

int Stack::AddSize(int size) {
	if (size + top < maxSize) return 1; 
	if (top > 1000000) return 0;
	else {
		if (!srealloc(vals, size+maxSize*2*sizeof(ScriptValue))) return 0;
		SetTop(varTop);
		maxSize=size+maxSize*2;
		return 1;
	}
}

void VirtualMachine::Cleanup() {
	int i;
	for (i=0; i<VM.numStacks; i++) {
		stacks[i]->Cleanup();
	}
	for (i=0; i<VM.maxStacks; i++) {
		free(stacks[i]);
	}
	free(stacks);

	for (i=0; i<VM.Globals.numValues; i++) {
		VM.globals[i].Release();
	}
	free(VM.globals);
	VM.Globals.Cleanup();

	VM.StringTable.Cleanup();
	VM.Functions.Cleanup();

	/*
	if (spareList.listVal) {
		spareList.Release();
		spareList.listVal = 0;
	}//*/
}

int RunStack(Stack *s) {
	ScriptValue sv;
	int res = VM.RunStack(s, sv);
	sv.Release();
	return res;
}

void VirtualMachine::CleanupStack(Stack *s) {
	s->Cleanup();
	for (int i=numStacks-1; i>=0; i--) {
		if (stacks[i] == s) {
			Stack *temp = s;
			stacks[i] = stacks[--numStacks];
			stacks[numStacks] = temp;
			return;
		}
	}
	free(s);
}

int VirtualMachine::RunStack(Stack *s, ScriptValue &out) {
	int res = s->Run();
	if (res == 2) {
		CreateNullValue(out);
		return 2;
	}
	out = s->out;
	CleanupStack(s);
	return 1;
}

Stack *VirtualMachine::CreateStack(int pos, ScriptValue &s, unsigned int flags, ScriptValue &obj) {
	if (pos < 0) return 0;
	if (numStacks == maxStacks) {
		if (!srealloc(stacks, (maxStacks+4)*sizeof(Stack*))) return 0;
		memset(stacks+maxStacks, 0, sizeof(Stack*) * 4);
		maxStacks += 4;
		if (numStacks == 100) {
			errorPrintf(2, "100 active stacks.  Possible leak?\n");
		}
	}
	Stack *stack;
	if (stacks[numStacks]) {
		stack = stacks[numStacks];
	}
	else {
		stacks[numStacks] = stack = (Stack*)malloc(sizeof(Stack));
	}
	memset(stack, 0, sizeof(Stack));
	if (!stack) return 0;
	numStacks++;
	stack->flags = flags;
	stack->pos = pos;
	if (!stack->AddSize(10000)) {
		free(stack);
		return 0;
	}
	stack->PushInt(-1);
	stack->PushInt(-1);
	stack->Push(s);
	stack->Push(obj);
	CreateIntValue(stack->out, 0);
	stack->globals = VM.globals;
	stack->strings = VM.StringTable.table;
	stack->SetTop(2);
	return stack;
}

