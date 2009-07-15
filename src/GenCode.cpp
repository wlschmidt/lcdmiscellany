#include <stdio.h>
#include <stdlib.h>
#include "GenCode.h"
#include "ScriptObjects/ScriptObjects.h"
#include "TempCode.h"
#include "Script.h"
#include "Config.h"
#include "vm.h"

void OutputSrc(TempCode *pos, FILE *out) {
	int size = pos->param3;
	if (size == 1) {
		size = 18;
	}
	else if (size == 2) {
		size = 32;
	}
	else if (size == 3) {
		size = 64;
	}
	char buffer[30];
	_i64toa(pos->intVal, buffer, 10);
	if (pos->param4 >= OP_ELEMENT) {
		fprintf(out, "element");
		if (!(pos->param4&~OP_ELEMENT)) return;
		fprintf(out, " [");
	}
	if ((7&pos->param4) == OP_NULL) {
		fprintf(out, "  null (%6s)", buffer);
	}
	else if ((7&pos->param4) == OP_INT) {
		fprintf(out, "int:%i (%6s)", size, buffer);
	}
	else if ((7&pos->param4) == OP_FLOAT) {
		fprintf(out, "flt:%i (%6le)", size, pos->doubleVal);
	}
	else if ((7&pos->param4) == OP_STRING) {
		fprintf(out, "str:%i (%6s  \"%s\")", size, buffer, VM.StringTable.table[pos->intVal].stringVal->value);
	}
	else if ((7&pos->param4) == OP_LOCAL) {
		fprintf(out, "loc:%i (%6s", size, buffer);
		if (pos->sv.type != SCRIPT_NULL) {
			fprintf(out, "  $%s", pos->sv.stringVal->value);
		}
		else if (pos->intVal == 1) {
			fprintf(out, "  $this");
		}
		else if (pos->intVal == 0) {
			fprintf(out, "  $");
		}
		fprintf(out, ")");
	}
	else if ((7&pos->param4) == OP_GLOBAL) {
		fprintf(out, "glb:%i (%6s  %s)", size, buffer, VM.Globals.table[pos->intVal].sv->value);
	}
	else if ((7&pos->param4) == OP_OBJECT) {
		fprintf(out, "obj:%i (%6s  %s)", size, buffer, VM.StringTable.table[pos->intVal].stringVal->value);
	}
	else if ((7&pos->param4) == OP_THIS_OBJECT) {
		fprintf(out, "this:%i(%6s  %s)", size, buffer, VM.StringTable.table[pos->intVal].stringVal->value);
	}
	if (pos->param4 >= OP_ELEMENT) {
		fprintf(out, "]");
	}
}

