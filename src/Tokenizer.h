#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "ScriptEnums.h"
#include "Unicode.h"

struct Token {
	ScriptTokenType type;
	union {
		__int64 intVal;
		double doubleVal;
	};
	int line;
	unsigned char *val;
	unsigned char *pos;
	unsigned char *start;
};

void CleanupTokens(Token *tokens, int numTokens);
int Tokenize(unsigned char* string, int len, Token * &tokens, int &numTokens, wchar_t *path);
#endif
