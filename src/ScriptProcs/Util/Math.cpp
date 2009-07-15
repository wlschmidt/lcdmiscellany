#include "Math.h"
#include <math.h>
#include "../../vm.h"
#include "../../util.h"

void abs(ScriptValue &s, ScriptValue *args) {
	if (args->type == SCRIPT_LIST && args->listVal->numVals > 0) {
		args[0].listVal->vals[0].AddRef();
		CoerceMath(args[0].listVal->vals[0], s);
		if (s.type == SCRIPT_INT) {
			if (s.intVal < 0) s.intVal = -s.intVal;
		}
		//else if (s.type == SCRIPT_DOUBLE) {
		else if (s.doubleVal < 0) s.doubleVal = -s.doubleVal;
		//}
		return;
	}
	CreateIntValue(s, 0);
}

void log(ScriptValue &s, ScriptValue *args) {
	CreateDoubleValue(s, log(args->doubleVal));
}

void log10(ScriptValue &s, ScriptValue *args) {
	CreateDoubleValue(s, log10(args->doubleVal));
}

void cos(ScriptValue &s, ScriptValue *args) {
	CreateDoubleValue(s, cos(args->doubleVal));
}

void sin(ScriptValue &s, ScriptValue *args) {
	CreateDoubleValue(s, sin(args->doubleVal));
}

void acos(ScriptValue &s, ScriptValue *args) {
	CreateDoubleValue(s, acos(args->doubleVal));
}

void asin(ScriptValue &s, ScriptValue *args) {
	CreateDoubleValue(s, asin(args->doubleVal));
}

void tan(ScriptValue &s, ScriptValue *args) {
	CreateDoubleValue(s, tan(args->doubleVal));
}

void atan(ScriptValue &s, ScriptValue *args) {
	CreateDoubleValue(s, atan(args->doubleVal));
}

void sqrt(ScriptValue &s, ScriptValue *args) {
	CreateDoubleValue(s, sqrt(args->doubleVal));
}

void exp(ScriptValue &s, ScriptValue *args) {
	CreateDoubleValue(s, exp(args->doubleVal));
}

void pow(ScriptValue &s, ScriptValue *args) {
	CreateDoubleValue(s, pow(args[0].doubleVal, args[1].doubleVal));
}


void rand(ScriptValue &s, ScriptValue *args) {
	if (args[0].intVal > args[1].intVal) {
		__int64 t = args[0].intVal;
		args[0].intVal = args[1].intVal;
		args[1].intVal = t;
	}
	unsigned __int64 range = args[1].intVal - args[0].intVal;
	if (range == (unsigned __int64) 0) {
		s = args[0];
	}
	else {
		unsigned __int64 v;
		while(1) {
			unsigned __int64 n = GetNumber();
			v = n%range;
			if (((unsigned __int64)(0-range)) > v*range) break;
		}
		CreateIntValue(s, v + args[0].intVal);
	}
}

void randf(ScriptValue &s, ScriptValue *args) {
	if (args[1].doubleVal==args[0].doubleVal) {
		s = args[0];
	}
	else {
		CreateDoubleValue(s, args[0].doubleVal + GetNumber()*(args[1].doubleVal-args[0].doubleVal)/(unsigned __int64)-1);
	}
}
