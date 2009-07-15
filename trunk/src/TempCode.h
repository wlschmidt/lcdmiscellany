#ifndef TEMPCODE_H
#define TEMPCODE_H

#include "ScriptEnums.h"
#include "ScriptValue.h"

struct ParseTree;

struct TempCode {
	TempCode *last;
	TempCode *prev;
	TempCode *next;
	OpCode op;
	int lineNumber;
	union {
		__int64 intVal;
		double doubleVal;
		struct {
			int param1;
			int param2;
			int param3;
			int param4;
		};
	};
	ScriptValue sv;
};

//TempCode *MakeCode(OpCode op, __int64 val);
//TempCode *MakeCode(OpCode op, int param1, int param2, int param3);
//TempCode *MakeCode(OpCode op);

TempCode *GenTempCode(ParseTree* t);

void CleanupTempCode(TempCode *code);
void CleanupDefines();

#endif
