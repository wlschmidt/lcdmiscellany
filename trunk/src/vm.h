#ifndef VM_H
#define VM_H

#include "ScriptEnums.h"

#include "ScriptValue.h"
#include "SymbolTable.h"
//#include "Script.h"

void CoerceIntNoRelease(ScriptValue &sv, ScriptValue& sv2);
void CoerceString(ScriptValue &sv, ScriptValue& sv2);

enum ScriptMode {
	CAN_DO_NOTHING=0,
	CAN_DRAW=1,
	CAN_WAIT=2
};

struct ArgListElement {
	unsigned char type;
	unsigned char count;
};

struct ArgLists {
	unsigned short *argLists;
	ArgListElement *list;
	int numArgLists;
	unsigned short listLen;
	void Cleanup();
	int AddList(int id, const unsigned char *args);
};

extern ArgLists argLists;

enum FunctionType {
	SCRIPT,
	DLL_CALL,
	UNDEFINED,
	ALIAS,
	C_4intsDraw,
	C_2intsDraw,
	C_intDraw,
	C_string4intsDraw,
	// Must be last drawing function.
	C_noargsDraw,

	C_2doubles,
	C_double,

	C_2ints4strings,

	C_3ints,
	C_4ints,
	C_5ints,
	C_stringintstring,
	C_obj6ints,
	//C_7ints,
	C_string5ints,
	C_string4ints,
	C_string3ints,
	C_string2ints,
	C_string2intsobj,
	C_2strings2ints,
	C_2strings3ints,
	C_3strings3ints,
	C_obj,
	C_2ints,
	C_noArgs,
	C_stringint,
	C_intstring,
	C_3strings,
	C_3stringsint,
	C_stringobj,
	C_2strings,
	C_2stringsobj,
	C_2strings2objs,
	C_2objs,
	C_string2objs,
	C_2stringsint,
	C_2stringsintstring,
	C_int,
	C_stringintobj,
	C_string,
	C_double2ints,
	C_double3ints,
	C_stringdouble,
	C_stringdoubleint,
	C_doubleint,
	C_direct,
	C_directwait,
};

typedef void ScriptProc (ScriptValue &s, ScriptValue *vals);

struct Function {
	FunctionType type:16;
	int needThis:16;
	union {
		int offset;
		ScriptProc *address;
	};
};

struct Code {
	Op *code;
	int codeDwords;
};
extern Code code;


struct Stack {
private:
	void GetPushValue(ScriptValue &sv, Op *&codePos);
public:

	union {
		ScriptValue *vars[3];
		struct {
			ScriptValue *strings, *locals, *globals;
		};
	};
	ScriptValue out;
	ScriptValue *vals;
	unsigned int flags;
	int pos;
	int maxSize;
	int top;
	int varTop;
	int compareResult;
	int status;
	//void Init(int pos);
	int AddSize(int size);
	void SetTop(int top);
	void GetFirstArg(ScriptValue &sv, Op *&codePos);

	inline void PushType(int type, const __int64 &i) {
		CreateTypeValue(vals[top++], type, i);
	}
	inline void PushInt(const __int64 &i) {
		CreateIntValue(vals[top++], i);
	}
	inline void PushDouble(const double &d) {
		CreateDoubleValue(vals[top++], d);
	}
	inline void PushNull() {
		CreateNullValue(vals[top++]);
	}
	inline void Discard() {
		vals[--top].Release();
	}
	inline ScriptValue &Pop() {
		return vals[--top];
	}
	inline void Push(ScriptValue &v) {
		vals[top ++] = v;
	}
	inline void Push(StringValue *s) {
		vals[top].type = SCRIPT_STRING;
		vals[top ++].stringVal = s;
	}
	/*
	inline void PushRef(ScriptValue *v) {
		vals[top].refVal = v;
		vals[top ++].type = SCRIPT_REFERENCE;
	}//*/
	inline ScriptValue &GetTop() {
		return vals[top-1];
	}

	int Call(const TableEntry<Function> &f, ScriptValue &argList, ScriptValue &obj);

	friend struct VirtualMachine;
protected:
	int Run();
	// Does not release stack itself.
	void Cleanup();
};

int RunStack(Stack *s);

struct VirtualMachine {
	StringTableStruct StringTable;
	SymbolTable<Function> Functions;
	SymbolTable<bool> Globals;
	// Contains values.
	ScriptValue *globals;
	Stack **stacks;
	int numStacks;
	int maxStacks;

	void Cleanup();

	int RunStack(Stack *s, ScriptValue &out);
	Stack *CreateStack(int pos, ScriptValue &s, unsigned int flags, ScriptValue &obj);
	void CleanupStack(Stack* s);
};

extern VirtualMachine VM;

int RunFunction(ObjectValue *object, int id, ScriptMode mode=CAN_WAIT);
int RunFunction(int id, ScriptMode mode=CAN_WAIT);
int RunFunction(int id, __int64 value, ScriptMode mode=CAN_WAIT);
int RunFunction(int id, ScriptValue &s, ScriptMode mode=CAN_WAIT, ObjectValue *v = 0);
int FindFunction(unsigned char *name);


typedef int ScriptWaitProc (ScriptValue *vals, Stack *stack);

__int64 CoerceIntNoRelease(ScriptValue &sv);

void CoerceIntNoRelease(ScriptValue &sv, ScriptValue& sv2);
void CoerceDoubleNoRelease(ScriptValue &sv, ScriptValue& sv2);
void CoerceMath(ScriptValue &sv, ScriptValue& sv2);
#endif

