#ifndef SCRIPT_H
#define SCRIPT_H

#include "vm.h"
#include "SymbolTable.h"
#include "ScriptEnums.h"

/*typedef void sevenIntProc (ScriptValue &s, int, int, int, int, int, int, int);
typedef void fourIntProc (ScriptValue &s, int, int, int, int);
typedef void charThreeIntProc (ScriptValue &s, unsigned char*, int, int, int);
typedef void twoIntProc (ScriptValue &s, int, int);
typedef void oneCharInt64Proc (ScriptValue &s, unsigned char *, __int64);
typedef void threeCharProc (ScriptValue &s, unsigned char *, unsigned char*, unsigned char*);
typedef void intProc (ScriptValue &s, int);
typedef void charProc (ScriptValue &s, unsigned char*);
typedef void noArgProc (ScriptValue &s);
typedef void doubleTwoIntProc (ScriptValue &s, double, int, int);
typedef void charDoubleProc (ScriptValue &s, unsigned char*, double);
*/



//ScriptValue *source(ScriptValue *s);
//ScriptValue *exec(unsigned char* string, ScriptValue *names);
int LoadScript(int i, int import = 0);
int AddFile(wchar_t *fileName, wchar_t *path, int &index);
int LoadScript(unsigned char *fileName);
void CleanupScripting();
int RegisterProcs();
int RegisterProc(unsigned char* name, ScriptProc *proc, FunctionType type);
void CleanupCompiler();

void CheckFxns();

#endif
