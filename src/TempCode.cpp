#include "TempCode.h"
#include "vm.h"
#include "util.h"
#include "Script.h"
#include "Config.h"
//#include <malloc.h>
//#include <stdio.h>
#include "Parser.h"
#include "ScriptProcs/InterProcess/ScriptDll.h"
#include "ScriptObjects/ScriptObjects.h"
#include <math.h>
#include <stdarg.h>

struct ParentTypeInfo {
	unsigned char* name;
	int len;
	ParseTree *tree;
	ObjectType *type;
};

int errorLine;
void CompileError(ParseTree *token, char *message) {
	errorLine = token->line;
	errorPrintf(2, "Error, line %i: %s\r\n", token->line, message);
	errors++;
	DisplayErrorLine(token->start, token->pos);
}

void CompileWarning(ParseTree *token, char *message) {
	errorPrintf(2, "Warning, line %i: %s\r\n", token->line, message);
	warnings++;
	DisplayErrorLine(token->start, token->pos);
}

void __cdecl CustomCompilerWarning(ParseTree *token, char *string, ...) {
	va_list arg;

	va_start (arg, string);
	verrorPrintf (2, string, arg);
	va_end (arg);
	DisplayErrorLine(token->start, token->pos);
}


int optimize;
TempCode *MakeCode(int num, OpCode op) {
	TempCode *out = (TempCode*) calloc(sizeof(TempCode), 1);
	if (!out) return 0;
	CreateNullValue(out->sv);
	out->lineNumber = num;
	out->op = op;
	return out;
}

TempCode *MakeCode(int num, OpCode op, __int64 val) {
	TempCode *out = MakeCode(num, op);
	if (!out) return 0;
	out->lineNumber = num;
	out->op = op;
	out->intVal = val;
	return out;
}

TempCode *MakeCode(int num, OpCode op, __int64 val, int param3) {
	TempCode *out = MakeCode(num, op);
	if (!out) return 0;
	out->lineNumber = num;
	out->op = op;
	out->intVal = val;
	out->param3 = param3;
	return out;
}

TempCode *MakeCode(int num, OpCode op, int val) {
	TempCode *out = MakeCode(num, op);
	if (!out) return 0;
	out->lineNumber = num;
	out->op = op;
	out->intVal = val;
	return out;
}

TempCode *MakeCode(int num, OpCode op, double doubleVal) {
	TempCode *out = MakeCode(num, op);
	if (!out) return 0;
	out->lineNumber = num;
	out->op = op;
	out->doubleVal = doubleVal;
	return out;
}

TempCode *MakeCode(int num, OpCode op, int param1, int param2, int param3, int param4) {
	TempCode *out = MakeCode(num, op);
	if (!out) return 0;
	out->lineNumber = num;
	out->op = op;
	out->param1 = param1;
	out->param2 = param2;
	out->param3 = param3;
	out->param4 = param4;
	return out;
}

TempCode *MakeCode(int num, OpCode op, __int64 val, int param3, int param4) {
	TempCode *out = MakeCode(num, op);
	if (!out) return 0;
	out->lineNumber = num;
	out->op = op;
	out->intVal = val;
	out->param3 = param3;
	out->param4 = param4;
	return out;
}

TempCode *MakeCode(int num, OpCode op, int val, int param3, int param4) {
	TempCode *out = MakeCode(num, op);
	if (!out) return 0;
	out->lineNumber = num;
	out->op = op;
	out->intVal = val;
	out->param3 = param3;
	out->param4 = param4;
	return out;
}

TempCode *MakeCode(int num, OpCode op, double val, int param3, int param4) {
	TempCode *out = MakeCode(num, op);
	if (!out) return 0;
	out->lineNumber = num;
	out->op = op;
	out->doubleVal = val;
	out->param3 = param3;
	out->param4 = param4;
	return out;
}

void CleanupTempCode(TempCode *code) {
	if (code) {
		while(code->prev) {
			code = code->prev;
		}
		while (code) {
			TempCode *temp = code->next;
			code->sv.Release();
			free(code);
			code = temp;
		}
	}
}

inline TempCode *GetLast(TempCode *c) {
	while (1) {
		if (c->last) c = c->last;
		else if (c->next) c = c->next;
		else break;
	}
	return c;
}

inline void Chain(TempCode *first, TempCode *last) {
	TempCode * t= GetLast(first);
	t->next = last;
	last->prev = t;
	if (last->last)
		first->last = t->last = last->last;
	else
		first->last = t->last = last;
}

int jump;
TempCode *continueTo;
TempCode *breakTo;
ObjectType *type;

