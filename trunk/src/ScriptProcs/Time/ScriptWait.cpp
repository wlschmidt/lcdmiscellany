//#include <stdlib.h>
#include "ScriptWait.h"
#include "../../global.h"
#include "../../vm.h"
#include "../../unicode.h"
//#include "../../util.h"
//#include "../../stringUtil.h"
#include <time.h>
#include "../../http/httpGetPage.h"
//#include "../../malloc.h"
#include "../../ScriptObjects/ScriptObjects.h"


struct ScriptWait {
	int start;
	int data;
	Stack *stack;
	int id;
	int time;
};

ScriptWait * waits = 0;
int numWaits = 0;

void Sleep(ScriptValue &s, ScriptValue *args) {
	int w;
	if (args[0].intVal < 0) w = 0;
	else if (args[0].intVal > 60000) w = 60000;
	else w = args[0].i32;

	int t = GetTickCount();
	Sleep(w);
	CreateIntValue(s, GetTickCount()-t);
}

static void CALLBACK WaitProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
	int index = (-(int)(idEvent))-1;
	KillTimer(hWnd, idEvent);
	if (index < 0 || index>=numWaits || waits[index].stack == 0) {
		return;
	}
	Stack *stack = waits[index].stack;
	waits[index].stack = 0;
	//__int64 end, freq;
	//QueryPerformanceCounter((LARGE_INTEGER*)&end);
	//QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
	//stack->PushInt((__int64)((end - waits[index].start)/((double)(freq)/1000)));
	stack->PushInt(GetTickCount() - waits[index].start);
	ScriptValue sv;
	int res = VM.RunStack(stack, sv);
	sv.Release();
}

int Wait(ScriptValue *args, Stack *stack) {
	if (!ghWnd) return 0;
	if (args->type == SCRIPT_LIST && args->listVal->numVals > 0) {
		ScriptValue sv;
		CoerceIntNoRelease(args->listVal->vals[0], sv);
		if (sv.intVal >= 10 && sv.intVal <= 3600000) {
			int index = 0;
			while (index < numWaits) {
				if (waits[index].stack == 0) break;
				index++;
			}
			if (index == numWaits) {
				if (!srealloc(waits, sizeof(ScriptWait) * (numWaits+1))) return 0;
				waits[numWaits].stack = 0;
				numWaits ++;
			}
			waits[index].stack = stack;
			waits[index].id = index;
			waits[index].time = sv.i32;
			//QueryPerformanceCounter((LARGE_INTEGER*)&waits[index].start);
			waits[index].start = GetTickCount();
			if (SetTimer(ghWnd, -index-1, sv.i32, WaitProc)) return 1;
			waits[index].stack = 0;
		}
	}
	return 0;
}

void SpawnThread(ScriptValue &s, ScriptValue *args) {
	ScriptValue arg;
	if (args[2].type == SCRIPT_LIST) {
		arg = args[2];
	}
	else {
		CreateNullValue(arg);
	}
	CreateIntValue(s, 0);
	if (args[1].type != SCRIPT_OBJECT) {
		TableEntry<Function> *f = VM.Functions.Find(args[0].stringVal);
		if (f) {
			arg.AddRef();
			s.i32 = RunFunction(f->index, arg);
		}
	}
	else {
		int id = VM.StringTable.Find(args[0].stringVal);
		ObjectType *type = &types[args[1].objectVal->type];
		if (id >= 0 && (id = type->FindFunction(id)) >= 0) {
			args[1].objectVal->AddRef();
			arg.AddRef();
			s.i32 = RunFunction(type->functs[id].functionID, arg, CAN_WAIT, args[1].objectVal);
		}
	}
}

