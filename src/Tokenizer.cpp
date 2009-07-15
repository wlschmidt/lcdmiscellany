#define _USE_MATH_DEFINES
#include "Tokenizer.h"
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include "stringUtil.h"
#include "malloc.h"
#include "unicode.h"
#include "config.h"
#include "script.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void LexDefError(char *def) {
	errorPrintf(2, "Lex error (in definition %s)\r\n", def);
	errors++;
}

void LexError(int line, unsigned char *lineStart, unsigned char *pos, char *message) {
	errorPrintf(2, "Lex error, line %i: %s\r\n", line, message);
	DisplayErrorLine(lineStart, pos);
	errors++;
}

void LexWarning(int line, unsigned char *lineStart, unsigned char *pos, char *message) {
	errorPrintf(2, "Lex warning, line %i: %s\r\n", line, message);
	DisplayErrorLine(lineStart, pos);
	warnings++;
}

void CleanupTokens(Token *tokens, int numTokens) {
	for (int i=0; i<numTokens; i++) {
		if (tokens[i].val) free(tokens[i].val);
	}
	free(tokens);
}

struct ReservedString {
	char * val;
	ScriptTokenType token;
};

SymbolTable<unsigned char *> defines;

int ReadToken(unsigned char *&string, Token * &tokens, int &numTokens, int &maxTokens, int &comment, unsigned char *end, int &line, unsigned char* &lineStart, int depth = 0) {
	if (comment) {
		if (string[0] == '*' && string[1] == '/') {
			string +=2;
			comment = 0;
			return 0;
		}
		string++;
		return 0;
	}
	const static ReservedString reserved[] = {
		/*{"raw", TOKEN_RAW},
		{"int", TOKEN_INT_TYPE},
		{"double", TOKEN_DOUBLE_TYPE},
		{"float", TOKEN_FLOAT_TYPE},
		{"string", TOKEN_STRING_TYPE},
		{"char", TOKEN_CHAR_TYPE},
		//*/
		{"if", TOKEN_IF},
		{"do", TOKEN_DO},
		{"for", TOKEN_FOR},
		{"var", TOKEN_VAR},
		{"else", TOKEN_ELSE},
		{"size", TOKEN_SIZE},
		{"dict", TOKEN_DICT},
		{"dll", TOKEN_DLL},
		{"dll32", TOKEN_DLL32},
		{"dll64", TOKEN_DLL64},
		{"break", TOKEN_BREAK},
		{"while", TOKEN_WHILE},
		{"return", TOKEN_RETURN},
		{"continue", TOKEN_CONTINUE},
		{"function", TOKEN_FUNCTION},
		{"alias", TOKEN_ALIAS},
		{"struct", TOKEN_STRUCT},
		{"extends", TOKEN_EXTENDS},
		{"call", TOKEN_CALL},
	};
	if (maxTokens <= numTokens + 30) {
		if (!srealloc(tokens, sizeof(Token)*(numTokens*2+30))) {
			LexError(line, lineStart, string, "Memory allocation error");
			return 1;
		}
		memset(tokens + maxTokens, 2*numTokens + 30 - maxTokens, 0);
		maxTokens = 2*numTokens + 30;
	}
	tokens[numTokens].line = line;
	tokens[numTokens].pos = string;
	tokens[numTokens].start = lineStart;
	switch (string[0]) {
		case ' ':case '\t':case '\r':case '\n':
			break;
		case '+':
			if (string[1] == '=') {
				tokens[numTokens++].type = TOKEN_ADDE;
				string++;
			}
			else if (string[1] == '+') {
				tokens[numTokens++].type = TOKEN_INC;
				string++;
			}
			else {
				tokens[numTokens++].type = TOKEN_ADD;
			}
			break;
		case '^':
			if (string[1] == '=') {
				tokens[numTokens++].type = TOKEN_XORE;
				string++;
			}
			else tokens[numTokens++].type = TOKEN_XOR;
			break;
		case ':':
			if (string[1] == ':') {
				string++;
				if (!numTokens || tokens[numTokens-1].type != TOKEN_DOUBLE_COLON)
					tokens[numTokens++].type = TOKEN_DOUBLE_COLON;
			}
			else
				tokens[numTokens++].type = TOKEN_COLON;
			break;
		case '&':
			if (string[1] == '=') {
				tokens[numTokens++].type = TOKEN_ANDE;
				string++;
			}
			else if (string[1] == '&') {
				tokens[numTokens++].type = TOKEN_BOOL_AND;
				string++;
			}
			else {
				tokens[numTokens++].type = TOKEN_AND;
			}
			break;
		case '|':
			if (string[1] == '=') {
				tokens[numTokens++].type = TOKEN_ORE;
				string++;
			}
			else if (string[1] == '|') {
				tokens[numTokens++].type = TOKEN_BOOL_OR;
				string++;
			}
			else {
				tokens[numTokens++].type = TOKEN_OR;
			}
			break;
		case '-':
			if (string[1] == '=') {
				tokens[numTokens++].type = TOKEN_SUBE;
				string++;
			}
			else if (string[1] == '-') {
				tokens[numTokens++].type = TOKEN_DEC;
				string++;
			}
			else {
				tokens[numTokens++].type = TOKEN_SUB;
			}
			break;
		case '*':
			if (string[1] == '=') {
				tokens[numTokens++].type = TOKEN_MULE;
				string++;
			}
			else tokens[numTokens++].type = TOKEN_MUL;
			break;
		case '%':
			if (string[1] == '=') {
				tokens[numTokens++].type = TOKEN_MODE;
				string++;
			}
			else tokens[numTokens++].type = TOKEN_MOD;
			break;
		case '/':
			if (string[1] == '*') {
				comment = 1;
				break;
			}
			if (string[1] == '=') {
				tokens[numTokens++].type = TOKEN_DIVE;
				string++;
			}
			else if (string[1] == '/') {
				while (string < end && string[0] != '\r' && string[0] !='\n')
					string++;
				string--;
			}
			else 
				tokens[numTokens++].type = TOKEN_DIV;
			break;
		case ';':
			tokens[numTokens++].type = TOKEN_END_STATEMENT;
			break;
		case ',':
			tokens[numTokens++].type = TOKEN_COMMA;
			break;
		case '@':
			tokens[numTokens++].type = TOKEN_APPEND;
			break;
		case '.':
			tokens[numTokens++].type = TOKEN_PERIOD;
			break;
		case '$':
			tokens[numTokens++].type = TOKEN_DOLLAR;
			break;
		case '~':
			tokens[numTokens++].type = TOKEN_TILDE;
			break;
		case '=':
			if (string[1] == '=') {
				tokens[numTokens++].type = TOKEN_E;
				string++;
			}
			else
				tokens[numTokens++].type = TOKEN_ASSIGN;
			break;
		case '!':
			if (string[1] == '=') {
				tokens[numTokens++].type = TOKEN_NE;
				string++;
			}
			else
				tokens[numTokens++].type = TOKEN_NOT;
			break;
		case '<':
			if (string[1] == '=') {
				tokens[numTokens++].type = TOKEN_LE;
				string++;
			}
			else if (string[1] == '<') {
				if (string[2] == '=') {
					tokens[numTokens++].type = TOKEN_LSHIFTE;
					string++;
				}
				else 
					tokens[numTokens++].type = TOKEN_LSHIFT;
				string++;
			}
			else
				tokens[numTokens++].type = TOKEN_L;
			break;
		case '>':
			if (string[1] == '=') {
				tokens[numTokens++].type = TOKEN_GE;
				string++;
			}
			else if (string[1] == '>') {
				if (string[2] == '=') {
					tokens[numTokens++].type = TOKEN_RSHIFTE;
					string++;
				}
				else 
					tokens[numTokens++].type = TOKEN_RSHIFT;
				string++;
			}
			else
				tokens[numTokens++].type = TOKEN_G;
			break;
		case ')':
				tokens[numTokens++].type = TOKEN_CLOSE_PAREN;
			break;
		case '(':
				tokens[numTokens++].type = TOKEN_OPEN_PAREN;
			break;
		case ']':
				tokens[numTokens++].type = TOKEN_CLOSE_BRACKET;
			break;
		case '[':
				tokens[numTokens++].type = TOKEN_OPEN_BRACKET;
			break;
		case '}':
			tokens[numTokens++].type = TOKEN_CLOSE_CODE;
			break;
		case '{':
			tokens[numTokens++].type = TOKEN_OPEN_CODE;
			break;
		case '"':
		case '\'':
			tokens[numTokens].type = TOKEN_STRING;
			{
				unsigned char type = *string;
				string++;
				unsigned char *start = string;
				while (string<end) {
					if (*string == '\n' || (*string == '\r' && string[1] !='\n')) {
						line++;
						lineStart = string+1;
					}
					if (*string == '|') string+=2;
					else if (*string == type) break;
					else string ++;
				}
				if (string >= end || !(tokens[numTokens].val = (unsigned char*)malloc(sizeof(unsigned char)*(string-start+1)))) {
					tokens[numTokens++].type = TOKEN_ERROR;
					LexError(line+1, lineStart, string-1, "String not closed");
					return 1;
				}
				memcpy(tokens[numTokens].val, start, string-start);
				int i;
				int j;
				unsigned char *s = tokens[numTokens].val;
				for (j = 0, i=0; i<string-start; i++,j++) {
					if (s[i] == '|' && (type == '\"' || s[i+1]=='\'')) {
						i++;
						if (LCASE(s[i]) == 'n')
							s[j] = '\n';
						else if (LCASE(s[i]) == 'r')
							s[j] = '\r';
						else if (LCASE(s[i]) == 't')
							s[j] = '\t';
						else if (LCASE(s[i]) == 'x' && (start+i)!= string && IsHex(s[i+1])) {
							unsigned int L = 0;
							int count = 0;
							while ((start+i)!= string && IsHex(s[i+1]) && count < 8) {
								i++;
								if (IsNumber(s[i])) L = L*16 + s[i]-'0';
								else L =  L*16 + LCASE(s[i])-'a' + 10;
								count ++;
							}
							if (L == 0) {
								s[j] = '\0';
							}
							else j += UTF32toUTF8(s+j, &L, 1)-1;
						}
						else if (IsNumber(s[i])) {
							unsigned int L = 0;
							int count = 0;
							i--;
							while ((start+i)!= string && IsNumber(s[i+1]) && count < 3) {
								i++;
								L = L*10 + s[i]-'0';
								count ++;
							}
							if (L == 0) {
								s[j] = '\0';
							}
							else j += UTF32toUTF8(s+j, &L, 1)-1;
						}
						else
							s[j] = s[i];
					}
					else
						s[j] = s[i];
				}
				tokens[numTokens].intVal = j;
				tokens[numTokens++].val[j] = 0;
			}
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			{
				unsigned char *v2, *d2;
				if (line >= 282) {
					line = line;
				}
				__int64 v = strtoi64((char*)string, (char**)&v2, 0);
				double d = strtod((char*)string, (char**)&d2);
				if (v != _I64_MAX && v != _I64_MIN && v2 != string && v2>=d2 && v2!=string) {
					tokens[numTokens].type = TOKEN_INT;
					tokens[numTokens++].intVal = v;
					string = v2-1;
				}
				else if (d != HUGE_VAL && d != -HUGE_VAL && d2>=v2 && v2!=string) {
					tokens[numTokens].type = TOKEN_DOUBLE;
					tokens[numTokens++].doubleVal = d;
					string = d2-1;
				}
				else {
					LexError(line, lineStart, string, "Error parsing number");
					tokens[numTokens++].type = TOKEN_ERROR;
					string++;
					return 1;
				}
				if ((UCASE(string[1]) >= 'A' && UCASE(string[1]) <= 'Z') ||
					string [1]== '_' || string[1] >= 0x80) {
					LexError(line, lineStart, string, "Space expected");
					tokens[numTokens-1].type = TOKEN_ERROR;
					string++;
					return 1;
				}
			}
			break;
		default:
			if (IsLetter(*string) || *string == '_') {
				unsigned char *start = string;
				string ++;
				while (IsLetter(*string) || IsNumber(*string) || *string == '_' || *string >= 0x80) {
					string++;
				}
				int l = (int)(string-start);
				int r, g, b, a;
				char c;
				if (l == 1 && numTokens && start[-1] == '=' && start[-2] == '+' && (start[0] == 's' || start[0] == 'S')) {
					tokens[numTokens-1].type = TOKEN_CONCATE;
				}
				else if (l == 1 && numTokens && start[-1] == '+' && (start[0] == 's' || start[0] == 'S') && tokens[numTokens-1].type == TOKEN_ADD) {
					tokens[numTokens-1].type = TOKEN_CONCAT;
				}
				else if (l == 1 && numTokens && (start[-1] == '=' || start[-1] == '<' || start[-1] == '>') && UCASE(start[0]) == 'S' &&
					(tokens[numTokens-1].type == TOKEN_E || tokens[numTokens-1].type == TOKEN_NE || tokens[numTokens-1].type == TOKEN_GE || tokens[numTokens-1].type == TOKEN_G || tokens[numTokens-1].type == TOKEN_L || tokens[numTokens-1].type == TOKEN_LE)) {
						if (start[0] == 'S') tokens[numTokens-1].type = (ScriptTokenType)(tokens[numTokens-1].type - TOKEN_E+TOKEN_SE);
						else tokens[numTokens-1].type = (ScriptTokenType)(tokens[numTokens-1].type+TOKEN_SCIE - TOKEN_E);
				}
				else if (l == 2 && substrcmp((char*)start, "pi") == 0) {
					tokens[numTokens].type = TOKEN_DOUBLE;
					tokens[numTokens].doubleVal = M_PI;
				}
				else if (l == 3 && substrcmp((char*)start, "RGB") == 0 && sscanf((char*)string, " ( %i , %i , %i %c", &r, &g, &b, &c) == 4 && c == ')') {
					tokens[numTokens].type = TOKEN_INT;
					tokens[numTokens].intVal = 0xFF000000 + ((r&0xFF)<<16) + ((g&0xFF)<<8) + (b&0xFF);
					string = strchr(string, ')')+1;
					numTokens++;
				}
				else if (l == 4 && substrcmp((char*)start, "RGBA") == 0 && sscanf((char*)string, " ( %i , %i , %i , %i %c", &r, &g, &b, &a, &c) == 5 && c == ')') {
					tokens[numTokens].type = TOKEN_INT;
					tokens[numTokens].intVal = ((a&0xFF)<<24) + ((r&0xFF)<<16) + ((g&0xFF)<<8) + (b&0xFF);
					string = strchr(string, ')')+1;
					numTokens++;
				}
				else {
					int i;
					for (i = sizeof(reserved)/sizeof(reserved[0])-1; i>=0; i--) {
						if (!substrcmp((const char*)start, reserved[i].val) && l == (int)strlen(reserved[i].val)) {
							tokens[numTokens++].type = reserved[i].token;
							break;
						}
					}
					if (i < 0) {
						char temp = string[0];
						string[0] = 0;
						TableEntry<unsigned char*>* entry = defines.Find(start);
						string[0] = temp;
						if (entry) {
							if (depth >= 100) {
								LexError(line, lineStart, start, "Maximum recursion depth exceeded");
								return 1;
							}
							unsigned char *s = entry->data;
							int len = strlen(s);
							unsigned char *end = s+len;
							int pos = numTokens;
							while (s < end) {
								int t = ReadToken(s, tokens, numTokens, maxTokens, comment, end, line, s, depth+1);
								if (t) {
									LexDefError((char*)entry->sv->value);
									if (depth == 0) {
										LexError(line, lineStart, start, "Error in #define");
									}
									return t+1;
								}
								if (depth == 0) {
									while (pos<numTokens) {
										tokens[pos].pos = start;
										tokens[pos].start = lineStart;
										pos ++;
									}
								}
							}
						}
						else if (l > 1024) {
							tokens[numTokens++].type = TOKEN_ERROR;
							LexError(line, lineStart, start, "Identifier too long");
							return 1;
						}
						else if (!(tokens[numTokens].val = (unsigned char*) malloc((string-start+1)*sizeof(unsigned char)))) {
							tokens[numTokens++].type = TOKEN_ERROR;
							LexError(line, lineStart, start, "Memory allocation error");
							return 1;
						}
						else {
							memcpy(tokens[numTokens].val, start, (string-start)*sizeof(unsigned char));
							tokens[numTokens].val[string-start] = 0;
							tokens[numTokens].intVal = (int)(string-start);
							tokens[numTokens++].type = TOKEN_IDENTIFIER;
						}
					}
				}
				string--;
			}
			else {
				tokens[numTokens++].type = TOKEN_ERROR;
				LexError(line, lineStart, string, "Invalid character");
				while (string[0] && !IsWhitespace(string[0])) string ++;
				return 1;
			}
			break;
	}
	string++;
	return 0;
}