TempCode *GenTempCode(ParseTree* t, SymbolTable<int> &locals, int &maxDepth) {
	TempCode *c1=0, *c2=0, *c3=0, *c4=0, *c5=0;
	maxDepth++;
	switch (t->type) {
		case NODE_INT:
			return MakeCode(t->line, PUSH, t->intVal, 3, OP_INT);
		case NODE_DOUBLE:
			return MakeCode(t->line, PUSH, t->doubleVal, 3, OP_FLOAT);
/*		case NODE_DICT_TO_LIST:
			c1 = GenTempCode(t->children[0], locals, maxDepth);
			c2 = MakeCode(t->line, DICT_TO_LIST);
			if (c1 && c2) {
				Chain(c1, c2);
				return c1;
			}
			break;//*/
		case NODE_DISCARD_LAST:
			c1 = GenTempCode(t->children[0], locals, maxDepth);
			c2 = MakeCode(t->line, DISCARD);
			if (c1 && c2) {
				Chain(c1, c2);
				return c1;
			}
			break;
		case NODE_ADD:
			c3 = MakeCode(t->line, ADD);
			goto C1C2;
		case NODE_SUB:
			c3 = MakeCode(t->line, SUB);
			goto C1C2;
		case NODE_MUL:
			c3 = MakeCode(t->line, MUL);
			goto C1C2;
		case NODE_LSHIFT:
			c3 = MakeCode(t->line, LSHIFT);
			goto C1C2;
		case NODE_RSHIFT:
			maxDepth -= 2;
			c3 = MakeCode(t->line, RSHIFT);
			goto C1C2;
		case NODE_DIV:
			maxDepth -= 2;
			c3 = MakeCode(t->line, DIV);
			goto C1C2;
		case NODE_MOD:
			maxDepth -= 2;
			c3 = MakeCode(t->line, MOD);
			goto C1C2;
		case NODE_XOR:
			maxDepth -= 2;
			c3 = MakeCode(t->line, XOR);
			goto C1C2;
		case NODE_OR:
			maxDepth -= 2;
			c3 = MakeCode(t->line, OR);
			goto C1C2;
		case NODE_MAKE_COPY:
			c2 = MakeCode(t->line, MAKE_COPY);
			goto C1;
		case NODE_AND:
			maxDepth -= 2;
			c3 = MakeCode(t->line, AND);
			goto C1C2;
		case NODE_E:
		case NODE_G:
		case NODE_GE:
		case NODE_L:
		case NODE_LE:
		case NODE_NE:
			maxDepth -= 2;
			c1 = GenTempCode(t->children[0], locals, maxDepth);
			c2 = GenTempCode(t->children[1], locals, maxDepth);
			if (c1 && c2) {
				if (!c1->next && c1->op == PUSH && c1->param4 == OP_INT && (c1->intVal == 0 || c2->next || c2->param4 != OP_INT || c2->op != PUSH)) {
					TempCode *t2 = c2;
					c2 = c1;
					c1 = t2;
					if (t->type == NODE_G) {
						t->type = NODE_L;
					}
					else if (t->type == NODE_GE) {
						t->type = NODE_LE;
					}
					else if (t->type == NODE_LE) {
						t->type = NODE_GE;
					}
					else if (t->type == NODE_L) {
						t->type = NODE_G;
					}
				}
				c3 = MakeCode(t->line, OpCode(COMPARE));
				c4 = MakeCode(t->line, OpCode(SET_E + t->type-NODE_E));
			}
			goto Push4;
		case NODE_S_E:
		case NODE_S_G:
		case NODE_S_GE:
		case NODE_S_L:
		case NODE_S_LE:
		case NODE_S_NE:
			maxDepth -= 2;
			c1 = GenTempCode(t->children[0], locals, maxDepth);
			c2 = GenTempCode(t->children[1], locals, maxDepth);
			c3 = MakeCode(t->line, OpCode(COMPARE_S));
			c4 = MakeCode(t->line, OpCode(SET_E + t->type-NODE_S_E));
			goto Push4;
		case NODE_SCI_E:
		case NODE_SCI_G:
		case NODE_SCI_GE:
		case NODE_SCI_L:
		case NODE_SCI_LE:
		case NODE_SCI_NE:
			maxDepth -= 2;
			c1 = GenTempCode(t->children[0], locals, maxDepth);
			c2 = GenTempCode(t->children[1], locals, maxDepth);
			c3 = MakeCode(t->line, OpCode(COMPARE_SCI));
			c4 = MakeCode(t->line, OpCode(SET_E + t->type-NODE_SCI_E));
			goto Push4;
		case NODE_CONCAT_STRING:
			c3 = MakeCode(t->line, CONCAT_STRING);
			maxDepth -= 2;
			goto C1C2;
C1C2:
			c1 = GenTempCode(t->children[0], locals, maxDepth);
			c2 = GenTempCode(t->children[1], locals, maxDepth);
Push3:
			if (c1 && c2 && c3) {
				Chain(c1, c2);
				Chain(c2, c3);
				return c1;
			}
			break;
		case NODE_RETURN:
			c2 = MakeCode(t->line, RET);
			if (!t->numChildren) {
				c1 = MakeCode(t->line, PUSH, 0, 1, OP_NULL);
				//return c2;
			}
			else {
C1:
				c1 = GenTempCode(t->children[0], locals, maxDepth);
			}
Push2:
			if (c1 && c2) {
				Chain(c1, c2);
				return c1;
			}
			break;
		case NODE_MUL_SIMPLE_ASSIGN:
			maxDepth --;
			c3 = MakeCode(t->line, MUL);

SIMPLE_OP_EQUALS:
			if (t->children[0]->val &&
				(!strcmp(t->children[0]->val, (unsigned char*)"null") ||
				 !strcmp(t->children[0]->val, (unsigned char*)"this"))) {
				CompileError(t, "Invalid Assignment");
				break;
			}
			if (t->children[0]->type == NODE_LOCAL_REFERENCE) {
				TableEntry<int> *entry;
				ScriptValue sv;
				if (!CreateStringValue(sv, t->children[0]->val, (int)t->children[0]->intVal)) {
					CompileError(t, "Memory allocation error");
					break;
				}
				if (VM.Globals.Find(sv.stringVal) && !locals.Find(sv.stringVal)) {
					CompileWarning(t, "Local and global variables share name");
				}
				if (type && type->FindValue(sv.stringVal) >= 0) {
					CompileWarning(t, "Local and class variables share name");
				}
				if (!(entry = locals.Add(sv.stringVal))) break;
				c1 = MakeCode(t->line, PUSH, entry->index, 3, OP_LOCAL);
				c4 = MakeCode(t->line, POP, entry->index, 3, OP_LOCAL);
				c5 = MakeCode(t->line, PUSH, entry->index, 3, OP_LOCAL);
			}
			else if (t->children[0]->type == NODE_GLOBAL_REFERENCE) {
				TableEntry<bool> *entry;
				int i = VM.StringTable.Add(t->children[0]->val, (int)t->children[0]->intVal);
				if (i < 0) {
					CompileError(t, "Memory allocation error");
					break;
				}
				if (!(entry = VM.Globals.Find(VM.StringTable.table[i].stringVal))) {
					if (locals.Find(VM.StringTable.table[i].stringVal)) {
						CompileWarning(t, "Local and global variables share name");
					}
					if (type && type->FindValue(i) >= 0) {
						CompileWarning(t, "Class and global variables share name");
					}
					VM.StringTable.table[i].stringVal->AddRef();
					if (!(entry = VM.Globals.Add(VM.StringTable.table[i].stringVal))) break;
				}
				c1 = MakeCode(t->line, PUSH, entry->index, 3, OP_GLOBAL);
				c4 = MakeCode(t->line, POP, entry->index, 3, OP_GLOBAL);
				c5 = MakeCode(t->line, PUSH, entry->index, 3, OP_GLOBAL);
			}
			else if (t->children[0]->type == NODE_STRUCT_REFERENCE || t->children[0]->type == NODE_STRUCT_THIS_REFERENCE) {
				int depth = maxDepth+3;
				int i;
				if (t->children[0]->type == NODE_STRUCT_REFERENCE) {
					i = VM.StringTable.Add(t->children[0]->children[1]->val, (int)t->children[0]->children[1]->intVal);
					if (i < 0) {
						CompileError(t, "Memory allocation error");
						break;
					}
					c1 = GenTempCode(t->children[0]->children[0], locals, maxDepth);
					c2 = MakeCode(t->line, OBJECT_PEEK, i);
					c4 = c3;
					c3 = GenTempCode(t->children[1], locals, depth);
					c5 = MakeCode(t->line, OBJECT_ASSIGN, i);
				}
				else {
					TableEntry<int> *entry;
					if (!type || !(entry = locals.Find((unsigned char*) "this"))) {
						CompileError(t, "Class variables illegal outside of class definition");
						break;
					}
					i = VM.StringTable.Add((unsigned char*)t->children[0]->val);
					if (type->FindValue(i) < 0) {
						CompileError(t, "Undefined class variable");
						break;
					}
					c1 = MakeCode(t->line, PUSH_THIS, i, 3, OP_OBJECT);
					c2 = GenTempCode(t->children[1], locals, depth);
					c4 = MakeCode(t->line, POP_THIS, i, 3, OP_OBJECT);
					c5 = MakeCode(t->line, PUSH_THIS, i, 3, OP_OBJECT);
				}
				goto Push5;
			}

			c2 = GenTempCode(t->children[1], locals, maxDepth);
			{
Push5:
				if (!c1 || !c2 || !c3 || !c4 || !c5) {
					break;
				}
				Chain(c1, c2);
				Chain(c1, c3);
				Chain(c1, c4);
				Chain(c1, c5);
				return c1;
			}

		case NODE_DIV_SIMPLE_ASSIGN:
			c3 = MakeCode(t->line, DIV);
			goto SIMPLE_OP_EQUALS;

		case NODE_CONCAT_SIMPLE_ASSIGN:
			c3 = MakeCode(t->line, CONCAT_STRING);
			goto SIMPLE_OP_EQUALS;

		case NODE_MOD_SIMPLE_ASSIGN:
			c3 = MakeCode(t->line, MOD);
			goto SIMPLE_OP_EQUALS;

		case NODE_ADD_SIMPLE_ASSIGN:
			c3 = MakeCode(t->line, ADD);
			goto SIMPLE_OP_EQUALS;

		case NODE_SUB_SIMPLE_ASSIGN:
			c3 = MakeCode(t->line, SUB);
			goto SIMPLE_OP_EQUALS;

		case NODE_XOR_SIMPLE_ASSIGN:
			c3 = MakeCode(t->line, XOR);
			goto SIMPLE_OP_EQUALS;

		case NODE_AND_SIMPLE_ASSIGN:
			c3 = MakeCode(t->line, AND);
			goto SIMPLE_OP_EQUALS;

		case NODE_OR_SIMPLE_ASSIGN:
			c3 = MakeCode(t->line, OR);
			goto SIMPLE_OP_EQUALS;

		case NODE_LSHIFT_SIMPLE_ASSIGN:
			c3 = MakeCode(t->line, LSHIFT);
			goto SIMPLE_OP_EQUALS;

		case NODE_RSHIFT_SIMPLE_ASSIGN:
			c3 = MakeCode(t->line, RSHIFT);
			goto SIMPLE_OP_EQUALS;

		case NODE_MUL_ELEMENT_ASSIGN:
			c5 = MakeCode(t->line, MUL);

ELEMENT_OP_EQUALS:
			c1 = GenTempCode(t->children[0], locals, maxDepth);
			if (c1->op == PUSH && c1->param4 < OP_LOCAL) {
				CompileError(t, "Invalid Assignment");
				break;
			}
			c2 = GenTempCode(t->children[1], locals, maxDepth);
			c3 = MakeCode(t->line, ELEMENT_PEEK);
			c4 = GenTempCode(t->children[2], locals, maxDepth);
			{
				TempCode *c6 = MakeCode(t->line, ELEMENT_ASSIGN);
				if (!c1 || !c2 || !c3 || !c4 || !c5 || !c6) {
					CleanupTempCode(c6);
					break;
				}
				Chain(c1, c2);
				Chain(c1, c3);
				Chain(c1, c4);
				Chain(c1, c5);
				Chain(c1, c6);
				return c1;
			}
		case NODE_DIV_ELEMENT_ASSIGN:
			c5 = MakeCode(t->line, DIV);
			goto ELEMENT_OP_EQUALS;

		case NODE_CONCAT_ELEMENT_ASSIGN:
			c5 = MakeCode(t->line, CONCAT_STRING);
			goto ELEMENT_OP_EQUALS;

		case NODE_MOD_ELEMENT_ASSIGN:
			c5 = MakeCode(t->line, MOD);
			goto ELEMENT_OP_EQUALS;

		case NODE_ADD_ELEMENT_ASSIGN:
			c5 = MakeCode(t->line, ADD);
			goto ELEMENT_OP_EQUALS;

		case NODE_SUB_ELEMENT_ASSIGN:
			c5 = MakeCode(t->line, SUB);
			goto ELEMENT_OP_EQUALS;

		case NODE_XOR_ELEMENT_ASSIGN:
			c5 = MakeCode(t->line, XOR);
			goto ELEMENT_OP_EQUALS;

		case NODE_AND_ELEMENT_ASSIGN:
			c5 = MakeCode(t->line, AND);
			goto ELEMENT_OP_EQUALS;

		case NODE_OR_ELEMENT_ASSIGN:
			c5 = MakeCode(t->line, OR);
			goto ELEMENT_OP_EQUALS;

		case NODE_LSHIFT_ELEMENT_ASSIGN:
			c5 = MakeCode(t->line, LSHIFT);
			goto ELEMENT_OP_EQUALS;

		case NODE_RSHIFT_ELEMENT_ASSIGN:
			c5 = MakeCode(t->line, RSHIFT);
			goto ELEMENT_OP_EQUALS;

		case NODE_NOT:
			c2 = MakeCode(t->line, NOT);
			goto C1;
		case NODE_BOOL_OR:
			{
				int jumpid = jump++;
				c1 = GenTempCode(t->children[0], locals, maxDepth);
				c2 = MakeCode(t->line, TEST);
				c3 = MakeCode(t->line, JMP_NE, jumpid, 0);

				c4 = GenTempCode(t->children[1], locals, maxDepth);
				c5 = MakeCode(t->line, TEST);

				TempCode *c6 = MakeCode(t->line, JMPDEST, jumpid);
				TempCode *c7 = MakeCode(t->line, SET_NE);
				if (!c1 || !c2 || !c3 || !c4 || !c5 || !c6 || !c7) {
					CleanupTempCode(c6);
					CleanupTempCode(c7);
					break;
				}
				Chain(c1, c2);
				Chain(c1, c3);
				Chain(c1, c4);
				Chain(c1, c5);
				Chain(c1, c6);
				Chain(c1, c7);
				return c1;
			}
		case NODE_BOOL_AND:
			{
				int jumpid = jump++;
				c1 = GenTempCode(t->children[0], locals, maxDepth);
				c2 = MakeCode(t->line, TEST);
				c3 = MakeCode(t->line, JMP_E, jumpid, 0);
				c4 = GenTempCode(t->children[1], locals, maxDepth);
				c5 = MakeCode(t->line, TEST);
				TempCode *c6 = MakeCode(t->line, JMPDEST, jumpid);
				TempCode *c7 = MakeCode(t->line, SET_NE);
				if (!c1 || !c2 || !c3 || !c4 || !c5 || !c6 || !c7) {
					CleanupTempCode(c6);
					CleanupTempCode(c7);
					break;
				}
				Chain(c1, c2);
				Chain(c1, c3);
				Chain(c1, c4);
				Chain(c1, c5);
				Chain(c1, c6);
				Chain(c1, c7);
				return c1;
			}
		case NODE_EMPTY_DICT:
			return MakeCode(t->line, PUSH_EMPTY_DICT);
		case NODE_DICT_CREATE:
			c3 = MakeCode(t->line, DICT_CREATE);
			goto C1C2;
		case NODE_NEG:
			c2 = MakeCode(t->line, NEG);
			goto C1;
		case NODE_BIN_NEG:
			c1 = GenTempCode(t->children[0], locals, maxDepth);
			c2 = MakeCode(t->line, PUSH, -1, 3, OP_INT);
			c3 = MakeCode(t->line, XOR);
			goto Push3;

		case NODE_SUB_REFERENCE:
			c1 = GenTempCode(t->children[0], locals, maxDepth);
			if (!c1) break;
			{
				TempCode *test = GetLast(c1);
				if (test->op == PUSH && test->param4 < OP_STRING) {
					CompileError(t, "Invalid Reference");
					break;
				}
			}
			c2 = GenTempCode(t->children[1], locals, maxDepth);
			c3 = MakeCode(t->line, PUSH, 0, 0, OP_ELEMENT);
			goto Push3;
			//c1 = GenTempCode(t->children[0], locals, maxDepth);
			//c2 = GenTempCode(t->children[1], locals, maxDepth);
			//goto Push2;
		case NODE_LIST_ADD_NULL:
			c1 = GenTempCode(t->children[0], locals, maxDepth);
			c2 = MakeCode(t->line, PUSH, 0, 1, OP_NULL);
			goto AddToList;
		case NODE_LIST_ADD:
			maxDepth -= 2;
			c1 = GenTempCode(t->children[0], locals, maxDepth);
			c2 = GenTempCode(t->children[1], locals, maxDepth);
AddToList:
			if (c1 && c2) {
				TempCode *t1 = c1;
				TempCode *t2 = 0;
				while (1) {
					t1->last = 0;
					if (!t1->next) break;
					t2 = t1;
					t1 = t1->next;
				}
				if (!t2 || t1->op != MAKE_LIST || t1->intVal > (1<<22)) {
					c3 = MakeCode(t->line, LIST_ADD_ELEMENT);
				}
				else {
					t2->next = 0;
					c3 = t1;
					t1->intVal++;
				}
				goto Push3;
			}
			break;
		case NODE_LIST_CREATE_NULL:
			//c1 = MakeCode(t->line, PUSH_EMPTY_LIST);
			//c1 = MakeCode(t->line, PUSH_NULL);
			c1 = MakeCode(t->line, PUSH, 0, 1, OP_NULL);
			c2 = MakeCode(t->line, MAKE_LIST, 1);
			goto Push2;
		case NODE_LIST_CREATE:
			//c1 = MakeCode(t->line, PUSH_EMPTY_LIST);
			c1 = GenTempCode(t->children[0], locals, maxDepth);
			c2 = MakeCode(t->line, MAKE_LIST, 1);
			goto Push2;
		case NODE_EMPTY_LIST:
			return MakeCode(t->line, MAKE_LIST, 0);
		case NODE_APPEND:
			maxDepth -=2;
			c3 = MakeCode(t->line, APPEND);
			goto C1C2;
		case NODE_SIZE:
			c1 = GenTempCode(t->children[0], locals, maxDepth);
			c2 = MakeCode(t->line, DSIZE);
			goto Push2;
		case NODE_STRING:
			{
				int entry;
				if ((entry = VM.StringTable.Add(t->val, (int)t->intVal))<0) break;
				return MakeCode(t->line, PUSH, entry, 3, OP_STRING);
			}
			break;
		/*case NODE_LOCAL_REFERENCE:
			{
				TableEntry<int> *entry;
				if (!(entry = locals.Add(t->val))) return 0;
				return MakeCode(t->line, LOCAL_REF, entry->index);
			}
		case NODE_GLOBAL_REFERENCE:
			{
				TableEntry<ScriptValue> *entry;
				if (entry = Globals.Find(t->val)) {
					return MakeCode(t->line, GLOBAL_REF, entry->index);
				}
				if (!(entry = Globals.Add(t->val))) return 0;
				Globals.table[entry->index].data.type = -1;
				Globals.table[entry->index].data.intVal = 0;
				return MakeCode(t->line, GLOBAL_REF, entry->index);
			}
			//*/
		case NODE_ELEMENT_ASSIGN:
			c1 = GenTempCode(t->children[0], locals, maxDepth);
			if (!c1) break;
			c2 = GenTempCode(t->children[1], locals, maxDepth);
			c3 = GenTempCode(t->children[2], locals, maxDepth);
			c4 = MakeCode(t->line, ELEMENT_ASSIGN);
			if (c1->op == PUSH && c1->param4 < OP_LOCAL) {
				CompileError(t, "Invalid Assignment");
				break;
			}
Push4:
			if (c1 && c2 && c3 && c4) {
				Chain(c1, c2);
				Chain(c1, c3);
				Chain(c1, c4);
				return c1;
			}
			break;
		case NODE_SIMPLE_ASSIGN:
			{
				//TableEntry<int> *entry;
				//if (!(entry = locals.Add(t->val))) return 0;
				//c1 = GenTempCode(t->children[1], locals, maxDepth);
				//c2 = GenTempCode(t->children[0], locals, maxDepth);
				if (t->children[0]->type == NODE_STRUCT_REFERENCE || t->children[0]->type == NODE_STRUCT_THIS_REFERENCE) {
					int depth = maxDepth+1;
					int i;
					c2 = GenTempCode(t->children[1], locals, depth);
					if (t->children[0]->type == NODE_STRUCT_REFERENCE) {
						i = VM.StringTable.Add(t->children[0]->children[1]->val, (int)t->children[0]->children[1]->intVal);
						if (i < 0) {
							CompileError(t, "Memory allocation error");
							break;
						}
						c1 = GenTempCode(t->children[0]->children[0], locals, maxDepth);
						c3 = MakeCode(t->line, OBJECT_ASSIGN, i);
					}
					else {
						TableEntry<int> *entry;
						if (!type || !(entry = locals.Find((unsigned char*) "this"))) {
							CompileError(t, "Class variables illegal outside of class definition");
							break;
						}
						i = VM.StringTable.Add((unsigned char*)t->children[0]->val);
						if (type->FindValue(i) < 0) {
							CompileError(t, "Undefined class variable");
							break;
						}
						c1 = c2;
						c2 = MakeCode(t->line, POP_THIS, i, 3, OP_OBJECT);
						c3 = MakeCode(t->line, PUSH_THIS, i, 3, OP_OBJECT);
					}
					if (depth > maxDepth) maxDepth = depth;
					goto Push3;
				}
				if (t->children[0]->val &&
					(!strcmp(t->children[0]->val, (unsigned char*)"null") ||
					 !strcmp(t->children[0]->val, (unsigned char*)"this"))) {
					CompileError(t, "Invalid Assignment");
					break;
				}
				if (t->children[0]->type == NODE_LOCAL_REFERENCE) {
					TableEntry<int> *entry;
					ScriptValue sv;
					if (!CreateStringValue(sv, t->children[0]->val, (int)t->children[0]->intVal)) {
						CompileError(t, "Memory allocation error");
						break;
					}
					if (VM.Globals.Find(sv.stringVal) && !locals.Find(sv.stringVal)) {
						CompileWarning(t, "Local and global variables share name");
					}
					if (type && type->FindValue(sv.stringVal) >= 0) {
						CompileWarning(t, "Local and class variables share name");
					}
					if (!(entry = locals.Add(sv.stringVal))) break;
					c2 = MakeCode(t->line, POP, entry->index, 3, OP_LOCAL);
					c3 = MakeCode(t->line, PUSH, entry->index, 3, OP_LOCAL);
				}
				else if (t->children[0]->type == NODE_GLOBAL_REFERENCE) {
					TableEntry<bool> *entry;
					int i = VM.StringTable.Add(t->children[0]->val, (int)t->children[0]->intVal);
					if (i < 0) {
						CompileError(t, "Memory allocation error");
						break;
					}
					if (!(entry = VM.Globals.Find(VM.StringTable.table[i].stringVal))) {
						if (locals.Find(VM.StringTable.table[i].stringVal)) {
							CompileWarning(t, "Local and global variables share name");
						}
						if (type && type->FindValue(i) >= 0) {
							CompileWarning(t, "Class and global variables share name");
						}
						VM.StringTable.table[i].stringVal->AddRef();
						if (!(entry = VM.Globals.Add(VM.StringTable.table[i].stringVal))) break;
					}
					c2 = MakeCode(t->line, POP, entry->index, 3, OP_GLOBAL);
					c3 = MakeCode(t->line, PUSH, entry->index, 3, OP_GLOBAL);
				}
				c1 = GenTempCode(t->children[1], locals, maxDepth);
				if (c1 && c2 && c3) {
					Chain(c1, c2);
					Chain(c1, c3);
					return c1;
				}
			}
			break;
		case NODE_ELEMENT_POST_INC:
		case NODE_ELEMENT_POST_DEC:
			{
				c1 = GenTempCode(t->children[0], locals, maxDepth);
				if (!c1) break;
				TempCode *temp = c1;
				while (temp->next) {
					temp->last = 0;
					temp = temp->next;
				}
				if (c1->op == PUSH && c1->param4 < OP_LOCAL) {
					if (t->type == NODE_ELEMENT_PRE_INC)
						CompileError(t, "Invalid increment");
					else
						CompileError(t, "Invalid decrement");
					break;
				}
				TableEntry<int> *entry;
				entry = locals.Find((unsigned char*)"&");
				if (!entry) {
					ScriptValue sv;
					if (!CreateStringValue(sv, (unsigned char*)"&", 1)) {
						CompileError(t, "Memory allocation error");
						break;
					}
					if (!(entry = locals.Add(sv.stringVal))) {
						sv.Release();
						CompileError(t, "Allocation error");
						break;
					}
				}
				if (t->type == NODE_ELEMENT_POST_INC)
					c5 = MakeCode(t->line, PUSH, 1, 3, OP_INT);
				else
					c5 = MakeCode(t->line, PUSH, -1, 3, OP_INT);
				TempCode *c6 = MakeCode(t->line, ADD);
				if (!c5 || !c6) {
					CleanupTempCode(c6);
					break;
				}
				if (temp->op == PUSH && temp->param4 == OP_LOCAL && temp->intVal == entry->index && temp->prev && temp->prev->prev && temp->prev->prev->prev && temp->prev->op == DISCARD) {
					c2 = temp->prev->prev->prev;
					c2->next = 0;
					Chain(c2, c5);
					Chain(c2, c6);
					Chain(c2, temp->prev->prev);
					return c1;
				}//*/
				c2 = MakeCode(t->line, ELEMENT_PEEK);
				c3 = MakeCode(t->line, POP, entry->index, 3, OP_LOCAL);
				c4 = MakeCode(t->line, PUSH, entry->index, 3, OP_LOCAL);
				TempCode *c7 = MakeCode(t->line, ELEMENT_ASSIGN);
				TempCode *c8 = MakeCode(t->line, DISCARD);
				TempCode *c9 = MakeCode(t->line, PUSH, entry->index, 3, OP_LOCAL);
				if (c2 && c3 && c4 && c7 && c8 && c9) {
					if (temp->op == ELEMENT_ASSIGN) {
						CleanupTempCode(c2);
						c2 = 0;
					}
					temp->prev->next = 0;
					temp->prev = 0;
					CleanupTempCode(temp);
					if (c2)
						Chain(c1, c2);
					Chain(c1, c3);
					Chain(c1, c4);
					Chain(c1, c5);
					Chain(c1, c6);
					Chain(c1, c7);
					Chain(c1, c8);
					Chain(c1, c9);
					return c1;
				}
				CleanupTempCode(c6);
				CleanupTempCode(c7);
				CleanupTempCode(c8);
				CleanupTempCode(c9);
				break;
			}
		case NODE_ELEMENT_PRE_INC:
		case NODE_ELEMENT_PRE_DEC:
			{
				c1 = GenTempCode(t->children[0], locals, maxDepth);
				if (!c1) break;
				if (c1->op == PUSH && c1->param4 < OP_LOCAL) {
					if (t->type == NODE_ELEMENT_PRE_INC)
						CompileError(t, "Invalid increment");
					else
						CompileError(t, "Invalid decrement");
					break;
				}
				if (t->type == NODE_ELEMENT_PRE_INC)
					c3 = MakeCode(t->line, PUSH, 1, 3, OP_INT);
				else
					c3 = MakeCode(t->line, PUSH, -1, 3, OP_INT);
				c4 = MakeCode(t->line, ADD);

				if (!c3 || !c4) break;
				TempCode *temp = c1;
				while (temp->next) temp = temp->next;
				temp->prev->next = temp->prev->last = 0;
				if (temp->op == ELEMENT_ASSIGN) {
					Chain(temp->prev, c3);
					Chain(temp->prev, c4);
					Chain(temp->prev, temp);
					return c1;
				}
				c2 = MakeCode(t->line, ELEMENT_PEEK);
				if (c2) {
					Chain(temp->prev, c2);
					Chain(temp->prev, c3);
					Chain(temp->prev, c4);
					Chain(temp->prev, temp);
					temp->op = ELEMENT_ASSIGN;
					temp->intVal = 0;
					return c1;
				}
			}
			break;
		case NODE_SIMPLE_POST_INC:
		case NODE_SIMPLE_POST_DEC:
			c1 = GenTempCode(t->children[0], locals, maxDepth);
			if (!c1) break;
			if (c1->op == PUSH && (c1->param4 < OP_LOCAL || (!c1->next && c1->param4 == OP_LOCAL && c1->intVal <= 1))) {
				if (t->type == NODE_SIMPLE_POST_INC)
					CompileError(t, "Invalid increment");
				else
					CompileError(t, "Invalid decrement");
				break;
			}
			if (t->type == NODE_SIMPLE_POST_INC)
				c2 = MakeCode(t->line, PUSH, 1, 3, OP_INT);
			else
				c2 = MakeCode(t->line, PUSH, -1, 3, OP_INT);
			c3 = MakeCode(t->line, ADD);
			if (c2 && c3) {
				TempCode *temp = c1;
				while (temp->next) temp = temp->next;
				if (temp->op == POP && (temp->param4 == OP_LOCAL || temp->param4 == OP_GLOBAL)) {
					temp->prev->last = 0;
					temp->prev->next = 0;
					Chain(temp->prev, c2);
					Chain(c2, c3);
					Chain(c2, temp);
					return c1;
				}
				else if (temp->op == DISCARD && temp->prev->op == OBJECT_ASSIGN) {
					temp->prev->prev->last = 0;
					temp->prev->prev->next = 0;
					Chain(temp->prev->prev, c2);
					Chain(c2, c3);
					Chain(c2, temp->prev);
					return c1;
				}
				else if (temp->op == PUSH && temp->param4 == OP_OBJECT) {
					temp->op = OBJECT_PEEK;
					c5 = c3;
					c4 = c2;
					c2 = MakeCode(t->line, SWAP);
					c3 = MakeCode(t->line, OBJECT_PEEK, temp->intVal);
					TempCode *c6 = MakeCode(t->line, OBJECT_ASSIGN, temp->intVal);
					TempCode *c7 = MakeCode(t->line, DISCARD);
					if (!c6 || !c7 || !c2 || !c3) {
						CleanupTempCode(c6);
						CleanupTempCode(c7);
						break;
					}
					Chain(c1, c2);
					Chain(c1, c3);
					Chain(c1, c4);
					Chain(c1, c5);
					Chain(c1, c6);
					Chain(c1, c7);
					return c1;
				}
				else {
					c4 = c3;
					c3 = c2;
					c2 = MakeCode(t->line, temp->op, temp->intVal, temp->param3, temp->param4);
					if (temp->op == PUSH && temp->param4 == OP_GLOBAL) {
						c5 = MakeCode(t->line, POP, temp->intVal, 3, OP_GLOBAL);
					}
					else if (temp->op == PUSH && temp->param4 == OP_LOCAL) {
						c5 = MakeCode(t->line, POP, temp->intVal, 3, OP_LOCAL);
					}
					else break;
					goto Push5;
				}
			}
			break;
		case NODE_SIMPLE_PRE_INC:
		case NODE_SIMPLE_PRE_DEC:
			c1 = GenTempCode(t->children[0], locals, maxDepth);
			if (!c1) break;
			if (c1->op == PUSH && (c1->param4 < OP_LOCAL || (c1->param4 == OP_LOCAL && !c1->next && c1->intVal <= 1))) {
				if (t->type == NODE_SIMPLE_PRE_INC)
					CompileError(t, "Invalid increment");
				else
					CompileError(t, "Invalid decrement");
				break;
			}
			if (t->type == NODE_SIMPLE_PRE_INC)
				c2 = MakeCode(t->line, PUSH, 1, 3, OP_INT);
			else
				c2 = MakeCode(t->line, PUSH, -1, 3, OP_INT);
			c3 = MakeCode(t->line, ADD);
			if (c2 && c3) {
				TempCode *temp = c1;
				while (temp->next) temp = temp->next;
				if (temp->prev && temp->prev->op == POP && (temp->prev->param4 == OP_LOCAL || temp->prev->param4 == OP_GLOBAL)) {
					temp->prev->prev->last = 0;
					temp->prev->prev->next = 0;
					Chain(temp->prev->prev, c2);
					Chain(c2, c3);
					Chain(c2, temp->prev);
					return c1;
				}
				else if (temp->op == OBJECT_ASSIGN) {
					temp->prev->last = 0;
					temp->prev->next = 0;
					Chain(temp->prev, c2);
					Chain(c2, c3);
					Chain(c2, temp);
					return c1;
				}
				else {
					if (temp->op == PUSH && temp->param4 == OP_GLOBAL) {
						c4 = MakeCode(t->line, POP, temp->intVal, 3, OP_GLOBAL);
						c5 = MakeCode(t->line, PUSH, temp->intVal, 3, OP_GLOBAL);
					}
					else if (temp->op == PUSH && temp->param4 == OP_LOCAL) {
						c4 = MakeCode(t->line, POP, temp->intVal, 3, OP_LOCAL);
						c5 = MakeCode(t->line, PUSH, temp->intVal, 3, OP_LOCAL);
					}
					else if (temp->op == PUSH && temp->param4 == OP_OBJECT) {
						c5 = c3;
						c4 = c2;
						c3 = temp;
						TempCode *temp2 = temp;
						temp2->prev->next = 0;
						temp2->prev->last = 0;
						temp2 = temp2->prev;
						while (temp2->prev) {
							temp2->prev->last = temp->prev;
							temp2 = temp2->prev;
						}
						c2 = MakeCode(t->line, DUPE);
						TempCode *c6 = MakeCode(t->line, OBJECT_ASSIGN, temp->intVal);
						if (!c6 || !c2) {
							CleanupTempCode(c6);
							break;
						}
						Chain(c1, c2);
						Chain(c1, c3);
						Chain(c1, c4);
						Chain(c1, c5);
						Chain(c1, c6);
						return c1;
					}
					else break;
					goto Push5;
				}
			}
			break;
		case NODE_DEREFERENCE:
			{
				if (t->children[0]->type == NODE_STRUCT_REFERENCE) {
					c1 = GenTempCode(t->children[0]->children[0], locals, maxDepth);
					int i = VM.StringTable.Add(t->children[0]->children[1]->val, (int)t->children[0]->children[1]->intVal);
					if (i < 0) {
						CompileError(t, "Memory allocation error");
						break;
					}
					c2 = MakeCode(t->line, PUSH, i, 3, OP_OBJECT);
					if (c1 && c2) {
						Chain(c1, c2);
						return c1;
					}
					break;
				}
				if (t->children[0]->type == NODE_STRUCT_THIS_REFERENCE) {
					TableEntry<int> *entry;
					if (!type || !(entry = locals.Find((unsigned char*) "this"))) {
						CompileError(t, "Class variables illegal outside of class definition");
						break;
					}
					int stringID = VM.StringTable.Add((unsigned char*)t->children[0]->val);
					if (type->FindValue(stringID) < 0) {
						CompileError(t, "Undefined class variable");
						break;
					}
					c1 = MakeCode(t->line, PUSH, entry->index, 3, OP_LOCAL);
					c2 = MakeCode(t->line, PUSH, stringID, 3, OP_OBJECT);
					if (c1 && c2) {
						Chain(c1, c2);
						return c1;
					}
					break;
				}
				if (t->children[0]->val &&
					!strcmp(t->children[0]->val, (unsigned char*)"null")) {
					return MakeCode(t->line, PUSH, 0, 1, OP_NULL);
					//return MakeCode(t->line, PUSH_NULL);
				}
				//TableEntry<int> *entry;
				//if (!(entry = locals.Add(t->val))) return 0;
				//c1 = GenTempCode(t->children[1], locals, maxDepth);
				//c2 = GenTempCode(t->children[0], locals, maxDepth);
				if (t->children[0]->type == NODE_LOCAL_REFERENCE) {
					TableEntry<int> *entry;
					if (!(entry = locals.Find(t->children[0]->val))) {
						ScriptValue sv;
						if (!strcmp(t->children[0]->val, (unsigned char*)"this")) {
							CompileError(t, "Illegal variable name");
							break;
						}
						if (!CreateStringValue(sv, t->children[0]->val, (int)t->children[0]->intVal)) {
							CompileError(t, "Memory allocation error");
							break;
						}
						if (!(entry = locals.Add(sv.stringVal))) {
							sv.Release();
							CompileError(t, "Allocation error");
							break;
						}
						if (VM.Globals.Find(sv.stringVal)) {
							CompileWarning(t, "Local and global variables share name");
						}
						if (type && type->FindValue(sv.stringVal)>=0) {
							CompileWarning(t, "Local and class variables share name");
						}
					}
					return MakeCode(t->line, PUSH, entry->index, 3, OP_LOCAL);
				}
				else if (t->children[0]->type == NODE_GLOBAL_REFERENCE) {
					if (!strcmp(t->children[0]->val, (unsigned char*)"this")) {
						CompileError(t, "Illegal global variable name");
						break;
					}
					TableEntry<bool> *entry;
					int i = VM.StringTable.Add(t->children[0]->val, (int)t->children[0]->intVal);
					if (i < 0) {
						CompileError(t, "Memory allocation error");
						break;
					}
					if (!(entry = VM.Globals.Find(VM.StringTable.table[i].stringVal))) {
						if (locals.Find(VM.StringTable.table[i].stringVal)) {
							CompileWarning(t, "Local and global variables share name");
						}
						if (type && type->FindValue(i)>=0) {
							CompileWarning(t, "Class and global variables share name");
						}
						VM.StringTable.table[i].stringVal->AddRef();
						if (!(entry = VM.Globals.Add(VM.StringTable.table[i].stringVal))) break;
					}
					return MakeCode(t->line, PUSH, entry->index, 3, OP_GLOBAL);
				}
			}
			break;
			/*
			c1 = GenTempCode(t->children[0], locals, maxDepth);
			c2 = MakeCode(t->line, DEREF);
			if (c1 && c2) {
				Chain(c1, c2);
				return c1;
			}
			break;//*/
		case NODE_DISCARD_FIRST:
			c1 = GenTempCode(t->children[0], locals, maxDepth);
			c2 = MakeCode(t->line, DISCARD);
			c3 = GenTempCode(t->children[1], locals, maxDepth);
			goto Push3;
		case NODE_CONTINUE:
			if (!continueTo) {
				break;
			}
			else {
				c1 = MakeCode(t->line, JMP, jump);
				c2 = MakeCode(t->line, JMPDEST, jump++);
				if (!c1 || !c2) {
					break;
				}
				continueTo->next = c2;
				c2->prev = continueTo;
				continueTo = c2;
				return c1;
			}
		case NODE_BREAK:
			if (!breakTo) {
				errorLine = t->line;
				CompileError(t, "Break not inside loop");
				break;
			}
			else {
				c1 = MakeCode(t->line, JMP, jump);
				c2 = MakeCode(t->line, JMPDEST, jump++);
				if (!c1 || !c2) {
					break;
				}
				breakTo->next = c2;
				c2->prev = breakTo;
				breakTo = c2;
				return c1;
			}
		case NODE_WHILE:
			{
				c1 = MakeCode(t->line, JMPDEST, jump);
				c4 = MakeCode(t->line, JMP_E, jump+1, 1);
				TempCode *c6 = MakeCode(t->line, JMP, jump);
				TempCode *c7 = MakeCode(t->line, JMPDEST, jump+1);
				jump+=2;
				if (!c1 || !c4 || !c6 || !c7) {
					CleanupTempCode(c6);
					CleanupTempCode(c7);
					break;
				}
				TempCode *t1 = continueTo;
				TempCode *t2 = breakTo;
				continueTo = c1;
				breakTo = c7;

				c2 = GenTempCode(t->children[0], locals, maxDepth);
				c3 = MakeCode(t->line, TEST);
				c5 = GenTempCode(t->children[1], locals, maxDepth);

				continueTo = t1;
				breakTo = t2;

				if (!c2 || !c3 || !c5) {
					CleanupTempCode(c6);
					CleanupTempCode(c7);
					break;
				}
				Chain(c1, c2);
				Chain(c1, c3);
				Chain(c1, c4);
				Chain(c1, c5);
				Chain(c1, c6);
				Chain(c1, c7);
				return c1;
			}

		case NODE_DO_WHILE:
			{
				c1 = MakeCode(t->line, JMPDEST, jump);
				// fake jump node.
				c3 = MakeCode(t->line, JMPDEST, jump+1);
				TempCode *c6 = MakeCode(t->line, JMP_NE, jump, 1);
				jump+=2;
				if (!c1 || !c3 || !c6) {
					CleanupTempCode(c6);
					break;
				}
				TempCode *t1 = continueTo;
				TempCode *t2 = breakTo;
				continueTo = c3;
				breakTo = c6;

				c2 = GenTempCode(t->children[0], locals, maxDepth);
				c4 = GenTempCode(t->children[1], locals, maxDepth);
				c5 = MakeCode(t->line, TEST);

				continueTo = t1;
				breakTo = t2;
				if (!c2 || !c4 || !c5) {
					CleanupTempCode(c6);
					break;
				}
				Chain(c1, c2);
				Chain(c1, c3);
				Chain(c1, c4);
				Chain(c1, c5);
				Chain(c1, c6);
				return c1;
			}
		case NODE_FOR:
			{
				c2 = MakeCode(t->line, JMPDEST, jump);
				c4 = MakeCode(t->line, TEST);
				c5 = MakeCode(t->line, JMP_E, jump+1, 1);
				TempCode *c7 = MakeCode(t->line, JMPDEST, jump+2);
				TempCode *c9 = MakeCode(t->line, JMP, jump);
				TempCode *c10 = MakeCode(t->line, JMPDEST, jump+1);

				jump += 3;
				c1 = GenTempCode(t->children[0], locals, maxDepth);
				c3 = GenTempCode(t->children[1], locals, maxDepth);

				// Fake jump node.
				TempCode *c8 = GenTempCode(t->children[2], locals, maxDepth);
				TempCode *c6 = 0;

				if (c7 && c9) {
					TempCode *t1 = continueTo;
					TempCode *t2 = breakTo;
					continueTo = c7;
					breakTo = c9;

					c6 = GenTempCode(t->children[3], locals, maxDepth);

					continueTo = t1;
					breakTo = t2;
				}
				if (!c1 || !c2 || !c3 || !c4 || !c5 || !c6 || !c7 || !c8 || !c9 || !c10) {
					CleanupTempCode(c6);
					CleanupTempCode(c7);
					CleanupTempCode(c8);
					CleanupTempCode(c9);
					CleanupTempCode(c10);
					break;
				}
				Chain(c1, c2);
				Chain(c1, c3);
				Chain(c1, c4);
				Chain(c1, c5);
				Chain(c1, c6);
				Chain(c1, c7);
				Chain(c1, c8);
				Chain(c1, c9);
				Chain(c1, c10);
				return c1;
			}

		case NODE_IF:
			c2 = MakeCode(t->line, TEST);
			c3 = MakeCode(t->line, JMP_E, jump, 1);
			c5 = MakeCode(t->line, JMPDEST, jump++);

			c1 = GenTempCode(t->children[0], locals, maxDepth);
			c4 = GenTempCode(t->children[1], locals, maxDepth);
			goto Push5;
		case NODE_IF_ELSE:
			{
				c2 = MakeCode(t->line, TEST);
				c3 = MakeCode(t->line, JMP_E, jump, 1);
				c5 = MakeCode(t->line, JMP, jump+1);
				TempCode *c6 = MakeCode(t->line, JMPDEST, jump++);
				TempCode *c8 = MakeCode(t->line, JMPDEST, jump++);

				c1 = GenTempCode(t->children[0], locals, maxDepth);
				c4 = GenTempCode(t->children[1], locals, maxDepth);
				TempCode *c7 = GenTempCode(t->children[2], locals, maxDepth);
				if (!c1 || !c2 || !c3 || !c4 || !c5 || !c6 || !c7 || !c8) {
					CleanupTempCode(c6);
					CleanupTempCode(c7);
					CleanupTempCode(c8);
					break;
				}
				Chain(c1, c2);
				Chain(c1, c3);
				Chain(c1, c4);
				Chain(c1, c5);
				Chain(c1, c6);
				Chain(c1, c7);
				Chain(c1, c8);
				return c1;
			}
		case NODE_STRUCT_FUNCTION_CALL:
			{
				c1 = GenTempCode(t->children[0], locals, maxDepth);
				if (!c1) break;
				int i = VM.StringTable.Add(t->children[1]->val, (int)t->children[1]->intVal);
				if (i == -1) {
					CompileError(t, "Memory allocation error");
					break;
				}
				//if (t->numChildren == 3) {
					c2 = GenTempCode(t->children[2], locals, maxDepth);
				/*}
				else {
					c2 = MakeCode(t->line, PUSH_NULL);
				}//*/
				c3 = MakeCode(t->line, CALL_OBJECT, i);
				goto Push3;
			}

		case NODE_STRUCT_THIS_FUNCTION_CALL:
			{
				if (!type) {
					CompileError(t, "Not a class function");
					break;
				}
				int i = VM.StringTable.Add(t->val, (int)t->intVal);
				if (i == -1) {
					CompileError(t, "Memory allocation error");
					break;
				}
				if (type->FindFunction(i) < 0) {
					StringValue *v = VM.StringTable.table[type->nameID].stringVal;
					int q = 2+(int)t->intVal + v->len;
					unsigned char *temp;
					if (temp = (unsigned char*) malloc(q+1)) {
						sprintf((char*)temp, "%s::%s", (char*)v->value, (char*)t->val);
						TableEntry<Function> *entry;
						int i = VM.StringTable.Add(temp, q);
						if (i == -1) {
							CompileError(t, "Memory allocation error");
							free(temp);
							break;
						}
						if (!VM.Functions.Find(VM.StringTable.table[i].stringVal)) {
							VM.StringTable.table[i].stringVal->AddRef();
							if (!(entry = VM.Functions.Add(VM.StringTable.table[i].stringVal))) {
								CompileError(t, "Memory Allocation error");
								free(temp);
								break;
							}
							else {
								entry->data.type = UNDEFINED;
							}
						}
						free(temp);
					}
				}

				c1 = MakeCode(t->line, PUSH, 1, 3, OP_LOCAL);
				//if (t->numChildren == 2) {
					c2 = GenTempCode(t->children[1], locals, maxDepth);
				/*}
				else {
					c2 = MakeCode(t->line, PUSH_NULL);
				}//*/
				c3 = MakeCode(t->line, CALL_OBJECT, i);
				goto Push3;
			}
		case NODE_REF_CALL:
			{
				int depth = 0;
				c1 = GenTempCode(t->children[0], locals, depth);
				int depth2 = 0;
				c2 = GenTempCode(t->children[1], locals, depth2);
				maxDepth += max(depth-1, depth2);
				c3 = MakeCode(t->line, REF_CALL);
				goto Push3;
			}
		case NODE_STRUCT_REF_CALL:
			{
				int depth = 0;
				c1 = GenTempCode(t->children[0], locals, depth);
				int depth2 = 0;
				c2 = GenTempCode(t->children[1], locals, depth2);
				int depth3 = 0;
				c3 = GenTempCode(t->children[2], locals, depth3);
				maxDepth += max(depth-1, max(depth2, depth3+1));
				c4 = MakeCode(t->line, REF_OBJECT_CALL);
				goto Push4;
			}
		case NODE_FUNCTION_CALL:
			{
				int i = VM.StringTable.Add(t->val, (int)t->intVal);
				if (i == -1) {
					CompileError(t, "Memory allocation error");
					break;
				}
				TableEntry<Function> *entry = VM.Functions.Find(VM.StringTable.table[i].stringVal);
				if (!entry) {
					VM.StringTable.table[i].stringVal->AddRef();
					entry = VM.Functions.Add(VM.StringTable.table[i].stringVal);
					if (!entry)
						break;
					entry->data.offset = -1;
					entry->data.type = UNDEFINED;
				}
				if (entry->data.type != UNDEFINED && entry->data.type != SCRIPT && entry->data.type != ALIAS) {
					if (entry->data.type == DLL_CALL)
						c2 = MakeCode(t->line, CALL_DLL, entry->index);
					else if (entry->data.type != C_directwait)
						c2 = MakeCode(t->line, CALL_C, entry->index);
					else
						c2 = MakeCode(t->line, CALL_WAIT, entry->index);
				}
				else {
					c2 = MakeCode(t->line, CALL, entry->index);
				}
			}
			maxDepth--;
			//if (t->numChildren == 2) {
				c1 = GenTempCode(t->children[1], locals, maxDepth);
			/*}
			else {
				c1 = MakeCode(t->line, PUSH_NULL);
			}//*/
			if (c1 && c2) {
				Chain(c1, c2);
				return c1;
			}
			break;
		case NODE_STATEMENT_LIST:
			{
				c3 = 0;
				while (t->type == NODE_STATEMENT_LIST) {
					int tempDepth = 0;
					c2 = GenTempCode(t->children[1], locals, tempDepth);
					if (!c2) break;
					if (c3) Chain(c2, c3);
					c3 = c2;
					if (tempDepth > maxDepth) maxDepth = tempDepth;
					t = t->children[0];
				}
				if (!c2) break;
				c3 = 0;
				int tempDepth = 0;
				c1 = GenTempCode(t, locals, tempDepth);
				if (c1) {
					Chain(c1, c2);
					//Chain(c1, c3);
					return c1;
				}
			}
			break;
		case NODE_EMPTY:
			return MakeCode(t->line, NOP);
		default:
			break;
	}
	if (errorLine == -1) {
		errorLine = t->line;
		CompileError(t, "Unknown Error");
	}
	CleanupTempCode(c1);
	CleanupTempCode(c2);
	CleanupTempCode(c3);
	CleanupTempCode(c4);
	CleanupTempCode(c5);
	return 0;
}