static void ScriptCheckPage(void *s) {
	BufferedHttpSocket *socket = GetBufferedHttpSocket((int)(__int64)s);
	if (socket) {
		int index = 0;
		for (; index<numWaits; index++) {
			if (waits[index].stack && waits[index].data == socket->id) break;
		}
		ScriptValue sv;
		CreateNullValue(sv);
		if (socket->result) {
			if (index < numWaits) {
				CreateStringValue(sv, (unsigned char*) socket->result->data, socket->result->len);
			}
		}
		socket->Cleanup();
		if (index < numWaits) {
			Stack *stack = waits[index].stack;
			waits[index].stack = 0;
			stack->Push(sv);
			int res = VM.RunStack(stack, sv);
			sv.Release();
		}
	}
}

int HttpGetWait(ScriptValue *args, Stack *stack) {
	if (!ghWnd) return 0;
	if (args->type == SCRIPT_LIST && args->listVal->numVals > 0 && args->listVal->vals[0].type == SCRIPT_STRING) {
		int index = 0;
		while (index < numWaits) {
			if (waits[index].stack == 0) break;
			index++;
		}
		if (index == numWaits) {
			if (!srealloc(waits, sizeof(ScriptWait) * (numWaits+1))) return 0;
			waits[numWaits].stack = 0;
			numWaits ++;
		}
		waits[index].stack = stack;
		waits[index].id = index;
		waits[index].time = 0;
		char *userName = 0;
		char *pass = 0;
		BufferedHttpSocket *b = HTTPGet((char*)args->listVal->vals->stringVal->value, ghWnd, WMU_CALL_PROC, (LPARAM) ScriptCheckPage);
		if (b) {
			waits[index].data = b->id;
			return 1;
		}
		waits[index].stack = 0;
	}
	return 0;
}

void CleanupWaits() {
	// Stacks cleaned up by VM.
	free(waits);
	numWaits = 0;
	waits = 0;
}

__forceinline void EatWhiteSpace(unsigned char * &buffer, int &len) {
	while(len && IsWhitespace(*buffer)) {
		len--;
		buffer++;
	}
}

static void EntityStrip(ScriptValue &sv) {
	int inpos = 0, outpos = 0;
	while (inpos < sv.stringVal->len) {
		if (sv.stringVal->value[inpos] != '&') {
			sv.stringVal->value[outpos] = sv.stringVal->value[inpos];
			inpos++;
			outpos++;
		}
		else {
			inpos++;
			int pos = inpos;
			while (inpos < sv.stringVal->len && sv.stringVal->value[inpos] != ';')
				inpos++;
			if (pos == inpos - 2 && substrcmp((char*)&sv.stringVal->value[pos], "lt") == 0)
				sv.stringVal->value[outpos++] = '<';
			else if (pos == inpos - 2 && substrcmp((char*)&sv.stringVal->value[pos], "gt") == 0)
				sv.stringVal->value[outpos++] = '>';
			else if (pos == inpos - 3 && substrcmp((char*)&sv.stringVal->value[pos], "amp") == 0)
				sv.stringVal->value[outpos++] = '&';
			else if (pos == inpos - 4 && substrcmp((char*)&sv.stringVal->value[pos], "apos") == 0)
				sv.stringVal->value[outpos++] = '\'';
			else if (pos == inpos - 4 && substrcmp((char*)&sv.stringVal->value[pos], "quot") == 0)
				sv.stringVal->value[outpos++] = '\"';
			else {
				int happy = 0;
				int tpos = pos;
				if (pos <= inpos-2 && sv.stringVal->value[pos] == '#' && IsNumber(sv.stringVal->value[pos+1])) {
					unsigned int temp = 0;
					pos ++;
					while (pos < inpos && IsNumber(sv.stringVal->value[pos])) {
						temp = temp*10 + sv.stringVal->value[pos] - '0';
						pos++;
					}
					if (pos == inpos) {
						outpos += UTF32toUTF8(&sv.stringVal->value[outpos], &temp, 1);
						happy = 1;
					}
				}
				else if (pos <= inpos-3 && sv.stringVal->value[pos] == '#' && sv.stringVal->value[pos+1] == 'x' || IsHex(sv.stringVal->value[pos+2])) {
					unsigned int temp = 0;
					pos +=2;
					while (pos < inpos && IsHex(sv.stringVal->value[pos])) {
						if (IsNumber(sv.stringVal->value[pos])) {
							temp = temp*16 + sv.stringVal->value[pos] - '0';
						}
						else {
							temp = temp*16 + LCASE(sv.stringVal->value[pos]) - 'a' + 10;
						}
						pos++;
					}
					if (pos == inpos) {
						outpos += UTF32toUTF8(&sv.stringVal->value[outpos], &temp, 1);
						happy = 1;
					}
				}
				if (!happy) {
					inpos = tpos;
					sv.stringVal->value[outpos] = sv.stringVal->value[inpos-1];
					outpos++;
					continue;
				}
			}
			if (inpos < sv.stringVal->len) inpos ++;
		}
	}
	if (outpos != sv.stringVal->len) {
		ResizeStringValue(sv, outpos);
	}
}