int Tokenize(unsigned char* string, int len, Token * &tokens, int &numTokens, wchar_t *path) {
	int maxTokens = 30 + len/2;
	tokens = (Token*) calloc(sizeof(Token), maxTokens);
	int line = 0;
	numTokens = 0;
	unsigned char *end = string+len;
	int numErrors = 0;
	int comment = 0;
	unsigned char *lineStart = string;
	while (string<end) {
		if ((string[0] == '\r' && (string == end-1 ||  string[1] != '\n')) || string[0] == '\n' || !string[0]) {
			line++;
			lineStart = string+1;
		}
		else if (string[0] == '\n') line++;
		if (comment) {
			if (string[0] == '*' && string[1] == '/') {
				string +=2;
				comment = 0;
				continue;
			}
			string++;
			continue;
		}
		else if (string[0] == '#') {
			unsigned char *start = string+1;
			while (*start == '\t' || *start == ' ') start++;
			string = start;
			while (*string && *string != '\r' && *string != '\n') string++;
			while (start < string && (string[-1] == ' ' || string[-1] == '\t')) string--;
			if (numTokens && tokens[numTokens-1].line == line) {
				numErrors ++;
				LexError(line, lineStart, string, "Preprocessor instruction must occur on its own line");
				break;
			}
			unsigned char *end = start;
			while (IsLetter(*end) || IsNumber(*end) || *end == '_' || *end >= 0x80) {
				end++;
			}
			int l = (int)(end-start);
			unsigned char *next = end;
			while (*next == ' ' || *next == '\t') next ++;

			unsigned char temp = string[0];
			string[0] = 0;

			if ((l == 8 && substricmp((char*)start, "requires") == 0) ||
				(l == 6 && substricmp((char*)start, "import") == 0)) {
				wchar_t *p = UTF8toUTF16Alloc(next);
				int res;
				int index = 0;
				if (!p || -2 == (res = AddFile(p, path, index))) {
					LexError(line, lineStart, next, "Invalid File Name");
				}
				else if (res == -1 && l == 6) {
					unsigned char* temp = UTF16toUTF8Alloc(path);
					LoadScript(index, 1);
					if (temp) {
						errorPrintf(2, "Returning to: %s\r\n", temp);
						free(temp);
					}
				}
				free(p);
			}
			else if (l == 6 && substricmp((char*)start, "define") == 0) {
				unsigned char *p = next;
				while (*p && *p != ' ' && *p != '\t') p++;
				int l = (int) (p-next);
				while (*p == ' ' || *p == '\t') p++;

				unsigned char temp2 = next[l];
				next[l] = 0;
				TableEntry<unsigned char*>* entry = defines.Find(start);
				next[l] = temp2;
				if (entry) {
					LexWarning(line, lineStart, start, "Duplicate define");
					free(entry->data);
				}
				else {
					ScriptValue s;
					if (!CreateStringValue(s, next, l)) {
						numErrors ++;
						LexError(line, lineStart, start, "Out of memory");
						break;
					}
					if (!(entry = defines.Add(s.stringVal))) {
						numErrors ++;
						LexError(line, lineStart, start, "Out of memory");
						break;
					}
				}
				entry->data = strdup(p);
			}
			else {
				numErrors ++;
				LexError(line, lineStart, start, "Unrecognized instruction");
				break;
			}
			string[0] = temp;
			string++;
			continue;
		}
		else {
			numErrors += ReadToken(string, tokens, numTokens, maxTokens, comment, end, line, lineStart);
		}
	}
	tokens[numTokens].line = line;
	tokens[numTokens].pos = string;
	tokens[numTokens].start = lineStart;
	tokens[numTokens++].type = TOKEN_END;
	int flen = numTokens;
	int dest = 0;
	int delta = 0;
	for (int i=0; i<numTokens-1; i++) {
		if (tokens[i].type == TOKEN_DOLLAR && tokens[i+1].type != TOKEN_IDENTIFIER) {
			flen++;
		}
		tokens[dest++] = tokens[i];
	}
	flen-=delta;
	tokens[--flen] = tokens[--numTokens];
	numTokens-=delta;
	int temp = numTokens-1;
	numTokens = flen+1;
	flen--;
	while (temp>=0) {
		if (tokens[temp].type == TOKEN_DOLLAR && tokens[temp+1].type != TOKEN_IDENTIFIER) {
			memset(tokens+flen, 0, sizeof(Token));
			tokens[flen].type = TOKEN_IDENTIFIER;
			tokens[flen].val = (unsigned char*)strdup("");
			if (tokens[flen].val==0) {
				numErrors ++;
				tokens[flen].type = TOKEN_ERROR;
			}
			flen--;
		}
		tokens[flen--] = tokens[temp--];
	}
	srealloc(tokens, numTokens*sizeof(Token));
	return numErrors;
}

void CleanupDefines() {
	for (int i=0; i<defines.numValues; i++) {
		free(defines.table[i].data);
	}
	defines.Cleanup();
}