TempCode *GenTempCodeFxn(ParseTree* t, ObjectType *type);

unsigned char GetType(unsigned char *s, int allowVoid) {
	if (!strcmp(s, (unsigned char*)"int")) {
		return SCRIPT_INT;
	}
	else if (!strcmp(s, (unsigned char*)"double")) {
		return SCRIPT_DOUBLE;
	}
	else if (!strcmp(s, (unsigned char*)"string")) {
		return SCRIPT_STRING;
	}
	else if (!strcmp(s, (unsigned char*)"astring")) {
		return SCRIPT_ASCII_STRING;
	}
	else if (!strcmp(s, (unsigned char*)"wstring")) {
		return SCRIPT_WIDE_STRING;
	}
	else if (allowVoid && !strcmp(s, (unsigned char*)"void")) {
		return SCRIPT_NULL;
	}
	/*
	else if (strcmp(t2->children[0]->val, (unsigned char*)"list")) {
		type = SCRIPT_LIST;
	}
	else if (strcmp(t2->children[0]->val, (unsigned char*)"dict")) {
		type = SCRIPT_DICT;
	}//*/
	return 0;
}

TempCode *GetTempCodeDllFxn(ParseTree *t, ObjectType *type) {
	if (!VM.Functions.Preallocate(10)) return 0;
	TableEntry<Function> *entry, *entry2;
	int added = 0;
	int base = 1;
	int returnType = 0;
	if (t->type == NODE_DLL_FUNCTION_SIMPLE) {
		returnType = GetType(t->children[1]->val, 1);
		if (!returnType) {
			CompileError(t->children[1], "Unrecognized return type");
			return 0;
		}
		base++;
	}
	int i = VM.StringTable.Add(t->children[base]->val, (int)t->children[base]->intVal);
	if (i == -1) {
		CompileError(t, "Memory allocation error");
		return 0;
	}
	if (entry = VM.Functions.Find(VM.StringTable.table[i].stringVal)) {
		if (entry->data.type != UNDEFINED) {
			CompileError(t, "Duplicate function definition");
			return 0;
		}
	}
	else {
		VM.StringTable.table[i].stringVal->AddRef();
		if (!(entry = VM.Functions.Add(VM.StringTable.table[i].stringVal))) {
			CompileError(t, "Memory Allocation error");
			return 0;
		}
		added = 1;
	}
	int len = 0;
	unsigned char *argTypes = 0;
	if (t->numChildren == base + 3) {
		ParseTree *t2 = t->children[t->numChildren-1];
		while (t2 && t2->type == NODE_DLL_ARGS) {
			if (len == 10 && returnType) {
				free(argTypes);
				CompileError(t, "At most 10 arguments allowed for this calling method");
				return 0;
			}
			if (!srealloc(argTypes, (len+1) * sizeof(unsigned char*))) {
				free(argTypes);
				CompileError(t, "Memory Allocation error");
				return 0;
			}
			unsigned char type = GetType(t2->children[0]->val, 0);
			if (!type) {
				free(argTypes);
				CompileError(t, "Unrecognized type");
				return 0;
			}
			argTypes[len] = type;
			len++;
			t2 = t2->children[1];
		}
		if (!srealloc(argTypes, (len+1) * sizeof(unsigned char*))) {
			free(argTypes);
			CompileError(t, "Memory Allocation error");
			return 0;
		}
		argTypes[len] = 0;
	}
	int index = CreateDllFunction(t->children[base+1]->val, t->val, returnType, argTypes);
	free(argTypes);
	if (index < 0) {
		CompileError(t, "Memory allocation failure");
		return 0;
	}

	if (!added) {
		unsigned char *tempstr = (unsigned char*) malloc(sizeof(unsigned char)*((int)t->children[base]->intVal + 10));
		if (!tempstr) {
			CompileError(t, "Memory Allocation error");
			return 0;
		}
		memcpy(tempstr, t->children[base]->val, (int)t->children[base]->intVal);
		memcpy(tempstr + t->children[base]->intVal, " dll", 4);
		int i = VM.StringTable.Add(tempstr, (int)t->children[base]->intVal+4);
		free(tempstr);
		if (i == -1) {
			CompileError(t, "Memory Allocation error");
			return 0;
		}
		VM.StringTable.table[i].stringVal->AddRef();
		if (!(entry2 = VM.Functions.Add(VM.StringTable.table[i].stringVal))) {
			CompileError(t, "Memory Allocation error");
			return 0;
		}

		//entry->data.type = SCRIPT;
		entry2->data.type = DLL_CALL;
		entry2->data.offset = index;
		int index2 = entry->index;
		entry->index = entry2->index;
		entry2->index = index2;

		ParseTree temp[4];
		temp[0].type = NODE_FUNCTION;
		temp[0].numChildren = 2;
		temp[0].val = tempstr;
		temp[0].children[1] = &temp[1];

		temp[1].type = NODE_RETURN;
		temp[1].numChildren = 1;
		temp[1].children[0] = &temp[2];

		temp[2].type = NODE_FUNCTION_CALL;
		temp[2].numChildren = 1;
		temp[2].val = t->children[base]->val;
		temp[2].children[0] = &temp[3];

		temp[3].type = NODE_LOCAL_REFERENCE;
		temp[3].numChildren = 1;
		temp[3].val = (unsigned char*)"";
		TempCode * out = GenTempCodeFxn(temp, type);
		free(tempstr);
		return out;
	}
	else {
		entry->data.type = DLL_CALL;
		entry->data.offset = index;
		return MakeCode(t->line, NOP);
	}
}