static int ParseXMLSub(unsigned char * &buffer, int &len, ScriptValue &s, int flags = 0) {
	ScriptValue sv, sv2;
	while (len) {
		EatWhiteSpace(buffer, len);
		if (buffer[0] == '<') {
			if (len <= 1) return 0;
			if (buffer[1] == '?') {
				len -= 2;
				buffer += 2;
				while (len > 1 && (buffer[0] != '?' || buffer[1] != '>')) {
					len --;
					buffer ++;
				}
				if (len <= 1)
					return 0;
				buffer += 2;
				len -= 2;
			}
			else if (len > 3 && buffer[1] == '!' && buffer[2] == '-' && buffer[3] == '-') {
				len -= 4;
				buffer += 4;
				while (len > 2 && (buffer[0] != '-' || buffer[1] != '-' || buffer[2] != '>')) {
					len--;
					buffer++;
				}
				if (len <= 2)
					return 0;
				buffer += 3;
				len -= 3;
			}
			else if (buffer[1] == '!') {
				len -= 2;
				buffer += 2;
				if (len >6 && buffer[0] == '[' && buffer[1] == 'C' && buffer[2] == 'D' && buffer[3] == 'A' && buffer[4] == 'T' && buffer[5] == 'A' && buffer[6] == '[') {
					unsigned char *pos1 = buffer;
					len -= 7;
					buffer += 7;
					unsigned char *pos2 = buffer;
					while (len > 2 && (buffer[0] != ']' || buffer[1] != ']' || buffer[2] != '>')) {
						len--;
						buffer ++;
					}
					if (len <= 2) {
						buffer += len;
						len = 0;
					}
					sv.intVal = 0;
					sv.type = 1;
					if (!CreateStringValue(sv2, pos2, (int)(buffer-pos2)))
						return 0;
					if (!s.listVal->PushBack(sv)) {
						sv2.Release();
						return 0;
					}
					if (!s.listVal->PushBack(sv2)) {
						s.listVal->numVals --;
						return 0;
					}
					buffer += 3;
					len-=3;
					if (len <= 0)
						return 0;
				}
				else {
					while (len && buffer[0] != '>') {
						len --;
						buffer ++;
					}
					if (len <= 1)
						return 0;
					buffer += 1;
					len -= 1;
				}
			}
			else if (buffer[1] == '/') {
				// Assume it matches.
				len -= 2;
				buffer += 2;
				while (len && buffer[0] != '>') {
					len --;
					buffer ++;
				}
				if (len < 1)
					return 0;
				buffer += 1;
				len -= 1;
				return 2;
			}
			else {
				len --;
				buffer ++;
				EatWhiteSpace(buffer, len);
				if (!len) return 0;
				//if (buffer[0] == '>') return 0;
				unsigned char *pos = buffer;
				while (len && !IsWhitespace(buffer[0]) && buffer[0] != '>' && buffer[0] != '/') {
					len --;
					buffer ++;
				}
				if (!len || pos==buffer)
					return 0;
				if (!CreateListValue(sv2, 2))
					return 0;
				sv.type = -1;
				sv.intVal = 0;
				if (!sv2.listVal->PushBack(sv) || !CreateListValue(sv, 2) || !sv2.listVal->PushBack(sv)) {
					sv2.Release();
					return 0;
				}
				if (!CreateStringValue(sv, pos, (int)(buffer-pos))) {
					sv2.Release();
					return 0;
				}
				if (!s.listVal->PushBack(sv)) {
					sv2.Release();
					return 0;
				}
				if (!s.listVal->PushBack(sv2)) {
					sv.Release();
					s.listVal->numVals --;
					return 0;
				}
				while (1) {
					EatWhiteSpace(buffer, len);
					if (!len)
						return 0;
					if (buffer[0] == '>') {
						buffer ++;
						len--;
						if (flags == 2) return 10;
						int res = ParseXMLSub(buffer, len, s.listVal->vals[s.listVal->numVals-1]);
						if (res != 2) return 0;
						if (flags == 1) return 10;
						break;
					}
					if (len > 1 && buffer[0] == '/' && buffer[1] == '>') {
						buffer +=2;
						len-=2;
						if (flags == 1) return 10;
						break;
					}
					unsigned char *pos = buffer;
					while (len && !IsWhitespace(buffer[0]) && buffer[0] != '>' && buffer[0] != '=' && buffer[0] != '/') {
						len --;
						buffer ++;
					}
					if (!len) return 0;
					unsigned char *pos2 = buffer;
					EatWhiteSpace(buffer, len);
					if (!len || buffer[0] != '=') {
						return 0;
					}
					buffer++;
					len--;
					while (len && IsWhitespace(buffer[0])) {
						len --;
						buffer ++;
					}
					if (!len)
						return 0;
					if (buffer[0] != '\"' && buffer[0] != '\'')
						return 0;
					unsigned char *pos3 = buffer;
					buffer ++;
					len --;
					while (len && buffer[0] != pos3[0]) {
						len --;
						buffer ++;
					}
					if (!len)
						return 0;
					pos3 ++;
					if (!CreateStringValue(sv, pos, (int)(pos2-pos)) || !sv2.listVal->vals[1].listVal->PushBack(sv))
						return 0;
					if (!CreateStringValue(sv, pos3, (int)(buffer-pos3)) || !sv2.listVal->vals[1].listVal->PushBack(sv)) {
						sv2.listVal->vals[--sv2.listVal->numVals].Release();
						return 0;
					}
					EntityStrip(sv2.listVal->vals[1].listVal->vals[sv2.listVal->vals[1].listVal->numVals-1]);
					len --;
					buffer ++;
				}
			}
		}
		else {
			if (flags) return 0;
			unsigned char *pos = buffer;
			while (len && buffer[0] != '<') {
				len --;
				buffer++;
			}
			sv.intVal = 0;
			sv.type = -1;
			if (!s.listVal->PushBack(sv))
				return 0;
			if (!CreateStringValue(sv, pos, (int)(buffer-pos)) || !s.listVal->PushBack(sv)) {
				s.listVal->numVals--;
				return 0;
			}
			EntityStrip(s.listVal->vals[s.listVal->numVals-1]);
			if (!len)
				return 1;
		}
	}
	return 1;
}

