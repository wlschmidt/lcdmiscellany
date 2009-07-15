#ifndef PARSER_H
#define PARSER_H

struct Token;

#include "ScriptEnums.h"

struct ParseTree {
	union {
		__int64 intVal;
		double doubleVal;
	};
	unsigned char *val;
	unsigned char *pos;
	unsigned char *start;
	int line;

	int numChildren;
	NodeType type;
	// Only for loops have 4 elements.
	ParseTree *children[5];
};

ParseTree * ParseScript(Token *tokens, int numTokens);
void CleanupTree(ParseTree *t);
void CleanupParser();
#endif