TempCode *GenTempCodeFxn(ParseTree* t, ObjectType *type) {
	ParseTree *val = t;
	if (!VM.Functions.Preallocate(10)) return 0;
	if (t->type == NODE_DLL_FUNCTION) {
		val = t->children[1];
	}
	else if (t->type == NODE_DLL_FUNCTION_SIMPLE) {
		val = t->children[2];
	}
	unsigned char *name = val->val;
	if (type) {
		int len = ((int)val->intVal) + VM.StringTable.table[type->nameID].stringVal->len+2;
		name = (unsigned char*) malloc(sizeof(unsigned char) * (len+1));
		if (!name) {
			CompileError(t, "Memory allocation error");
			return 0;
		}
		sprintf((char*)name, "%s::%s", VM.StringTable.table[type->nameID].stringVal->value, val->val);
		free(val->val);
		val->val = name;
		val->intVal = len;
	}
	
	if (t->type == NODE_DLL_FUNCTION || t->type == NODE_DLL_FUNCTION_SIMPLE) {
		return GetTempCodeDllFxn(t, type);
	}
	breakTo = 0;
	continueTo = 0;
	SymbolTable<int> locals = {0,0};
	TableEntry<Function> *entry, *entry2;
	int added = 0;
	int i = VM.StringTable.Add(t->val, (int)t->intVal);
	if (i < 0) {
		CompileError(t, "Memory allocation error");
		return 0;
	}
	if (entry = VM.Functions.Find(VM.StringTable.table[i].stringVal)) {
		if (entry->data.type != UNDEFINED) {
			CompileError(t, "Duplicate function definition");
			return 0;
		}
	}
	else {
		VM.StringTable.table[i].stringVal->AddRef();
		if (!(entry = VM.Functions.Add(VM.StringTable.table[i].stringVal))) {
			CompileError(t, "Memory Allocation error");
			return 0;
		}
		added = 1;
	}
	if (t->type == NODE_FUNCTION_ALIAS) {
		int i = VM.StringTable.Add(t->children[1]->val, (int)t->children[1]->intVal);
		if (i < 0) {
			CompileError(t, "Memory allocation error");
			return 0;
		}
		entry2 = VM.Functions.Find(VM.StringTable.table[i].stringVal);
		if (!entry2) {
			VM.StringTable.table[i].stringVal->AddRef();
			if (!(entry2 = VM.Functions.Add(VM.StringTable.table[i].stringVal))) {
				CompileError(t, "Memory Allocation error");
				return 0;
			}
			entry2->data.offset = -1;
			entry2->data.type = UNDEFINED;
		}
		if (added && entry2->data.type != SCRIPT && entry2->data.type != UNDEFINED && entry2->data.type != ALIAS) {
			entry->data = entry2->data;
			return MakeCode(t->line, NOP);
		}
		if (entry2->data.type == SCRIPT || entry2->data.type == UNDEFINED || entry2->data.type == ALIAS) {
			entry->data.type = ALIAS;
			entry->data.offset = -entry2->index;
			return MakeCode(t->line, NOP);
		}
		ParseTree *w = (ParseTree*) calloc(1, sizeof(ParseTree));
		ParseTree *w2 = (ParseTree*) calloc(1, sizeof(ParseTree));
		ParseTree *w3 = (ParseTree*) calloc(1, sizeof(ParseTree));
		unsigned char *v = (unsigned char*) strdup("");
		if (!w || !w2 || !w3 || !v) {
			free(w);
			free(w2);
			free(w3);
			free(v);
			return 0;
		}
		w->type = NODE_RETURN;
		w->numChildren = 1;
		w->children[0] = t->children[1];
		t->children[1]->numChildren = 2;
		t->children[1]->type = NODE_FUNCTION_CALL;
		t->children[1]->children[1] = w2;
		w2->numChildren = 1;
		w2->type = NODE_DEREFERENCE;
		w2->children[0] = w3;
		w3->numChildren = 1;
		w3->type = NODE_LOCAL_REFERENCE;
		w3->val = v;
		t->type = NODE_FUNCTION;
		t->children[1] = w;
	}
	entry->data.type = SCRIPT;
	entry->data.offset = -1;
	int index = entry->index;
	int body = 1;
	int args = 0;
	TempCode *out = 0;
	ScriptValue sv;
	sv.stringVal = 0;
	char *self = "this";
	// Invalid name...Fills space of this, so I can just use this's existence in the local
	// variable table to check if I'm in a struct or not.
	if (!type) self = "{}";
	if (CreateStringValue(sv, (unsigned char*) "") && locals.Add(sv.stringVal) &&
		CreateStringValue(sv, (unsigned char*) self) && locals.Add(sv.stringVal)) {
		if (t->numChildren == 3) {
			ParseTree* temp = t->children[1];
			ScriptValue sv;
			while (1) {
				if (temp->val &&
					(!strcmp(temp->val, (unsigned char*)"null") ||
					 !strcmp(temp->val, (unsigned char*)"this"))) {
					CompileError(temp, "Invalid parameter name");
					break;
				}
				if (!CreateStringValue(sv, temp->val, (int)temp->intVal)) {
					CompileError(t, "Memory allocation error");
					break;
				}
				if (locals.Find(sv.stringVal)) {
					CompileError(temp, "Duplicate function paramater");
					break;
				}
				if (VM.Globals.Find(sv.stringVal)) {
					CompileWarning(temp, "Local and global variables share name");
				}
				if (type && type->FindValue(sv.stringVal)>=0) {
					CompileWarning(temp, "Local and class variables share name");
				}
				if (!locals.Add(sv.stringVal)) {
					CompileError(t, "Memory allocation error");
					break;
				}
				args++;
				if (temp->numChildren == 1) {
					temp = 0;
					break;
				}
				temp = temp->children[1];
			}
			if (temp) {
				locals.Cleanup();
				return 0;
			}
			/*if (!CreateStringValue(sv, temp->val, (int)temp->intVal) || !locals.Add(sv.stringVal)) {
				CompileError(t, "Memory allocation error");
				locals.Cleanup();
				return 0;
			}
			//*/
			args++;
			body = 2;
		}
		int maxDepth = 0;
		TempCode * temp = GenTempCode(t->children[body], locals, maxDepth);
		if (temp) {
			TempCode *junk = MakeCode(t->line, PUSH, (__int64)0, 1, OP_NULL);
			TempCode *ret = MakeCode(t->line, RET, 0);
			entry = &VM.Functions.table[index];
			out = MakeCode(t->line, FUNCTION64, locals.numValues-args-1, maxDepth+2, entry->index, args);
			//TempCode *resize = MakeCode(t->line, RESIZE_STACK, maxDepth+2);
			if (!ret || !out || !junk) {
				CleanupTempCode(out);
				CleanupTempCode(temp);
				CleanupTempCode(ret);
				CleanupTempCode(junk);
				//CleanupTempCode(resize);
				out = 0;
			}
			else {
				//Chain(out, resize);
				Chain(out, temp);
				if (out->last->op != RET) {
					Chain(out, junk);
					Chain(out, ret);
				}
				else {
					CleanupTempCode(ret);
					CleanupTempCode(junk);
				}
				//TempCode *pos = out;
				//while (pos) {
					//if (pos->op == RET)
					//	pos->intVal = locals.numValues;
				//	pos = pos->next;
				//}
			}
		}
	}
	else {
		CompileError(t, "Memory allocation error");
	}
	TempCode *temp = out;
	while (temp) {
		if ((temp->op == POP || temp->op == PUSH) && temp->param4 == OP_LOCAL) {
			if (temp->intVal >= 2) {
				CreateStringValue(temp->sv, locals.table[temp->intVal].sv);
				temp->sv.AddRef();
			}
		}
		temp = temp->next;
	}
	locals.Cleanup();
	return out;
}