void ParseXML(ScriptValue &s, ScriptValue *args) {
	int len = args->stringVal->len;
	unsigned char *buffer = args->stringVal->value;
	int utf16 = 0;
	if (buffer[0] == 0xFF &&
		buffer[1] == 0xFE) {
		utf16 = 1;
	}
	else if (buffer[0] == 0xFE &&
			 buffer[1] == 0xFF) {
		for (int i=2; i < len -1; i+=2) {
			unsigned char c = buffer[0];
			buffer[0] = buffer[1];
			buffer[1] = c;
		}
		utf16 = 1;
	}
	unsigned char *temp = 0;
	if (utf16) {
		len -= 2;
		buffer += 2;
		len /= 2;
		temp = UTF16toUTF8Alloc((wchar_t*)buffer, &len);
		if (!temp) {
			return;
		}
		buffer = temp;
	}
	else {
		if (buffer[0] == 0xEF &&
			buffer[1] == 0xBB &&
			buffer[2] == 0xBF) {
				len -= 3;
				buffer += 3;
		}
	}
	if (CreateListValue(s, 2))
		ParseXMLSub(buffer, len, s);
	if (temp) free(temp);
}

#define XML_FAIL			0
#define XML_EAT_WHITESPACE	1
#define XML_CLOSED			2
#define XML_HAPPY			3

