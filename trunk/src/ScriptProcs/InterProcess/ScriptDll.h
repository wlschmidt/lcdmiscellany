#ifndef SCRIPT_DLL_H
#define SCRIPT_DLL_H

#include "../../ScriptValue.h"



int CreateDll(unsigned char *alias, unsigned char *dll, unsigned char *init);
int CreateDllFunction(unsigned char *function, unsigned char *dllAlias, unsigned char returnType, unsigned char *argTypes);
void CallDll(ScriptValue &out, ScriptValue &in, int id);
void UnloadDll(ScriptValue &out, ScriptValue *args);

void CleanupDlls();

#endif