int LoadDll(ParseTree* t) {
#ifdef X64
	if (t->type == NODE_DLL32) return 1;
#else
	if (t->type == NODE_DLL64) return 1;
#endif
	unsigned char *v3 = 0;
	if (t->numChildren == 3) {
		v3 = t->children[2]->val;
	}
	int res = CreateDll(t->val, t->children[1]->val, v3);
	if (res == 0) return 1;
	if (res == 1)
		CompileError(t->children[1], "One name used to refer to two dlls.");
	else
		CompileError(t, "Memory allocation error.");
	return 0;
}


TempCode *RemoveNode(TempCode * t) {
	TempCode *w;
	if (!t->prev) {
		if (!t->next) {
			t = 0;
			return 0;
		}
		w = t;
		t = t->next;
		t->prev = 0;
	}
	else {
		t->prev->next = t->next;
		if (t->next)
			t->next->prev = t->prev;
		w = t;
		t = t->prev;
	}
	w->prev = 0;
	w->next = 0;
	CleanupTempCode(w);
	return t;
}
/*
int DeltaStack(OpCode o, int &needValues) {
	switch(o) {
		case DEREF:
			needValues = 1;
			return 0;
		case SUBREF:
			needValues = 2;
			return -1;
		case ELEMENT_ASSIGN:
			needValues = 2;
			return -2;
		case ELEMENT_PEEK:
			needValues = 2;
			return 1;
		case PUSHL:
		case PUSHG:
		case PUSH_NULL:
		case PUSH_EMPTY_LIST:
		case PUSH_STRING:
		case PUSH_EMPTY_DICT:
		case PUSH_INT64:
		case PUSH_DOUBLE64:
			needValues = 0;
			return 1;
		case POPL:
		case POPG:
			needValues = 1;
			return -1;
		case ADD:
		case SUB:
		case MUL:
		case DIV:
		case MOD:
		case OR:
		case XOR:
		case AND:
		case LSHIFT:
		case RSHIFT:
			needValues = 2;
			return -1;
		case COMPARE_E:
		case COMPARE_G:
		case COMPARE_GE:
		case COMPARE_L:
		case COMPARE_LE:
		case COMPARE_NE:
			needValues = 2;
			return -2;
		case NEG:
		case NOT:
			needValues = 1;
			return 0;
		case CALL_C:
		case CALL:
			needValues = 1;
			return 0;
		case DISCARD:
			needValues = 1;
			return -1;
		case RET:
			needValues = 1;
			return -1;
		case NOP:
			needValues = 0;
			return 0;
		case CONCAT_STRING:
		case APPEND:
			needValues = 2;
			return -1;
		case DSIZE:
			needValues = 1;
			return 0;
		case LIST_ADD_ELEMENT:
			needValues = 2;
			return -1;
		case MAKE_BOOL:
			needValues = 1;
			return 0;
		case DICT_CREATE:
			needValues = 2;
			return -1;
		case JMP:
			needValues = 0;
			return 0;
		case JMPTRUE:
		case JMPFALSE:
			needValues = 1;
			return -1;
		case JMPTRUESKIM:
		case JMPFALSESKIM:
			needValues = 1;
			return 0;
		case JMPDEST:
			needValues = 0;
			return 0;
		case FUNCTION64:
			return 0;
		default:
			exit(0);
	}
}

/*
int CheckNull(TempCode *t, int stack = 0) {
	while (1) {
		int need = 0;
		int delta = DeltaStack(t->op, need);
		if (stack - need < 0) return 0;
	}
}
//*/