void PartialParseXML(ScriptValue &s, ScriptValue *args) {
	int len = args->stringVal->len;
	unsigned char *buffer = args->stringVal->value;
	unsigned char *start = buffer;
	int flags = args[1].i32;
	if (flags < 0 || flags > 1) {
		CreateNullValue(s);
		return;
	}
	flags ++;
	if (buffer[0] == 0xEF &&
		buffer[1] == 0xBB &&
		buffer[2] == 0xBF) {
			len -= 3;
			buffer += 3;
	}
	if (CreateListValue(s, 3)) {
		s.listVal->PushBack(0);
		s.listVal->PushBack(0);
		s.listVal->PushBack(0);
		int happy = 0;
		int code = XML_FAIL;
		if (CreateListValue(s.listVal->vals[0], 2)) {
			int res = ParseXMLSub(buffer, len, s.listVal->vals[0], flags);
			s.listVal->vals[1].i32 = (buffer == (args->stringVal->value+len));
			if (res) {
				if (res == 1) {
					// Eaten whitespace only.
					code = XML_EAT_WHITESPACE;
					happy = 1;
				}
				else if (res == 10) {
					// Eaten thing happily.
					code = XML_HAPPY;
					happy = 1;
				}
				else if (res == 2) {
					// Eaten thing happily.
					code = XML_CLOSED;
					happy = 1;
				}
				s.listVal->vals[1].i32 = code;
				s.listVal->vals[2].i32 = (int) (buffer - start);
			}
			else if (buffer - start == args->stringVal->len) {
				code = XML_EAT_WHITESPACE;
			}
		}
		if (!happy) {
			s.listVal->vals[0].Release();
			CreateNullValue(s.listVal->vals[0]); 
		}
		s.listVal->vals[1].i32 = code;
	}
}

void FindNextXML(ScriptValue &s, ScriptValue *args) {
	CreateIntValue(s, -1);
	if (args->type == SCRIPT_LIST && args->listVal->numVals > 0 && args->listVal->vals->type == SCRIPT_LIST) {
		int first = 0;
		ListValue *list = args->listVal->vals->listVal;
		ScriptValue sv, sv2;
		if (args->listVal->numVals > 1) {
			if (args->listVal->numVals > 2) {
				CoerceIntNoRelease(args->listVal->vals[2], sv2);
				if (sv2.intVal < 0 || sv2.intVal > (1<<30)) return;
				first = sv2.i32 + (sv2.i32&1);
			}
			args->listVal->vals[1].AddRef();
			CoerceString(args->listVal->vals[1], sv);
		}
		else {
			CreateNullValue(sv);
			CoerceString(sv, sv);
		}
		while (first >= 0 && first < list->numVals) {
			if (list->vals[first].type == -1) {
				if (sv.stringVal->len == 0) {
					CreateIntValue(s, first);
					break;
				}
			}
			else if (list->vals[first].type == SCRIPT_STRING && strcmp(sv.stringVal->value, list->vals[first].stringVal->value) == 0) {
				CreateIntValue(s, first);
				break;
			}
			first += 2;
		}
		sv.Release();
	}
}