int GenCode (TempCode *tempCode, unsigned char *input, int length, unsigned char *fileName) {
	if (code.codeDwords == 0) {
		// hack.
		code.codeDwords = 1;
		code.code = (Op*)calloc(1,sizeof(Op));
	}
	int test = config.GetConfigInt((unsigned char*)"Script", (unsigned char*) "ScriptDebug", 0);
	FILE *out = 0;
	if (test & 1) {
		// Much longer than MAX_PATH
		wchar_t temp[4098];
		if (GetConfFile(temp, L" ASM.log"))
			out = _wfopen(temp, L"ab");
	}
	int len = 0;
	TempCode * pos = tempCode;
	//int res = 0;
	while (pos) {
		len += OpSize(pos->op, pos->param3);
		pos = pos->next;
	}
	if (out && fileName)
		fprintf(out, "File: %s\r\n\r\n", fileName);
	if (srealloc(code.code, (code.codeDwords+len) * sizeof(Op))) {
		code.codeDwords+=len;
		len = code.codeDwords-len;
		pos = tempCode;
		int sline = -1;
		int spos = 0;
		int function = -1;
		while (pos) {
			if (out && pos->op != JMPDEST) {
				if (function != -1 && pos->op == FUNCTION64) {
					fprintf(out, "%05X     END %s\r\n", len-1, VM.Functions.table[function].sv->value);
					function = -1;
				}
				int lineNumber = pos->lineNumber;
				//if (pos->prev) lineNumber = pos->prev->lineNumber;
				while (spos < length && pos->lineNumber > sline) {
					while (spos < length && input[spos] != '\r' && input[spos] != '\n') {
						fwrite(input+spos, 1, 1, out);
						spos++;
					}
					if (spos >= length) fprintf(out, "\r\n");
					else {
						fprintf(out, "\r\n");
						if (input[spos] == '\r' && input[spos+1] == '\n') spos++;
					}
					spos++;
					sline++;
				}
			}
			if (pos->op == JMPDEST) {
				pos = pos->next;
				continue;
			}
			code.code[len].data = 0;
			code.code[len].op = pos->op;
			/*if (OpSize(pos->op, pos->param3) == 3) {
				((__int64*)&code.code[len+1])[0] = pos->intVal;
			}//*/
			int p;
			switch (pos->op) {
				//case RESIZE_STACK:
				case COMPARE:
				case DSIZE:
				case ADD:
				case SUB:
				case MUL:
				case DIV:
				//case LOCAL_REF:
				case PUSH_THIS:
				case POP_THIS:
				case PUSH:
				case POP:
					code.code[len].src.size = (int)pos->param3;
					code.code[len].src.type = (int)pos->param4;
					if (pos->param3 <= 1) {
						code.code[len].src.value = (int)pos->intVal;
					}
					else if (pos->param3 <= 2) {
						*(int*)&code.code[len+1] = (int)pos->intVal;
					}
					else if (pos->param3 <= 3) {
						*(__int64*)&code.code[len+1] = pos->intVal;
					}
					break;
				//case RET:
				case CALL:
				//case GLOBAL_REF:
				//case PUSH_STRING:
				//case PUSHL:
				//case PUSHG:
				//case POPL:
				//case POPG:
				//case PUSH_INT24:
				case JMP:
				default:
					// Often not needed, but can't hurt.
					code.code[len].data = (int)pos->intVal;
					break;
				case FUNCTION64:
					VM.Functions.table[pos->param3].data.offset = len;
					VM.Functions.table[pos->param3].data.type = SCRIPT;
					code.code[len].data = (int)pos->param4;
					code.code[len+1].high = (short)pos->param1;
					code.code[len+1].low = (short)pos->param2;
					break;
				case CALL_C:
					code.code[len].call.function = (int)pos->intVal;
					code.code[len].call.list = pos->param3;
					code.code[len].call.discard = pos->param4;
					break;
				case CALL_DLL:
					p = (int)pos->intVal;
					code.code[len].data = VM.Functions.table[(int)pos->intVal].data.offset;
					code.code[len].call.list = pos->param3;
					code.code[len].call.discard = pos->param4;
					break;
			}
			if (out) {
				fprintf(out, "%05X     ", len);
				switch(pos->op) {
					case COMPARE_SCI:
						fprintf(out, "cmpsi");
						break;
					case COMPARE_S:
						fprintf(out, "cmps");
						break;
					case SET_E:
						fprintf(out, "sete");
						break;
					case SET_GE:
						fprintf(out, "setge");
						break;
					case SET_G:
						fprintf(out, "setg");
						break;
					case SET_LE:
						fprintf(out, "setle");
						break;
					case SET_L:
						fprintf(out, "setl");
						break;
					case SET_NE:
						fprintf(out, "setne");
						break;
					case JMP_E:
						fprintf(out, "je       %6i (%06X)", code.code[len].data, code.code[len].data+len);
						break;
					case JMP_GE:
						fprintf(out, "jge      %6i (%06X)", code.code[len].data, code.code[len].data+len);
						break;
					case JMP_G:
						fprintf(out, "jg       %6i (%06X)", code.code[len].data, code.code[len].data+len);
						break;
					case JMP_LE:
						fprintf(out, "jle      %6i (%06X)", code.code[len].data, code.code[len].data+len);
						break;
					case JMP_L:
						fprintf(out, "jl       %6i (%06X)", code.code[len].data, code.code[len].data+len);
						break;
					case JMP_NE:
						fprintf(out, "jne      %6i (%06X)", code.code[len].data, code.code[len].data+len);
						break;
					case JMP:
						fprintf(out, "jmp      %6i (%06X)", code.code[len].data, code.code[len].data+len);
						break;
					case CONCAT_STRING:
						fprintf(out, "concat");
						break;
					case OBJECT_ASSIGN:
						fprintf(out, "assignObj%6i (%s)", code.code[len].data, VM.StringTable.table[code.code[len].data].stringVal->value);
						break;
					/*case OBJECT_POP:
						fprintf(out, "popObj   %6i (%s)", code.code[len].data, StringTable.table[code.code[len].data].stringVal->value);
						break;//*/
					/*case OBJECT_PUSH:
						fprintf(out, "pushObj  %6i (%s)", code.code[len].data, StringTable.table[code.code[len].data].stringVal->value);
						break;//*/
					case OBJECT_PEEK:
						fprintf(out, "peekObj  %6i (%s)", code.code[len].data, VM.StringTable.table[code.code[len].data].stringVal->value);
						break;
					case POP:
						fprintf(out, "pop      ");
						OutputSrc(pos, out);
						break;
						/*
					case POPL:
						fprintf(out, "popl     %6i", code.code[len].data);
						break;
					case POPG:
						fprintf(out, "popg     %6i", code.code[len].data, Globals.table[code.code[len].data].sv->value);
						break;
						//*/
					case COMPARE:
						fprintf(out, "cmp      ");
						if (pos->param4) OutputSrc(pos, out);
						break;
					case ADD:
						fprintf(out, "add      ");
						if (pos->param4) OutputSrc(pos, out);
						break;
					case MUL:
						fprintf(out, "mul      ");
						if (pos->param4) OutputSrc(pos, out);
						break;
					case SUB:
						fprintf(out, "sub      ");
						if (pos->param4) OutputSrc(pos, out);
						break;
					case DIV:
						fprintf(out, "div      ");
						if (pos->param4) OutputSrc(pos, out);
						break;
					case DSIZE:
						fprintf(out, "size     ");
						if (pos->param4) OutputSrc(pos, out);
						break;
					case PUSH:
						fprintf(out, "push     ");
						OutputSrc(pos, out);
						break;
					/*case PUSH_THIS:
						fprintf(out, "pushthis ");
						OutputSrc(pos, out);
						break;
						//*/
					case POP_THIS:
						fprintf(out, "popthis  ");
						OutputSrc(pos, out);
						break;
					/*case PUSH_STRING:
						fprintf(out, "pushStr  %6i (\"%s\")", code.code[len].data, StringTable.table[code.code[len].data].stringVal->value);
						break;
					case PUSHL:
						fprintf(out, "pushl    %6i", code.code[len].data);
						break;
					case PUSHG:
						fprintf(out, "pushg    %6i (%s)", code.code[len].data, Globals.table[code.code[len].data].sv->value);
						break;
					case PUSH_DOUBLE64:
						fprintf(out, "pushFlt  %6le", pos->doubleVal);
						break;
					case PUSH_INT64:
						{
							char buffer[30];
							_i64toa(pos->intVal, buffer, 10);
							fprintf(out, "pushI64  %6s", buffer);
							break;
						}
					case PUSH_NULL:
						fprintf(out, "pushNull");
						if (code.code[len].data) {
							if (code.code[len].data > 1) {
								fprintf(out, "s %4i", code.code[len].data);
							}
							fprintf(out, "\r\n%05X     discard", len);
						}
						break;
						//*/
					case DUPE:
						fprintf(out, "dupe");
						break;
					case SWAP:
						fprintf(out, "swap");
						break;
					case DISCARD:
						fprintf(out, "discard");
						break;
					//case SIMPLE_ASSIGN:
					//	fprintf(out, "assign_simple\r\n");
					//	break;
					case LIST_ADD_ELEMENT:
						fprintf(out, "listAdd");
						break;
					case PUSH_EMPTY_DICT:
						fprintf(out, "push_dict");
						break;
					case DICT_CREATE:
						fprintf(out, "dictPair");
						break;
					/*case DICT_TO_LIST:
						fprintf(out, "dictToList");
						break;
					//case DEREF:
					//	fprintf(out, "deref\r\n");
					//	break;
					//case LOCAL_REF:
					//	fprintf(out, "push_lrf %6i\r\n", code.code[len].data);
					//	break;
/*					case SWAP:
						fprintf(out, "swap");
						break;
					case SUBREF:
						fprintf(out, "sub_ref");
						break;
						//*/
					case APPEND:
						fprintf(out, "append");
						break;
					case OR:
						fprintf(out, "or");
						break;
					case AND:
						fprintf(out, "and");
						break;
					case LSHIFT:
						fprintf(out, "lshift");
						break;
					case RSHIFT:
						fprintf(out, "rshift");
						break;
					case XOR:
						fprintf(out, "xor");
						break;
					case NOT:
						fprintf(out, "not");
						break;
					case NEG:
						fprintf(out, "neg");
						break;
					case MOD:
						fprintf(out, "mod");
						break;
					case MAKE_LIST:
						fprintf(out, "makeLst  %6i", code.code[len].data);
						break;
					case CALL_DLL:
						fprintf(out, "callDll  %6i (%s)", code.code[len].data, VM.Functions.table[p].sv->value);
						break;
					case REF_CALL:
						fprintf(out, "refCall");
						break;
					case REF_OBJECT_CALL:
						fprintf(out, "refCallObj");
						break;
					case CALL:
						fprintf(out, "call     %6i (%s)", code.code[len].data, VM.Functions.table[code.code[len].data].sv->value);
						break;
					case CALL_C:
						if (code.code[len].call.list) {
							fprintf(out, "makeLst  %6i\r\n%05X     ", code.code[len].call.list-1, len);
						}
						fprintf(out, "callNat  %6i (%s)", code.code[len].call.function, VM.Functions.table[code.code[len].call.function].sv->value);
						if (code.code[len].call.discard) {
							fprintf(out, "\r\n%05X     discard", len);
						}
						break;
					case CALL_WAIT:
						fprintf(out, "callWait %6i (%s)", code.code[len].data, VM.Functions.table[code.code[len].call.function].sv->value);
						break;
					case TEST:
						fprintf(out, "test");
						break;
					case NTEST:
						fprintf(out, "ntest");
						break;
					case NOP:
						fprintf(out, "nop");
						break;
					case RET:
						fprintf(out, "ret");
						if (code.code[len].data) {
							fprintf(out, "      null");
						}
						break;
					case MAKE_COPY:
						fprintf(out, "mkcpy");
						break;
					case FUNCTION64:
						{
							for (int i=0; i<VM.Functions.numValues; i++) {
								if (VM.Functions.table[i].data.offset == len && VM.Functions.table[i].data.type == SCRIPT) {
									function = i;
									fprintf(out, "BEGIN %s\r\n", VM.Functions.table[i].sv->value);
									fprintf(out, "%05X     ", len);
									break;
								}
							}
						}
						fprintf(out, "function %6i, %6i, %6i", code.code[len].data, pos->param1, pos->param2);
						break;
					case ELEMENT_ASSIGN:
						fprintf(out, "assignElem");
						break;
					/*case ELEMENT_PUSH:
						fprintf(out, "pushElem");
						break;//*/
					case ELEMENT_POP:
						fprintf(out, "popElem");
						break;
					/*case ELEMENT_POKE:
						fprintf(out, "poke_elt");
						break;//*/
					case ELEMENT_PEEK:
						fprintf(out, "peekElem");
						break;
					case MAKE_OBJECT:
						fprintf(out, "makeObj  %6i (%s)", (int) pos->intVal, VM.StringTable.table[types[(int)pos->intVal].nameID].stringVal->value);
						break;
					case CALL_OBJECT:
						fprintf(out, "callObj  %6i (\"%s\")", (int) pos->intVal, VM.StringTable.table[(int)pos->intVal].stringVal->value);
						break;
					/*case PUSH_INT24:
						fprintf(out, "pushI24  %6i", (int) pos->intVal);
						break;//*/
					default:
						pos=pos;
						break;
				}
				fprintf(out, "\r\n");
			}
			len += OpSize(pos->op, pos->param3);
			pos = pos->next;
		}
		if (out) {
			if (function != -1) {
				fprintf(out, "%05X     END %s\r\n", len-1, VM.Functions.table[function].sv->value);
				function = -1;
			}
			while (spos < length) {
				while (spos < length && input[spos] != '\r' && input[spos] != '\n') {
					fwrite(input+spos, 1, 1, out);
					spos++;
				}
				if (spos >= length) fprintf(out, "\r\n");
				else {
					fprintf(out, "\r\n");
					if (input[spos] == '\r' && input[spos+1] == '\n') spos++;
				}
				spos++;
				sline++;
			}
			fprintf(out, "\r\n\r\n\r\n");
		}
	}
	int changed = 1;
	while (changed) {
		changed = 0;
		for (int i=0; i<VM.Functions.numValues; i++) {
			if (VM.Functions.table[i].data.type == ALIAS &&
				VM.Functions.table[i].data.offset < 0) {
					VM.Functions.table[i].data = VM.Functions.table[-VM.Functions.table[i].data.offset].data;
					changed = 1;
			}
		}
	}
	if (out) fclose(out);
	CleanupTempCode(tempCode);
	return 1;
}