int IsSet (OpCode op) {
	return (op >= SET_E && op <= SET_NE);
}

int IsJump (OpCode op) {
	return (op >= JMP_E && op <= JMP_NE) || op == JMP;
}

OpCode Opposite (OpCode op) {
	if (op == SET_E) return SET_NE;
	if (op == SET_NE) return SET_E;
	if (op == SET_L) return SET_GE;
	if (op == SET_GE) return SET_L;
	if (op == SET_LE) return SET_G;
	if (op == SET_G) return SET_LE;

	if (op == JMP_E) return JMP_NE;
	if (op == JMP_NE) return JMP_E;
	if (op == JMP_L) return JMP_GE;
	if (op == JMP_GE) return JMP_L;
	if (op == JMP_LE) return JMP_G;
	if (op == JMP_G) return JMP_LE;
	return op;
}

int Optimize(TempCode *&first, int jumps) {
	TempCode **jumpPos = (TempCode**) malloc(2*jumps * (sizeof(int) + sizeof(TempCode*)));
	int *jump = (int*) (jumpPos + jumps);
	if (!jumpPos) return 0;
	TempCode *t;
	int pass = 0;
	if (optimize & 1) {
		while (1) {
			pass ++;
			t = first;
			int changed = 0;
			while (t) {
				if (t->op == COMPARE && t->prev->op == PUSH && t->prev->param4 == OP_INT) {
					int remove = 0;
					if (t->prev->intVal == 0) {
						remove = 1;
					}
					if (remove) {
						t->op = TEST;
						RemoveNode(t->prev);
						changed = 1;
					}
				}
				if ((t->op == PUSH && t->param4 == OP_INT) && t->next->op == TEST) {
					int test = 0;
					int jump;
					if (t->next->next->op == JMP_E) {
						test = 1;
						jump = !t->intVal;
					}
					else if (t->next->next->op == JMP_NE && t->intVal == 0) {
						test = 1;
						jump = t->intVal != 0;
					}
					if (test) {
						TempCode * g = t->next->next;
						while (1) {
							g = g->next;
							if (!g || g->param1 == t->next->next->param1) break;
						}
						if (g) {
							while (g->op == JMPDEST) g = g->next;
							if (g->op < SET_E || g->op > JMP_NE) {
								g = 0;
							}
						}
						if (!g) {
							g = t->next->next;
							while (1) {
								g = g->next;
								if (g->op != JMPDEST) break;
							}
							if (g->op < SET_E || g->op > JMP_NE) {
								g = 0;
							}
						}
						if (jump) {
							t->next->next->op = JMP;
						}
						else {
							RemoveNode(t->next->next);
						}
						if (!g) {
							RemoveNode(t->next);
							t = RemoveNode(t);
							changed = 1;
							continue;
						}
					}
				}
				if (t->op == PUSH && t->param4 == OP_FLOAT && t->doubleVal && t->next->op == DIV) {
					t->next->op = MUL;
					t->doubleVal = 1/t->doubleVal;
				}
				if (t->op == PUSH && t->next->op == PUSH
					&& (t->param4 == OP_FLOAT || t->param4 == OP_INT || t->param4 == OP_NULL)
					&& (t->next->param4 == OP_FLOAT || t->next->param4 == OP_INT || t->next->param4 == OP_NULL)) {
					__int64 iVal1;
					double dVal1;
					__int64 iVal2;
					double dVal2;
					int type = 0;
					if (t->param4 == OP_FLOAT) {
						dVal1 = t->doubleVal;
						type = 2;
					}
					else {
						iVal1 = t->intVal;
						dVal1 = (double)t->intVal;
					}
					if (t->next->param4 == OP_FLOAT) {
						dVal2 = t->next->doubleVal;
						type = 2;
					}
					else {
						iVal2 = t->next->intVal;
						dVal2 = (double)t->next->intVal;
					}
					int c = 0;
					if (t->next->next->op == ADD) {
						c = 1;
						if (!type) {
							t->param4 = OP_INT;
							t->intVal = iVal1+iVal2;
						}
						else {
							t->param4 = OP_FLOAT;
							t->doubleVal = dVal1+dVal2;
						}
					}
					else if (t->next->next->op == SUB) {
						c = 1;
						if (!type) {
							t->param4 = OP_INT;
							t->intVal = iVal1-iVal2;
						}
						else {
							t->param4 = OP_FLOAT;
							t->doubleVal = dVal1-dVal2;
						}
					}
					else if (t->next->next->op == MUL) {
						c = 1;
						if (!type) {
							t->param4 = OP_INT;
							t->intVal = iVal1*iVal2;
						}
						else {
							t->param4 = OP_FLOAT;
							t->doubleVal = dVal1*dVal2;
						}
					}
					if (c) {
						RemoveNode(t->next);
						RemoveNode(t->next);
						changed = 1;
						continue;
					}
				}
				if (t->op == PUSH && t->next->op == PUSH) {
						if ((t->next->next->op == ADD && t->next->next->next->op == ADD)  ||
							(t->next->next->op == MUL && t->next->next->next->op == MUL)) {
								TempCode *n2 = t->next->next;
								TempCode *n3 = t->next;
								TempCode *n4 = t->next->next->next;
								t->next = n2;
								n2->prev = t;
								n2->next = n3;
								n3->prev = n2;
								n3->next = n4;
								n4->prev = n3;
						}
				}
				if ((t->next && t->next->next && (t->op == ADD && t->next->next->op == ADD) ||
					((t->op == MUL && t->op == DIV) && (t->next->next->op == MUL && t->next->next->op == DIV))) &&
					t->prev->op == PUSH && t->prev->param4 <= OP_FLOAT && t->next->op == PUSH && t->next->param4 <= OP_FLOAT) {
					int type = 0;
					__int64 iVal1;
					double dVal1;
					__int64 iVal2;
					double dVal2;
					if (t->prev->param4 == OP_FLOAT) {
						dVal1 = t->prev->doubleVal;
						type = 2;
					}
					else {
						type = 1;
						iVal1 = t->prev->intVal;
						dVal1 = (double)t->prev->intVal;
					}
					if (type) {
						if (t->prev->param4 == OP_FLOAT) {
							type = 2;
							dVal2 = t->next->doubleVal;
						}
						else {
							iVal2 = t->next->intVal;
							dVal2 = (double)t->next->intVal;
						}
						if (t->op == ADD) {
							if (type == 1) {
								t->prev->intVal = iVal1 + iVal2;
								t->prev->param3 = 3;
								t->prev->param4 = OP_INT;
							}
							else {
								t->prev->doubleVal = dVal1 + dVal2;
								t->prev->param3 = 3;
								t->prev->param4 = OP_FLOAT;
							}
						}
						else {
							if (t->op == MUL) {
								if (t->next->next->op == MUL) {
									if (type == 1) {
										if (log10(fabs((double)iVal2)) + log10(fabs((double)iVal1)) < 62.9) {
											t->prev->intVal = iVal1 * iVal2;
											t->prev->param3 = 3;
											t->prev->param4 = OP_INT;
										}
										else type = 0;
									}
									else {
										t->prev->doubleVal = dVal1 * dVal2;
										t->prev->param3 = 3;
										t->prev->param4 = OP_FLOAT;
									}
								}
								else {
									if (type == 1) {
										if (iVal2 && (iVal1 / iVal2) * iVal2 == iVal1) {
											t->prev->intVal = iVal1 / iVal2;
											t->prev->param3 = 3;
											t->prev->param4 = OP_INT;
										}
										else if (iVal1 && (iVal2 / iVal1) * iVal1 == iVal2) {
											t->prev->intVal = iVal2 / iVal1;
											t->prev->param3 = 3;
											t->prev->param4 = OP_INT;
											t->op = DIV;
										}
										else type = 0;
									}
									else {
										if (dVal2) {
											t->prev->doubleVal = dVal1 / dVal2;
											t->prev->param3 = 3;
											t->prev->param4 = OP_FLOAT;
										}
										else type = 0;
									}
								}
							}
							else {
								if (t->next->next->op == MUL) {
									if (type == 1) {
										if (iVal2 && (iVal1 / iVal2) * iVal2 == iVal1) {
											t->prev->intVal = iVal1 / iVal2;
											t->prev->param3 = 3;
											t->prev->param4 = OP_INT;
										}
										else if (iVal1 && (iVal2 / iVal1) * iVal1 == iVal2) {
											t->prev->intVal = iVal2 / iVal1;
											t->prev->param3 = 3;
											t->prev->param4 = OP_INT;
											t->op = MUL;
										}
										else type = 0;
									}
									else {
										if (dVal2) {
											t->prev->doubleVal = dVal1 / dVal2;
											t->prev->param3 = 3;
											t->prev->param4 = OP_FLOAT;
										}
										else type = 0;
									}
								}
								else {
									if (type == 1) {
										if (iVal2 && (iVal1 / iVal2) * iVal2 == iVal1) {
												t->prev->intVal = iVal1 / iVal2;
												t->prev->param3 = 3;
												t->prev->param4 = OP_INT;
										}
										else if (iVal1 && (iVal2 / iVal1) * iVal1 == iVal2) {
											t->prev->intVal = iVal2 / iVal1;
											t->prev->param3 = 3;
											t->prev->param4 = OP_INT;
											t->op = DIV;
										}
										else type = 0;
									}
									else {
										if (type == 1) {
											if (log10(fabs((double)iVal2)) + log10(fabs((double)iVal1)) < 62.9) {
												t->prev->intVal = iVal1 * iVal2;
												t->prev->param3 = 3;
												t->prev->param4 = OP_INT;
											}
											else type = 0;
										}
										else {
											t->prev->doubleVal = dVal1 * dVal2;
											t->prev->param3 = 3;
											t->prev->param4 = OP_FLOAT;
										}
									}
								}
							}
						}
						if (type) {
							RemoveNode(t->next);
							RemoveNode(t->next);
							changed = 1;
							continue;
						}
					}
				}
				//TempCode *prev = t->prev;
				if (t->op == NOP) {
					if (first == t) {
						first = t = RemoveNode(t);
					}
					else
						t = RemoveNode(t);
					changed = 1;
				}
				else if (t->op == OBJECT_ASSIGN && t->next->op == DISCARD) {
					t->op = POP;
					t->param3 = 3;
					t->param4 = OP_OBJECT;
					RemoveNode(t->next);
					changed = 1;
				}
				// Push loop is probably marginally faster than PSHU-DUPE-PUSH.  Can also optimize
				// a bit more from here, in some cases.
				else if (t->op == DUPE && t->prev->op == PUSH && t->prev->param4 < OP_OBJECT) {
					t->op = PUSH;
					t->param1 = t->prev->param1;
					t->param2 = t->prev->param2;
					t->param3 = t->prev->param3;
					t->param4 = t->prev->param4;
					changed = 1;
				}
				else if (t->op == PUSH && t->param4 == OP_LOCAL && t->intVal == 1 && t->next->op == PUSH && t->next->param4 == OP_OBJECT) {
					t->next->op = PUSH_THIS;
					t = RemoveNode(t);
					changed = 1;
				}
				/*
				else if (t->op == ADD && (t->prev->op == PUSH_INT64 || t->prev->op == PUSH_INT24) && (t->next->op == PUSH_INT64 || t->next->op == PUSH_INT24) && t->next->next->op == ADD) {
					t->prev->op = PUSH_INT64;
					t->prev->intVal += t->next->intVal;
					RemoveNode(t->next);
					RemoveNode(t->next);
				}
				else if (t->op == MUL && (t->prev->op == PUSH_INT64 || t->prev->op == PUSH_INT24) && (t->next->op == PUSH_INT64 || t->next->op == PUSH_INT24) && t->next->next->op == MUL && (!t->prev->intVal || (t->prev->intVal * t->next->intVal) / t->prev->intVal == t->next->intVal)) {
					t->prev->op = PUSH_INT64;
					t->prev->intVal *= t->next->intVal;
					RemoveNode(t->next);
					RemoveNode(t->next);
				}//*/
				// list()
				else if (t->op == CALL_C && t->intVal == 1) {
					t = RemoveNode(t);
					changed = 1;
				}
				else if ((t->op == RET || t->op == JMP) && t->next && t->next->op != FUNCTION64 && t->next->op != JMPDEST) {
					RemoveNode(t->next);
					changed = 1;
				}//*/
				else if (pass > 1 && t->op == CALL_C && t->next->op == DISCARD && t->param4 == 0) {
					t->param4 = 1;
					RemoveNode(t->next);
					changed = 1;
				}
				else if (t->op == DISCARD && t->next->op == RET && t->next->intVal) {
					t = RemoveNode(t);
					changed = 1;
				}
				else if (t->op == PUSH && t->param4 == OP_NULL && t->next->op == RET && t->next->intVal == 0) {
					t->next->intVal = 1;
					TempCode *w = t;
					t = t->prev;
					RemoveNode(w);
					changed = 1;
					while (t->op == JMPDEST) {
						t = t->prev;
					}
					if (t->op == DISCARD) {
						t = RemoveNode(t);
					}
				}
				else if (t->op == CALL_C && t->prev->op == MAKE_LIST && t->param3 == 0 && t->prev->intVal < 255) {
					t->param3 = 1+(int) t->prev->intVal;
					RemoveNode(t->prev);
					changed = 1;
				}
				else if (t->op == DISCARD && (t->prev->op == DUPE || (t->prev->op == PUSH && t->prev->param4 <= OP_GLOBAL) || t->prev->op == PUSH_THIS)) {
						RemoveNode(t->prev);
						t = RemoveNode(t);
						changed = 1;
				}
				/*else if ((t->op == PUSHL || t->op == PUSHG) && t->next->op == t->op && t->next->intVal == t->intVal) {
					TempCode * w = t->next;
					do {
						w->op = DUPE;
						w->intVal = 0;
						w = w->next;
					}
					while (w->op == t->op && w->intVal == t->intVal);
					changed = 1;
				}//*/
				/*else if (t->next && 
					(t->next->op == RET && t->op == RET)) {
						RemoveNode(t->next);
						changed = 1;
				}//*/
				else if (t->next && t->next->op == NEG && t->op == PUSH && (t->param4 == OP_NULL || t->param4 == OP_INT || t->param4 == OP_FLOAT)) {
						if (t->param4 == OP_NULL) {
							t->param4 = OP_INT;
						}
						else if (t->param4 == OP_INT) {
							t->intVal = -t->intVal;
						}
						else {
							t->doubleVal = -t->doubleVal;
						}
						RemoveNode(t->next);
						changed = 1;
				}
				else if (t->op == NEG && t->next->op == ADD) {
					t->op = SUB;
					RemoveNode(t->next);
					changed = 1;
				}
				else if (t->op == NEG && t->next->op == SUB) {
					t->op = ADD;
					RemoveNode(t->next);
					changed = 1;
				}
				/*else if (t->op == PUSH_INT64 && (t->intVal < (1<<24) && t->intVal > (-1<<24))) {
					t->op = PUSH_INT24;
				}//*/
				//*
				else if (IsSet(t->op) && t->next->op == NOT) {
					t->op = Opposite(t->op);
					RemoveNode(t->next);
					changed = 1;
				}
				else if ((t->op == JMP_E || t->op == JMP_L || t->op == JMP_LE) && t->param3 == 1 && t->prev->op == TEST && 
					IsSet(t->prev->prev->op)) {
						t->op = (OpCode) (JMP_E-SET_E + Opposite(t->prev->prev->op));
							 RemoveNode(t->prev);
							 RemoveNode(t->prev);
							 changed = 1;
				}
				else if ((t->op == JMP_NE || t->op == JMP_G || t->op == JMP_GE) && t->param3 == 1 && t->prev->op == TEST && 
					IsSet(t->prev->prev->op)) {
						t->op = (OpCode) (JMP_E-SET_E + t->prev->prev->op);
							 RemoveNode(t->prev);
							 RemoveNode(t->prev);
							 changed = 1;
				}
				else if ((t->op == JMP_E || t->op == JMP_GE || t->op == JMP_G || t->op == JMP_LE || t->op == JMP_L || t->op == JMP_NE) &&
					t->next->op == JMP && t->next->next->op == JMPDEST && t->next->next->intVal == t->intVal) {
						t->op = Opposite(t->op);
						t->intVal = t->next->intVal;
						RemoveNode(t->next);
						RemoveNode(t->next);
						changed = 1;
				}
				else if (!changed && (t->op == TEST || t->op == NTEST) && t->prev->op == NOT) {
					if (t->op == TEST)
						t->op = NTEST;
					else
						t->op = TEST;
					RemoveNode(t->prev);
					changed = 1;
				}
				/*else if (t->op == PUSH_NULL) {
					if (!CheckNull(t)) {
						t = RemoveNode(t);
					}
				}*/
				/*else if (t->op == DISCARD) {
				}*/
				else {
					t = t->next;
				}
			}

			memset(jumpPos, 0, 2*jumps * (sizeof(int) + sizeof(TempCode*)));
			t = first;
			while (t) {
				if (t->op == JMPDEST) {
					jumpPos[t->intVal] = t;
				}
				t = t->next;
			}

			t = first;
			while (t) {
				if (t->op == JMP_E || t->op == JMP_GE || t->op == JMP_G || t->op == JMP_LE || t->op == JMP_L || t->op == JMP_NE || 
					t->op == JMP) {
					TempCode * temp = jumpPos[t->intVal];
					int sig = 0;
					while (1) {
						while (temp->next && temp->next->op == JMPDEST) temp = temp->next;
						if (!temp->next || temp->next->op != JMP) {
							//*
							if (t->op == Opposite(temp->next->op)) {
								sig = 1;
								temp = temp->next;
								t->param3 = temp->param3;
								continue;
							}//*/
							if (t->op != temp->next->op) {
								break;
							}
							t->param3 = temp->next->param3;
						}
						sig = 1;
						temp = jumpPos[temp->next->intVal];
					}
					if (sig) {
						TempCode *j = jumpPos[t->intVal];
						j->prev->next = j->next;
						j->next->prev = j->prev;
						j->prev = temp;
						j->next = temp->next;
						j->next->prev = j;
						temp->next = j;
					}
					jump[t->intVal]++;
				}
				t = t->next;
			}
			for (int i=0; i<jumps; i++) {
				if (jump[i] == 0 && jumpPos[i] && jumpPos[i]->next && jumpPos[i]->prev) {
					RemoveNode(jumpPos[i]);
				}
			}
			if (!changed) break;
		}
	}

	memset(jumpPos, 0, 2*jumps * (sizeof(int) + sizeof(TempCode*)));
	t = first;
	int pos = 0;
	while (t) {
		if ((t->op == PUSH || t->op == POP|| t->op == PUSH_THIS || t->op == POP_THIS)) {
			if (t->op == PUSH_THIS) {
				t->op = PUSH;
				t->param4 = OP_THIS_OBJECT;
			}
			if (t->param4 == OP_ELEMENT) {
				if (t->prev->op == PUSH && t->prev->param4 < OP_ELEMENT && t->prev->param3) {
					t->prev->param4 += t->param4;
					t = t->next;
					RemoveNode(t->prev);
					continue;
				}//*/
			}
			//if (t->param3) {
			else if (t->param4 < OP_ELEMENT) {
				if (t->intVal <= MAX_MINI_VALUE && t->intVal >= MIN_MINI_VALUE) {
					t->param3 = 1;
				}
				else if (t->intVal <= MAX_MID_VALUE && t->intVal >= MIN_MID_VALUE) {
					t->param3 = 2;
				}
			}
			/*}
			if (!t->param3) {
				t=t;
			}//*/
		}
		/*if (t->op == DISCARD && t->next->op == PUSH_NULL) {
			t->op = PUSH_NULL;
			t->intVal = t->next->intVal+1;
			RemoveNode(t->next);
			continue;
		}//*/
		if (t->op == ELEMENT_ASSIGN && t->next->op == DISCARD) {
			t->op = ELEMENT_POP;
			RemoveNode(t->next);
			continue;
		}
		if ((t->op >= ADD && t->op <= DSIZE) || t->op == COMPARE) {
			if (t->prev->op == PUSH && t->prev->param4 != OP_NULL) {
				t->prev->op = t->op;
				t = t->next;
				RemoveNode(t->prev);
				continue;
			}
		}
		if (t->op == JMPDEST) {
			jump[t->intVal] = pos;
		}
		pos += OpSize(t->op, t->param3);
		t = t->next;
	}

	pos = 0;
	t = first;
	while (t) {
		if (t->op == JMP_E || t->op == JMP_GE || t->op == JMP_G || t->op == JMP_LE || t->op == JMP_L || t->op == JMP_NE || 
			t->op == JMP) {
			t->intVal = jump[t->intVal]-pos;
		}
		pos += OpSize(t->op, t->param3);
		t = t->next;
	}
	free(jumpPos);
	return 1;
}

void Optimize(ParseTree *tree) {
	for (int i=0; i<tree->numChildren; i++) {
		if (tree->children[i])
			Optimize(tree->children[i]);
	}
	/*
	if (tree->type == NODE_DIV) {
		tree = tree;
	}//*/
	if (tree->type == NODE_DISCARD_FIRST || tree->type == NODE_DISCARD_LAST) {
		if (tree->type == NODE_DISCARD_LAST) {
			do {
				tree = tree->children[tree->numChildren-1];
			}
			while (tree->type == NODE_DISCARD_FIRST && tree->numChildren>1);
		}
		else
			tree = tree->children[0];
		while (tree->type == NODE_SIMPLE_POST_INC || tree->type == NODE_SIMPLE_POST_DEC ||
				tree->type == NODE_SIMPLE_PRE_INC || tree->type == NODE_SIMPLE_PRE_DEC ||
				tree->type == NODE_ELEMENT_POST_INC || tree->type == NODE_ELEMENT_POST_DEC ||
				tree->type == NODE_ELEMENT_PRE_INC || tree->type == NODE_ELEMENT_PRE_DEC) {
			if (tree->type == NODE_SIMPLE_POST_INC) {
				tree->type = NODE_SIMPLE_PRE_INC;
			}
			else if (tree->type == NODE_SIMPLE_POST_DEC) {
				tree->type = NODE_SIMPLE_PRE_DEC;
			}
			else if (tree->type == NODE_ELEMENT_POST_INC) {
				tree->type = NODE_ELEMENT_PRE_INC;
			}
			else if (tree->type == NODE_ELEMENT_POST_DEC) {
				tree->type = NODE_ELEMENT_PRE_DEC;
			}
			tree = tree->children[0];
		}
	}
	if (tree->numChildren == 1) {
		if (tree->type == NODE_NEG) {
			if (tree->children[0]->type == NODE_INT) {
				tree->intVal = -tree->children[0]->intVal;
				tree->type = NODE_INT;
				CleanupTree(tree->children[0]);
				tree->children[0] = 0;
			}
			else if (tree->children[0]->type == NODE_DOUBLE) {
				tree->doubleVal = -tree->children[0]->doubleVal;
				tree->type = NODE_DOUBLE;
				CleanupTree(tree->children[0]);
				tree->children[0] = 0;
			}
		}
	}
	else if (tree->numChildren == 2 && tree->children[0] && tree->children[1] && (tree->type >= NODE_ADD && tree->type <= NODE_XOR)) {
		if (tree->type == NODE_ADD || tree->type == NODE_XOR || tree->type == NODE_AND || tree->type == NODE_MUL || tree->type == NODE_OR) {
			if (tree->children[1]->type == NODE_INT || tree->children[1]->type == NODE_DOUBLE) {
				if (tree->children[0]->type != NODE_INT && tree->children[0]->type != NODE_DOUBLE) {
					ParseTree *temp = tree->children[0];
					tree->children[0] = tree->children[1];
					tree->children[1] = temp;
				}
			}
		}
		int sign1 = 1;
		int sign2 = 1;
		ParseTree *p1 = tree->children[0];
		ParseTree *p2 = tree->children[1];
		ParseTree *parent1 = tree;
		ParseTree *parent2 = tree;
		if (tree->type == NODE_LSHIFT) {
			tree = tree;
		}
		int change = 0;
		if (tree->type == NODE_SUB) {
			// Remove subtraction if I can.  Do the same much later as well.
			if (tree->children[1]->type == NODE_INT) {
				tree->type = NODE_ADD;
				tree->children[1]->intVal = -tree->children[1]->intVal;
			}
			else if (tree->children[1]->type == NODE_DOUBLE) {
				tree->type = NODE_ADD;
				tree->children[1]->doubleVal = -tree->children[1]->doubleVal;
			}
			else
				sign2 = -1;
		}
		if ((tree->type == NODE_ADD || tree->type == NODE_MUL) && tree->children[1]->type != NODE_INT) {
			if (tree->children[0]->type == NODE_INT || tree->children[0]->type == NODE_DOUBLE) {
				tree->children[0] = p2;
				tree->children[1] = p1;
				p1 = p2;
				p2 = tree->children[1];
			}
		}
		if (tree->type == NODE_ADD || tree->type == NODE_SUB) {
			while (p1) {
				if (p1->type == NODE_ADD) {
					parent1 = p1;
					p1 = p1->children[1];
				}
				else if (p1->type == NODE_SUB) {
					parent1 = p1;
					p1 = p1->children[1];
					sign1 = -sign1;
				}
				else if (p1->type == NODE_INT || p1->type == NODE_DOUBLE) break;
				else {
					return;
				}
			}
			while (p2) {
				if (p2->type == NODE_ADD) {
					parent2 = p2;
					p2 = p2->children[0];
				}
				else if (p2->type == NODE_SUB) {
					parent2 = p2;
					p2 = p2->children[2];
					sign2 = -sign2;
				}
				else if (p2->type == NODE_INT || p2->type == NODE_DOUBLE) break;
				else return;
			}
			change = 1;
		}
		else if (tree->type == NODE_MUL) {
			while (p1) {
				if (p1->type == NODE_MUL) {
					parent1 = p1;
					p1 = p1->children[1];
				}
				else if (p1->type == NODE_INT || p1->type == NODE_DOUBLE) break;
				else return;
			}
			while (p2) {
				if (p2->type == NODE_MUL) {
					parent2 = p2;
					p2 = p2->children[0];
				}
				else if (p2->type == NODE_INT || p2->type == NODE_DOUBLE) break;
				else return;
			}
			change = 1;
		}
		else if (tree->type == NODE_DIV) {
			// Handle (y/z).
			if ((p1->type == NODE_INT || p1->type == NODE_DOUBLE) &&
				(p2->type == NODE_INT || p2->type == NODE_DOUBLE))
				change = 1;
			// Handle x/y/z, when x and/or z is a double.
			else if (p1->type == NODE_DIV &&
				((p2->type == NODE_DOUBLE && (p1->children[1]->type == NODE_DOUBLE || p1->children[1]->type == NODE_INT)) ||
				 (p2->type == NODE_INT && p1->children[1]->type == NODE_DOUBLE))) {
					 double d;
					 if (p1->children[1]->type == NODE_DOUBLE) d = p1->children[1]->doubleVal;
					 else d = (double) p1->children[1]->intVal;
					 if (p2->type == NODE_DOUBLE) d *= p2->doubleVal;
					 else d *= (double) p2->intVal;

					 p2->type = NODE_DOUBLE;
					 p2->doubleVal = d;
					 ParseTree *temp = p1;
					 tree->children[0] = p1->children[0];
					 p1->children[0] = 0;
					 CleanupTree(p1);
			}
			else if (p1->type == NODE_DIV && p1->children[1]->type == NODE_INT && p2->type == NODE_INT) {
				__int64 prod = p1->children[1]->intVal * p2->intVal;
				// Overflow check.
				if (prod/p2->intVal == p1->children[1]->intVal) {
					 p2->intVal = prod;
					 ParseTree *temp = p1;
					 tree->children[0] = p1->children[0];
					 p1->children[0] = 0;
					 CleanupTree(p1);
				}
			}
		}
		else if (tree->type == NODE_LSHIFT || tree->type == NODE_RSHIFT) {
			// Handle (y/z).
			if ((p1->type == NODE_INT || p1->type == NODE_DOUBLE) &&
				(p2->type == NODE_INT || p2->type == NODE_DOUBLE))
				change = 1;
		}
		if (change) {
			union {
				__int64 iVal;
				double dVal;
			} val;
			NodeType type;
			if (p1->type == NODE_INT && p2->type == NODE_INT) {
				if (tree->type == NODE_ADD || tree->type == NODE_SUB)
					val.iVal = p1->intVal * sign1 + p2->intVal * sign2;
				else if (tree->type == NODE_MUL)
					val.iVal = p1->intVal * p2->intVal;
				else if (tree->type == NODE_DIV) {
					if (p2->intVal)
						val.iVal = p1->intVal / p2->intVal;
					else
						val.iVal = 0;
				}
				else if (tree->type == NODE_MOD) {
					if (p2->intVal)
						val.iVal = p1->intVal % p2->intVal;
					else
						val.iVal = 0;
				}
				else if (tree->type == NODE_LSHIFT) {
					val.iVal = p1->intVal << p2->intVal;
				}
				else if (tree->type == NODE_RSHIFT) {
					val.iVal = p1->intVal >> p2->intVal;
				}
				else return;
				type = NODE_INT;
			}
			else {
				if (p1->type == NODE_INT) {
					val.dVal = (double)p1->intVal;
				}
				else {
					val.dVal = p1->doubleVal;
				}
				double d2;
				if (p2->type == NODE_INT) {
					d2 = (double)p2->intVal;
				}
				else {
					d2 = p2->doubleVal * sign2;
				}
				if (tree->type == NODE_ADD || tree->type == NODE_SUB)
					val.dVal = val.dVal * sign1 + d2 * sign2;
				else if (tree->type == NODE_MUL)
					val.dVal = val.dVal * d2 * sign2;
				else if (tree->type == NODE_DIV) {
					val.dVal = val.dVal / d2;
				}
				else return;
				type = NODE_DOUBLE;
			}
			if (parent2 == tree) {
				ParseTree *temp = tree->children[0];
				if (type == NODE_INT)
					p1->intVal = val.iVal*sign1;
				else
					p1->doubleVal = val.dVal*sign1;
				p1->type = type;
				*tree = *temp;
				temp->numChildren = 0;
				CleanupTree(temp);
				CleanupTree(p2);
			}
			else {
				//*
				if (type == NODE_INT)
					p2->intVal = val.iVal*sign2;
				else
					p2->doubleVal = val.dVal*sign2;
				p2->type = type;
				ParseTree *temp = parent1->children[0];
				*parent1 = *temp;
				free(temp);
				CleanupTree(p1);
				//*/
			}
		}
	}
	/*
	if ((optimize&1) && c1 && c2 && c3 && c3->next == 0) {
		int remove = 0;
		TempCode *w1=0, *w2=0
		if (c1->next == 0 && c2->next == 0) {
			remove = 1;
			w1 = c1;
			w2 = c2;
		}
		else {
			if (c1->next) {
				w1 = GetLast(c1);
				if (w1->op == c3->op &&
					(c3->op == MUL || c3->op == ADD
			}
			else {
				w1 = c1;
			}
		}
		if (remove) {
			if ((c1->op == PUSH_INT64 || c1->op == PUSH_NULL) && (c2->op == PUSH_INT64 || c2->op == PUSH_NULL)) {
				__int64 v1=0, v2=0;
				if (c1->op == PUSH_INT64)
					v1 = c1->intVal;
				if (c2->op == PUSH_INT64)
					v2 = c2->intVal;
				switch (c3->op) {
					case ADD:
						v1 += v2;
						break;
					case SUB:
						v1 -= v2;
						break;
					case MUL:
						v1 *= v2;
						break;
					case DIV:
						if (v2)
							v1 /= v2;
						else v1 = 0;
						break;
					case MOD:
						if (v2)
							v1 %= v2;
						else v1 = 0;
						break;
					default:
						remove = 0;
						break;
				}
				if (remove) {
					c1->intVal = v1;
					c1->op = PUSH_INT64;
				}
			}
			else if ((c1->op == PUSH_INT64 || c1->op == PUSH_NULL || c1->op == PUSH_DOUBLE64) && (c2->op == PUSH_INT64 || c2->op == PUSH_NULL || c2->op == PUSH_DOUBLE64)) {
				double v1=0, v2=0;
				if (c1->op == PUSH_INT64)
					v1 = (double) c1->intVal;
				else if (c1->op == PUSH_DOUBLE64)
					v1 = c1->doubleVal;
				if (c2->op == PUSH_INT64)
					v2 = (double)c2->intVal;
				else if (c2->op == PUSH_DOUBLE64)
					v2 = c2->doubleVal;
				switch (c3->op) {
					case ADD:
						v1 += v2;
						break;
					case SUB:
						v1 -= v2;
						break;
					case MUL:
						v1 *= v2;
						break;
					case DIV:
						v1 /= v2;
						break;
					default:
						remove = 0;
						break;
				}
				if (remove) {
					c1->doubleVal = v1;
					c1->op = PUSH_DOUBLE64;
				}
			}
			else remove = 0;
		}
		if (remove==1) {
			CleanupTempCode(c2);
			CleanupTempCode(c3);
			return c1;
		}
		else if (remove == 2) {
		}
	}//*/
}

TempCode *GenTempCode(ParseTree* parseTree) {
	{
		int temp = config.GetConfigInt((unsigned char*)"Script", (unsigned char*)"Optimize", 1);
		optimize = 0;
		if (temp) optimize = 1;
		temp = config.GetConfigInt((unsigned char*)"Script", (unsigned char*)"Aggressively Optimize", 1);
		if (temp) optimize |= 2;
	}
	if (optimize & 1) Optimize(parseTree);
	jump = 0;
	TempCode *out = 0;
	TempCode *last = 0;
	ParseTree *t = parseTree;
	errorLine = -1;
	ParseTree *t2 = 0;
	//ObjectType *type = 0;
	type = 0;
	while (t || t2) {
		TempCode *temp = 0;
		ParseTree *active;
		if (t2) {
			active = t2->children[0];
			t2 = t2->children[1];
			if (active->type == NODE_STRUCT_VALUES) continue;
		}
		else {
			active = t->children[0];
			t = t->children[1];
			type = 0;
		}

		if (active->type != NODE_IGNORE) {
			if (active->type == NODE_DLL32 || active->type == NODE_DLL64) {
				if (LoadDll(active)) continue;
			}
			else if (active->type == NODE_STRUCT) {
				//temp = GenTempCodeFxn(active);
				if (!VM.Functions.Preallocate(10)) return 0;
				TableEntry<Function> *entry, *entry2;
				if ((entry = VM.Functions.Find(active->val)) && entry->data.type != UNDEFINED) {
					CompileError(active, "Already defined");
					CleanupTempCode(out);
					out = 0;
					break;
				}
				type = CreateObjectType(active->val, 1);
				if (!type) {
					CompileError(active, "Type already defined");
					CleanupTempCode(out);
					out = 0;
					break;
				}
				char temps[2066];
				sprintf(temps, "%s::%s", active->val, active->val);
				int initProc = VM.StringTable.Add((unsigned char*)temps);
				if (!entry) VM.StringTable.table[type->nameID].stringVal->AddRef();
				if (initProc == -1 ||
					!(entry || (entry = VM.Functions.Add(VM.StringTable.table[type->nameID].stringVal))) ||
					!(entry2 = VM.Functions.Add(VM.StringTable.table[initProc].stringVal))) {
					CompileError(active, "Allocation error");
					CleanupTempCode(out);
					out = 0;
					break;
				}
				VM.StringTable.table[initProc].stringVal->AddRef();
				entry->data.type = SCRIPT;
				entry2->data.type = UNDEFINED;
				temp = MakeCode(active->line, FUNCTION64, 0, 4, entry->index, 0);
				TempCode *n1, *n3, *n4, *n5, *n6, *n8;//, *n2, *n7
				n1 = MakeCode(active->line, MAKE_OBJECT, type->type);
				//n2 = MakeCode(active->line, POP, 1, 3, OP_LOCAL);
				n3 = MakeCode(active->line, PUSH, 2, 3, OP_LOCAL);
				n4 = MakeCode(active->line, PUSH, 0, 3, OP_LOCAL);
				n5 = MakeCode(active->line, CALL_OBJECT, type->nameID);
				n6 = MakeCode(active->line, DISCARD);
				//n7 = MakeCode(active->line, PUSH, 1, 3, OP_LOCAL);
				n8 = MakeCode(active->line, RET, 0);
				//TempCode *resize = MakeCode(t->line, RESIZE_STACK, maxDepth+2);
				//if (!temp || !n1 || !n2 || !n3 || !n4 || !n5 || !n6 || !n7 || !n8) {
				if (!temp || !n1 || !n3 || !n4 || !n5 || !n6 || !n8) {
					CleanupTempCode(temp);
					CleanupTempCode(n1);
					//CleanupTempCode(n2);
					CleanupTempCode(n3);
					CleanupTempCode(n4);
					CleanupTempCode(n5);
					CleanupTempCode(n6);
					//CleanupTempCode(n7);
					CleanupTempCode(n8);
					CleanupTempCode(out);
					out = 0;
					break;
				}
				Chain(temp, n1);
				//Chain(temp, n2);
				Chain(temp, n3);
				Chain(temp, n4);
				Chain(temp, n5);
				Chain(temp, n6);
				//Chain(temp, n7);
				Chain(temp, n8);

				if (active->numChildren == 3) {
					t2 = active;
					int numParents = 0;
					ParentTypeInfo *parents = 0;
					while (t2->numChildren > 1) {
						t2 = t2->children[1];
						if (!srealloc(parents, sizeof(ParentTypeInfo) * (numParents+1))) {
							free(parents);
							CompileError(active, "Allocation error");
							CleanupTempCode(out);
							CleanupTempCode(temp);
							return 0;
						}
						parents[numParents].name = t2->val;
						parents[numParents].len = (int)t2->intVal;
						parents[numParents].tree = t2;
						numParents ++;
					}
					int i;
					ObjectType **types = (ObjectType**) parents;
					t2 = active;
					for (i = 0; i<numParents; i++) {
						t2 = t2->children[1];
						types[i] = FindObjectType(parents[i].name, parents[i].len);
						int error = 1;
						if (!types[i]) {
							CompileError(t2, "Undefined parent class");
						}
						else if (types[i]->canSubclass == 0) {
							CompileError(t2, "Can't extend class");
							break;
						}
						else if (types[i] == type) {
							CompileError(t2, "Class can't extend itself");
						}
						else {
							int j;
							for (j=0; j<i; j++) {
								if (types[j] == types[i]) {
									CompileError(t2, "Duplicate parent class");
									break;
								}
							}
							if (j == i) {
								error = 0;
							}
							for (j=0; j<types[i]->numValues; j++) {
								StringValue *s = VM.StringTable.table[types[i]->values[j].stringID].stringVal;
								if (!type->AddValue(s->value, s->len)) {
									//CompileWarning(t2, "Class variable defined more than once");
									if (!error)
										CustomCompilerWarning(t2, "Warning, line %i: Duplicate parent class variables merged (%s)\r\n", t2->line, s->value);
								}
							}
							for (j=0; j<types[i]->numFuncts; j++) {
								ObjectTypeFunction *f = &types[i]->functs[j];
								int k = 0;
								ParseTree *t3 = active->children[2];
								StringValue *s = VM.StringTable.table[f->stringID].stringVal;
								while (t3) {
									ParseTree *t4 = t3->children[0];
									t3 = t3->children[1];
									if (t4->type != NODE_STRUCT_VALUES) {
										if (t4->val && t4->intVal == s->len && !strcmp(t4->val, s->value)) {
											k = 1;
											break;
										}
									}
								}
								if (!k) {
									int id = type->FindFunction(f->stringID);
									if (id >= 0) {
										if (type->functs[id].functionID != f->functionID) {
											CompileError(t2, "Extends two classes with identically named functions");
											error = 1;
										}
									}
									else {
										if (!type->AddFunction(f->stringID, f->functionID)) {
											CompileError(t2, "Allocation error");
											error = 1;
										}
									}
								}
							}
						}
						if (error) {
							free(parents);
							CleanupTempCode(out);
							CleanupTempCode(temp);
							return 0;
						}
					}
					free(parents);
					t2 = active->children[2];
				}
				else
					t2 = active->children[1];
				ParseTree *t3 = t2;
				while (t3) {
					ParseTree *t4 = t3->children[0];
					t3 = t3->children[1];
					if (t4->type == NODE_STRUCT_VALUES) {
						while (t4) {
							if (!type->AddValue(t4->val, (int)t4->intVal)) {
								CompileWarning(t4, "Class variable defined more than once");
								//CleanupTempCode(out);
								//CleanupTempCode(temp);
								//return 0;
							};
							t4 = t4->children[1];
						}
					}
				}
			}
			else {
				// NODE_FUNCTION, NODE_DLL32, NODE_DLL64, NODE_ALIAS
				temp = GenTempCodeFxn(active, type);
				if (type && temp) {
					ParseTree *val = active;
					if (active->type == NODE_DLL_FUNCTION) {
						val = active->children[1];
					}
					unsigned char *name = val->val + VM.StringTable.table[type->nameID].stringVal->len+2;
					TableEntry<Function> *entry = VM.Functions.Find(val->val);
					if (!entry || !type->AddFunction(name, ((int)val->intVal) - (VM.StringTable.table[type->nameID].stringVal->len+2), entry->index)) {
						CompileError(active, "Allocation error");
						CleanupTempCode(out);
						return 0;
					}
				}
			}

			if (temp) {
				if (!out) {
					out = last = temp;
				}
				else {
					Chain(last, temp);
				}
				temp = GetLast(temp);
			}
			else {
				CleanupTempCode(out);
				out = 0;
				break;
			}
		}
	}
	if (out) {
		if (!Optimize(out, jump)) {
			CleanupTempCode(out);
			return 0;
		}
	}
	return out;
}
