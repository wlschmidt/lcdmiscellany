#include "Parser.h"
#include "Tokenizer.h"
#include "malloc.h"
#include "config.h"
//#include <stdio.h>
//#include "string.h"
#define REDUCE 0x20000000
#define SHIFT  0x40000000
#define ACCEPT 0x60000000

#define ACTION_MASK 0x60000000

struct Rule {
	ScriptTokenType result;
	NodeType nodeType;
	unsigned char numTerms:4;
	unsigned char operatorPos:4;
	ScriptTokenType terms[8];
};

const Rule rules[] = {

	{PARSER_SCRIPT, NODE_NONE, 2, 1, PARSER_FUNCTION_LIST, TOKEN_END},

	{PARSER_FUNCTION_LIST, NODE_FUNCTION_LIST, 2, 1, PARSER_FUNCTION, PARSER_FUNCTION_LIST},
	{PARSER_FUNCTION_LIST, NODE_FUNCTION_LIST, 1, 0, PARSER_FUNCTION},
	{PARSER_FUNCTION_LIST, NODE_FUNCTION_LIST, 2, 1, PARSER_STRUCT, PARSER_FUNCTION_LIST},
	{PARSER_FUNCTION_LIST, NODE_FUNCTION_LIST, 1, 0, PARSER_STRUCT},
	//{PARSER_FUNCTION_LIST, NODE_FUNCTION_LIST, 1, 0, PARSER_RAW_STRUCT},

	/*
	{PARSER_RAW_STRUCT, NODE_RAW_STRUCT, 6, 1, TOKEN_RAW, TOKEN_STRUCT, TOKEN_IDENTIFIER, TOKEN_OPEN_CODE, PARSER_RAW_STRUCT_LIST, TOKEN_CLOSE_CODE, TOKEN_END_STATEMENT},
	{PARSER_RAW_STRUCT_LIST, NODE_RAW_STRUCT_LIST, 3, 0, PARSER_RAW_STRUCT_ENTRY, TOKEN_SEMICOLON, PARSER_STRUCT_LIST},
	{PARSER_RAW_STRUCT_LIST, NODE_RAW_STRUCT_LIST, 2, 0, PARSER_RAW_STRUCT_ENTRY, TOKEN_SEMICOLON},
	{PARSER_RAW_STRUCT_ENTRY, NODE_STRUCT_VALUES, 2, 0, PARSER_TYPE, TOKEN_IDENTIFIER},
	{PARSER_RAW_STRUCT_ENTRY, NODE_STRUCT_VALUES, 3, 0, PARSER_RAW_STRUCT_ENTRY, TOKEN_COMMA, TOKEN_IDENTIFIER},
	{PARSER_RAW_STRUCT_ENTRY, NODE_STRUCT_VALUES, 5, 0, PARSER_TYPE, TOKEN_ TOKEN_IDENTIFIER},
	{PARSER_RAW_STRUCT_ENTRY, NODE_STRUCT_VALUES, 6, 0, PARSER_RAW_STRUCT_ENTRY, TOKEN_COMMA, TOKEN_IDENTIFIER},

	{PARSER_TYPE, NODE_INT_TYPE, 1, 0, TOKEN_INT_TYPE},
	{PARSER_TYPE, NODE_FLOAT_TYPE, 1, 0, TOKEN_FLOAT_TYPE},
	{PARSER_TYPE, NODE_DOUBLE_TYPE, 1, 0, TOKEN_DOUBLE_TYPE},
	{PARSER_TYPE, NODE_CHAR_TYPE, 1, 0, TOKEN_CHAR_TYPE},
	{PARSER_TYPE, NODE_STRING_TYPE, 1, 0, TOKEN_STRING_TYPE},
	//*/

	{PARSER_STRUCT, NODE_STRUCT, 5, 0, TOKEN_STRUCT, TOKEN_IDENTIFIER, TOKEN_OPEN_CODE, PARSER_STRUCT_LIST, TOKEN_CLOSE_CODE, TOKEN_END_STATEMENT},
	{PARSER_STRUCT, NODE_STRUCT, 7, 0, TOKEN_STRUCT, TOKEN_IDENTIFIER, TOKEN_EXTENDS, PARSER_IDENTIFIER_LIST, TOKEN_OPEN_CODE, PARSER_STRUCT_LIST, TOKEN_CLOSE_CODE, TOKEN_END_STATEMENT},
	//{PARSER_STRUCT, NODE_STRUCT, 4, 1, TOKEN_STRUCT, TOKEN_IDENTIFIER, TOKEN_OPEN_CODE, TOKEN_CLOSE_CODE},
	{PARSER_STRUCT_LIST, NODE_STRUCT_LIST, 2, 1, PARSER_STRUCT_ENTRY, PARSER_STRUCT_LIST},
	{PARSER_STRUCT_LIST, NODE_STRUCT_LIST, 1, 0, PARSER_STRUCT_ENTRY},

	{PARSER_STRUCT_ENTRY, NODE_NONE, 3, 1, TOKEN_VAR, PARSER_STRUCT_VALUES, TOKEN_END_STATEMENT},
	{PARSER_STRUCT_ENTRY, NODE_NONE, 1, 0, PARSER_FUNCTION},
	{PARSER_STRUCT_VALUES, NODE_STRUCT_VALUES, 1, 0, TOKEN_IDENTIFIER},
	{PARSER_STRUCT_VALUES, NODE_STRUCT_VALUES, 2, 1, TOKEN_MOD, TOKEN_IDENTIFIER},
	{PARSER_STRUCT_VALUES, NODE_STRUCT_VALUES, 3, 0, TOKEN_IDENTIFIER, TOKEN_COMMA, PARSER_STRUCT_VALUES},
	{PARSER_STRUCT_VALUES, NODE_STRUCT_VALUES, 4, 1, TOKEN_MOD, TOKEN_IDENTIFIER, TOKEN_COMMA, PARSER_STRUCT_VALUES},


	{PARSER_FUNCTION, NODE_IGNORE, 1, 0, TOKEN_END_STATEMENT},
	{PARSER_FUNCTION, NODE_FUNCTION_ALIAS, 5, 3, TOKEN_ALIAS, TOKEN_FUNCTION, TOKEN_IDENTIFIER, TOKEN_IDENTIFIER, TOKEN_END_STATEMENT},

	//{PARSER_FUNCTION, NODE_DLL32, 4, 1, TOKEN_DLL32, TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_END_STATEMENT},
	//{PARSER_FUNCTION, NODE_DLL64, 4, 1, TOKEN_DLL64, TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_END_STATEMENT},
	{PARSER_FUNCTION, NODE_DLL32, 5, 1, TOKEN_DLL32, TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_STRING, TOKEN_END_STATEMENT},
	{PARSER_FUNCTION, NODE_DLL64, 5, 1, TOKEN_DLL64, TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_STRING, TOKEN_END_STATEMENT},

	{PARSER_FUNCTION, NODE_DLL32, 4, 1, TOKEN_DLL32, TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_END_STATEMENT},
	{PARSER_FUNCTION, NODE_DLL64, 4, 1, TOKEN_DLL64, TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_END_STATEMENT},

	{PARSER_FUNCTION, NODE_DLL_FUNCTION, 6, 1, TOKEN_DLL, TOKEN_IDENTIFIER, TOKEN_FUNCTION, TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_END_STATEMENT},
	{PARSER_FUNCTION, NODE_DLL_FUNCTION, 7, 1, TOKEN_DLL, TOKEN_IDENTIFIER, TOKEN_FUNCTION, TOKEN_IDENTIFIER, TOKEN_STRING, PARSER_DLL_ARG_LIST, TOKEN_END_STATEMENT},
	{PARSER_FUNCTION, NODE_DLL_FUNCTION_SIMPLE, 7, 1, TOKEN_DLL, TOKEN_IDENTIFIER, TOKEN_IDENTIFIER, TOKEN_IDENTIFIER, TOKEN_STRING, PARSER_DLL_ARG_LIST, TOKEN_END_STATEMENT},

	// Hack to allow 0 element list.
	{PARSER_DLL_ARG_LIST, NODE_DLL_ARG, 2, 0, TOKEN_OPEN_PAREN, TOKEN_CLOSE_PAREN},
	{PARSER_DLL_ARG_LIST, NODE_NONE, 3, 0, TOKEN_OPEN_PAREN, PARSER_DLL_ARGS, TOKEN_CLOSE_PAREN},

	{PARSER_DLL_ARGS, NODE_DLL_ARGS, 1, 0, PARSER_DLL_ARG},
	{PARSER_DLL_ARGS, NODE_DLL_ARGS, 3, 0, PARSER_DLL_ARG, TOKEN_COMMA, PARSER_DLL_ARGS},

	{PARSER_DLL_ARG, NODE_DLL_ARG, 2, 0, TOKEN_IDENTIFIER, TOKEN_IDENTIFIER},
	{PARSER_DLL_ARG, NODE_DLL_ARG, 1, 0, TOKEN_IDENTIFIER},


	{PARSER_FUNCTION, NODE_FUNCTION, 6, 0, TOKEN_FUNCTION, TOKEN_IDENTIFIER, TOKEN_OPEN_PAREN, PARSER_IDENTIFIER_LIST, TOKEN_CLOSE_PAREN, PARSER_STATEMENT_BLOCK},
	{PARSER_FUNCTION, NODE_FUNCTION, 5, 0, TOKEN_FUNCTION, TOKEN_IDENTIFIER, TOKEN_OPEN_PAREN, TOKEN_CLOSE_PAREN, PARSER_STATEMENT_BLOCK},
	{PARSER_STRUCT_ENTRY, NODE_FUNCTION, 7, 0, TOKEN_FUNCTION, TOKEN_MOD, TOKEN_IDENTIFIER, TOKEN_OPEN_PAREN, PARSER_IDENTIFIER_LIST, TOKEN_CLOSE_PAREN, PARSER_STATEMENT_BLOCK},
	{PARSER_STRUCT_ENTRY, NODE_FUNCTION, 6, 0, TOKEN_FUNCTION, TOKEN_MOD, TOKEN_IDENTIFIER, TOKEN_OPEN_PAREN, TOKEN_CLOSE_PAREN, PARSER_STATEMENT_BLOCK},

	//{PARSER_SIMPLE_EXPRESSION, NODE_FUNCTION_CALL, 3, TOKEN_IDENTIFIER, TOKEN_OPEN_PAREN, TOKEN_CLOSE_PAREN},
	{PARSER_SIMPLE_DEREFERENCE_EXPRESSION, NODE_STRUCT_REF_CALL, 8, 1, PARSER_DEREFERENCE_EXPRESSION, TOKEN_PERIOD, TOKEN_CALL, TOKEN_OPEN_PAREN, PARSER_EXPRESSION, TOKEN_COMMA, PARSER_EXPRESSION, TOKEN_CLOSE_PAREN},
	{PARSER_SIMPLE_DEREFERENCE_EXPRESSION, NODE_REF_CALL, 6, 0, TOKEN_CALL, TOKEN_OPEN_PAREN, PARSER_EXPRESSION, TOKEN_COMMA, PARSER_EXPRESSION, TOKEN_CLOSE_PAREN},
	{PARSER_SIMPLE_DEREFERENCE_EXPRESSION, NODE_FUNCTION_CALL, 2, 0, TOKEN_IDENTIFIER, PARSER_VALUE_LIST},
	{PARSER_SIMPLE_DEREFERENCE_EXPRESSION, NODE_STRUCT_FUNCTION_CALL, 4, 1, PARSER_DEREFERENCE_EXPRESSION, TOKEN_PERIOD, TOKEN_IDENTIFIER, PARSER_VALUE_LIST},
	{PARSER_SIMPLE_DEREFERENCE_EXPRESSION, NODE_STRUCT_FUNCTION_CALL, 5, 1, PARSER_DEREFERENCE_EXPRESSION, TOKEN_PERIOD, TOKEN_MOD, TOKEN_IDENTIFIER, PARSER_VALUE_LIST},
	{PARSER_SIMPLE_DEREFERENCE_EXPRESSION, NODE_STRUCT_THIS_FUNCTION_CALL, 3, 0, TOKEN_MOD, TOKEN_IDENTIFIER, PARSER_VALUE_LIST},
	//{PARSER_FUNCTION_CALL, NODE_FUNCTION_CALL, 4, TOKEN_IDENTIFIER, TOKEN_OPEN_PAREN, PARSER_LIST, TOKEN_CLOSE_PAREN},

	{PARSER_VALUE_LIST, NODE_MAKE_COPY, 4, 0, TOKEN_OPEN_PAREN, TOKEN_APPEND, PARSER_EXPRESSION, TOKEN_CLOSE_PAREN},
	{PARSER_VALUE_LIST, NODE_LIST_CREATE, 3, 0, TOKEN_OPEN_PAREN, PARSER_EXPRESSION, TOKEN_CLOSE_PAREN},
	{PARSER_VALUE_LIST, NODE_EMPTY_LIST, 2, 0, TOKEN_OPEN_PAREN, TOKEN_CLOSE_PAREN},
	{PARSER_VALUE_LIST, NODE_LIST_ADD_NULL, 3, 0, TOKEN_OPEN_PAREN, PARSER_LIST_EXPRESSION, TOKEN_CLOSE_PAREN},
	{PARSER_VALUE_LIST, NODE_LIST_ADD, 4, 0, TOKEN_OPEN_PAREN, PARSER_LIST_EXPRESSION, PARSER_EXPRESSION, TOKEN_CLOSE_PAREN},
	{PARSER_VALUE_LIST, NODE_APPEND, 5, 0, TOKEN_OPEN_PAREN, PARSER_LIST_EXPRESSION, TOKEN_APPEND, PARSER_EXPRESSION, TOKEN_CLOSE_PAREN},

	{PARSER_SIMPLE_EXPRESSION, NODE_NONE, 1, 0, PARSER_LIST},
	//{PARSER_LIST, NODE_LIST_ADD_NULL, 4, TOKEN_LIST, TOKEN_OPEN_PAREN, PARSER_LIST_EXPRESSION, TOKEN_CLOSE_PAREN},
	//{PARSER_LIST, NODE_LIST_CREATE, 4, TOKEN_LIST, TOKEN_OPEN_PAREN, PARSER_EXPRESSION, TOKEN_CLOSE_PAREN},
	//{PARSER_LIST, NODE_EMPTY_LIST, 3, TOKEN_LIST, TOKEN_OPEN_PAREN, TOKEN_CLOSE_PAREN},
//	{PARSER_SIMPLE_EXPRESSION, NODE_DICT_TO_LIST, 4, 0, TOKEN_DICT_TO_LIST, TOKEN_OPEN_PAREN, PARSER_EXPRESSION, TOKEN_CLOSE_PAREN},

	{PARSER_LIST, NODE_NONE, 4, 0, TOKEN_OPEN_PAREN, TOKEN_APPEND, PARSER_EXPRESSION, TOKEN_CLOSE_PAREN},
	{PARSER_LIST, NODE_LIST_ADD_NULL, 3, 0, TOKEN_OPEN_PAREN, PARSER_LIST_EXPRESSION, TOKEN_CLOSE_PAREN},
	{PARSER_LIST, NODE_LIST_ADD, 4, 0, TOKEN_OPEN_PAREN, PARSER_LIST_EXPRESSION, PARSER_EXPRESSION, TOKEN_CLOSE_PAREN},
	{PARSER_LIST, NODE_APPEND, 5, 0, TOKEN_OPEN_PAREN, PARSER_LIST_EXPRESSION, TOKEN_APPEND, PARSER_EXPRESSION, TOKEN_CLOSE_PAREN},
	//{PARSER_LIST, NODE_LIST_CREATE, 4, TOKEN_OPEN_PAREN, PARSER_EXPRESSION, TOKEN_COMMA, TOKEN_CLOSE_PAREN},
	//{PARSER_LIST_EXPRESSION, NODE_LIST_CREATE, 3, PARSER_LIST_EXPRESSION, TOKEN_COMMA, PARSER_EXPRESSION},

	{PARSER_LIST_EXPRESSION, NODE_LIST_ADD, 3, 0, PARSER_LIST_EXPRESSION, PARSER_EXPRESSION, TOKEN_COMMA},
	{PARSER_LIST_EXPRESSION, NODE_LIST_ADD_NULL, 2, 0, PARSER_LIST_EXPRESSION, TOKEN_COMMA},
	{PARSER_LIST_EXPRESSION, NODE_APPEND, 4, 0, PARSER_LIST_EXPRESSION, TOKEN_APPEND, PARSER_EXPRESSION, TOKEN_COMMA},

	{PARSER_LIST_EXPRESSION, NODE_LIST_CREATE_NULL, 1, 0, TOKEN_COMMA},
	//{PARSER_LIST_EXPRESSION, NODE_LIST_CREATE, 2, PARSER_EXPRESSION, TOKEN_COMMA},
	{PARSER_LIST_EXPRESSION, NODE_LIST_CREATE, 2, 0, PARSER_EXPRESSION, TOKEN_COMMA},
	{PARSER_LIST_EXPRESSION, NODE_NONE, 3, 0, TOKEN_APPEND, PARSER_EXPRESSION, TOKEN_COMMA},


	{PARSER_SIMPLE_EXPRESSION, NODE_NONE, 1, 0, PARSER_DICT},
	{PARSER_DICT, NODE_NONE, 4, 0, TOKEN_DICT, TOKEN_OPEN_PAREN, PARSER_DICT_EXPRESSION, TOKEN_CLOSE_PAREN},
	{PARSER_DICT, NODE_NONE, 3, 0, TOKEN_OPEN_PAREN, PARSER_DICT_EXPRESSION, TOKEN_CLOSE_PAREN},
	{PARSER_DICT, NODE_EMPTY_DICT, 3, 0, TOKEN_DICT, TOKEN_OPEN_PAREN, TOKEN_CLOSE_PAREN},
	{PARSER_DICT_EXPRESSION, NODE_APPEND, 3, 0, PARSER_DICT_EXPRESSION, TOKEN_COMMA, PARSER_MINI_DICT_EXPRESSION},
	{PARSER_DICT_EXPRESSION, NODE_NONE, 1, 0, PARSER_MINI_DICT_EXPRESSION},
	{PARSER_MINI_DICT_EXPRESSION, NODE_DICT_CREATE, 3, 0, PARSER_EXPRESSION, TOKEN_COLON, PARSER_EXPRESSION},
	{PARSER_MINI_DICT_EXPRESSION, NODE_MAKE_COPY, 2, 0, TOKEN_APPEND, PARSER_EXPRESSION},

	{PARSER_SIMPLE_EXPRESSION, NODE_SIZE, 4, 0, TOKEN_SIZE, TOKEN_OPEN_PAREN, PARSER_EXPRESSION, TOKEN_CLOSE_PAREN},

	{PARSER_IDENTIFIER_LIST, NODE_IDENTIFIER_LIST, 3, 0, TOKEN_IDENTIFIER, TOKEN_COMMA, PARSER_IDENTIFIER_LIST},
	{PARSER_IDENTIFIER_LIST, NODE_IDENTIFIER_LIST, 1, 0, TOKEN_IDENTIFIER},

	{PARSER_IDENTIFIER_LIST, NODE_IDENTIFIER_LIST, 4, 0, TOKEN_DOLLAR, TOKEN_IDENTIFIER, TOKEN_COMMA, PARSER_IDENTIFIER_LIST},
	{PARSER_IDENTIFIER_LIST, NODE_IDENTIFIER_LIST, 2, 0, TOKEN_DOLLAR, TOKEN_IDENTIFIER},

	{PARSER_STATEMENT_LIST, NODE_NONE, 1, 0, PARSER_STATEMENT},
	{PARSER_STATEMENT_LIST, NODE_STATEMENT_LIST, 2, 0, PARSER_STATEMENT_LIST, PARSER_STATEMENT},

	{PARSER_STATEMENT_BLOCK, NODE_NONE, 3, 0, TOKEN_OPEN_CODE, PARSER_STATEMENT_LIST, TOKEN_CLOSE_CODE},
	{PARSER_STATEMENT_BLOCK, NODE_EMPTY, 2, 0, TOKEN_OPEN_CODE, TOKEN_CLOSE_CODE},

	{PARSER_STATEMENT, NODE_DISCARD_LAST, 2, 0, PARSER_EXPRESSION, TOKEN_END_STATEMENT},
	{PARSER_STATEMENT, NODE_EMPTY, 1, 0, TOKEN_END_STATEMENT},
	{PARSER_STATEMENT, NODE_NONE, 1, 0, PARSER_STATEMENT_BLOCK},
	//{PARSER_STATEMENT, NODE_DISCARD_FIRST, 1, PARSER_IF},
	//{PARSER_STATEMENT, NODE_DISCARD_FIRST, 1, PARSER_IF_ELSE},

	//{PARSER_STATEMENT, NODE_NONE, 1, PARSER_RETURN},
	//{PARSER_RETURN, NODE_RETURN, 3, TOKEN_RETURN, PARSER_EXPRESSION, TOKEN_END_STATEMENT},

	{PARSER_STATEMENT, NODE_RETURN, 3, 0, TOKEN_RETURN, PARSER_EXPRESSION, TOKEN_END_STATEMENT},
	{PARSER_STATEMENT, NODE_RETURN, 2, 0, TOKEN_RETURN, TOKEN_END_STATEMENT},
	{PARSER_STATEMENT, NODE_BREAK, 2, 0, TOKEN_BREAK, TOKEN_END_STATEMENT},
	{PARSER_STATEMENT, NODE_CONTINUE, 2, 0, TOKEN_CONTINUE, TOKEN_END_STATEMENT},

	{PARSER_STATEMENT, NODE_FOR, 7, 0, TOKEN_FOR, TOKEN_OPEN_PAREN, PARSER_FOR_PRE_STATEMENT_LIST, PARSER_EXPRESSION, PARSER_FOR_POST_STATEMENT_LIST, TOKEN_CLOSE_PAREN, PARSER_STATEMENT},

	{PARSER_FOR_PRE_STATEMENT_LIST, NODE_EMPTY, 1, 0, TOKEN_END_STATEMENT},
	{PARSER_FOR_POST_STATEMENT_LIST, NODE_EMPTY, 1, 0, TOKEN_END_STATEMENT},
	{PARSER_FOR_PRE_STATEMENT_LIST, NODE_DISCARD_LAST, 2, 0, PARSER_FOR_SUB_STATEMENT_LIST, TOKEN_END_STATEMENT},
	{PARSER_FOR_POST_STATEMENT_LIST, NODE_DISCARD_LAST, 2, 0, TOKEN_END_STATEMENT, PARSER_FOR_SUB_STATEMENT_LIST},

	{PARSER_FOR_SUB_STATEMENT_LIST, NODE_NONE, 1, 0, PARSER_EXPRESSION},
	{PARSER_FOR_SUB_STATEMENT_LIST, NODE_DISCARD_FIRST, 3, 0, PARSER_EXPRESSION, TOKEN_COMMA, PARSER_FOR_SUB_STATEMENT_LIST},

	{PARSER_STATEMENT, NODE_WHILE, 5, 0, TOKEN_WHILE, TOKEN_OPEN_PAREN, PARSER_EXPRESSION, TOKEN_CLOSE_PAREN, PARSER_STATEMENT},
	{PARSER_STATEMENT, NODE_DO_WHILE, 7, 0, TOKEN_DO, PARSER_STATEMENT, TOKEN_WHILE, TOKEN_OPEN_PAREN, PARSER_EXPRESSION, TOKEN_CLOSE_PAREN, TOKEN_END_STATEMENT},


	//{PARSER_IF, NODE_IF, 5, TOKEN_IF, TOKEN_OPEN_PAREN, PARSER_EXPRESSION, TOKEN_CLOSE_PAREN, PARSER_STATEMENT},
	//{PARSER_IF_ELSE, NODE_IF_ELSE, 7, TOKEN_IF, TOKEN_OPEN_PAREN, PARSER_EXPRESSION, TOKEN_CLOSE_PAREN, PARSER_STATEMENT, TOKEN_ELSE, PARSER_STATEMENT},
	{PARSER_STATEMENT, NODE_IF, 5, 0, TOKEN_IF, TOKEN_OPEN_PAREN, PARSER_EXPRESSION, TOKEN_CLOSE_PAREN, PARSER_STATEMENT},
	{PARSER_STATEMENT, NODE_IF_ELSE, 7, 0, TOKEN_IF, TOKEN_OPEN_PAREN, PARSER_EXPRESSION, TOKEN_CLOSE_PAREN, PARSER_STATEMENT, TOKEN_ELSE, PARSER_STATEMENT},

	{PARSER_EXPRESSION, NODE_NONE, 1, 0, PARSER_ASSIGN_EXPRESSION},

	//{PARSER_ASSIGN_EXPRESSION, NODE_ASSIGN, 3, PARSER_LOCAL_INDEX_EXPRESSION, TOKEN_ASSIGN, PARSER_ADD_EXPRESSION},
	{PARSER_ASSIGN_EXPRESSION, NODE_CONCAT_SIMPLE_ASSIGN, 3, 1, PARSER_IDENTIFIER_EXPRESSION, TOKEN_CONCATE, PARSER_ASSIGN_EXPRESSION},
	{PARSER_ASSIGN_EXPRESSION, NODE_MUL_SIMPLE_ASSIGN, 3, 1, PARSER_IDENTIFIER_EXPRESSION, TOKEN_MULE, PARSER_ASSIGN_EXPRESSION},
	{PARSER_ASSIGN_EXPRESSION, NODE_DIV_SIMPLE_ASSIGN, 3, 1, PARSER_IDENTIFIER_EXPRESSION, TOKEN_DIVE, PARSER_ASSIGN_EXPRESSION},
	{PARSER_ASSIGN_EXPRESSION, NODE_MOD_SIMPLE_ASSIGN, 3, 1, PARSER_IDENTIFIER_EXPRESSION, TOKEN_MODE, PARSER_ASSIGN_EXPRESSION},
	{PARSER_ASSIGN_EXPRESSION, NODE_ADD_SIMPLE_ASSIGN, 3, 1, PARSER_IDENTIFIER_EXPRESSION, TOKEN_ADDE, PARSER_ASSIGN_EXPRESSION},
	{PARSER_ASSIGN_EXPRESSION, NODE_SUB_SIMPLE_ASSIGN, 3, 1, PARSER_IDENTIFIER_EXPRESSION, TOKEN_SUBE, PARSER_ASSIGN_EXPRESSION},
	{PARSER_ASSIGN_EXPRESSION, NODE_AND_SIMPLE_ASSIGN, 3, 1, PARSER_IDENTIFIER_EXPRESSION, TOKEN_ANDE, PARSER_ASSIGN_EXPRESSION},
	{PARSER_ASSIGN_EXPRESSION, NODE_XOR_SIMPLE_ASSIGN, 3, 1, PARSER_IDENTIFIER_EXPRESSION, TOKEN_XORE, PARSER_ASSIGN_EXPRESSION},
	{PARSER_ASSIGN_EXPRESSION, NODE_OR_SIMPLE_ASSIGN, 3, 1, PARSER_IDENTIFIER_EXPRESSION, TOKEN_ORE, PARSER_ASSIGN_EXPRESSION},
	{PARSER_ASSIGN_EXPRESSION, NODE_LSHIFT_SIMPLE_ASSIGN, 3, 1, PARSER_IDENTIFIER_EXPRESSION, TOKEN_LSHIFTE, PARSER_ASSIGN_EXPRESSION},
	{PARSER_ASSIGN_EXPRESSION, NODE_RSHIFT_SIMPLE_ASSIGN, 3, 1, PARSER_IDENTIFIER_EXPRESSION, TOKEN_RSHIFTE, PARSER_ASSIGN_EXPRESSION},

	{PARSER_ASSIGN_EXPRESSION, NODE_CONCAT_ELEMENT_ASSIGN, 6, 4, PARSER_DEREFERENCE_EXPRESSION, TOKEN_OPEN_BRACKET, PARSER_EXPRESSION, TOKEN_CLOSE_BRACKET, TOKEN_CONCATE, PARSER_ASSIGN_EXPRESSION},
	{PARSER_ASSIGN_EXPRESSION, NODE_MUL_ELEMENT_ASSIGN, 6, 4, PARSER_DEREFERENCE_EXPRESSION, TOKEN_OPEN_BRACKET, PARSER_EXPRESSION, TOKEN_CLOSE_BRACKET, TOKEN_MULE, PARSER_ASSIGN_EXPRESSION},
	{PARSER_ASSIGN_EXPRESSION, NODE_DIV_ELEMENT_ASSIGN, 6, 4, PARSER_DEREFERENCE_EXPRESSION, TOKEN_OPEN_BRACKET, PARSER_EXPRESSION, TOKEN_CLOSE_BRACKET, TOKEN_DIVE, PARSER_ASSIGN_EXPRESSION},
	{PARSER_ASSIGN_EXPRESSION, NODE_MOD_ELEMENT_ASSIGN, 6, 4, PARSER_DEREFERENCE_EXPRESSION, TOKEN_OPEN_BRACKET, PARSER_EXPRESSION, TOKEN_CLOSE_BRACKET, TOKEN_MODE, PARSER_ASSIGN_EXPRESSION},
	{PARSER_ASSIGN_EXPRESSION, NODE_ADD_ELEMENT_ASSIGN, 6, 4, PARSER_DEREFERENCE_EXPRESSION, TOKEN_OPEN_BRACKET, PARSER_EXPRESSION, TOKEN_CLOSE_BRACKET, TOKEN_ADDE, PARSER_ASSIGN_EXPRESSION},
	{PARSER_ASSIGN_EXPRESSION, NODE_SUB_ELEMENT_ASSIGN, 6, 4, PARSER_DEREFERENCE_EXPRESSION, TOKEN_OPEN_BRACKET, PARSER_EXPRESSION, TOKEN_CLOSE_BRACKET, TOKEN_SUBE, PARSER_ASSIGN_EXPRESSION},
	{PARSER_ASSIGN_EXPRESSION, NODE_AND_ELEMENT_ASSIGN, 6, 4, PARSER_DEREFERENCE_EXPRESSION, TOKEN_OPEN_BRACKET, PARSER_EXPRESSION, TOKEN_CLOSE_BRACKET, TOKEN_ANDE, PARSER_ASSIGN_EXPRESSION},
	{PARSER_ASSIGN_EXPRESSION, NODE_XOR_ELEMENT_ASSIGN, 6, 4, PARSER_DEREFERENCE_EXPRESSION, TOKEN_OPEN_BRACKET, PARSER_EXPRESSION, TOKEN_CLOSE_BRACKET, TOKEN_XORE, PARSER_ASSIGN_EXPRESSION},
	{PARSER_ASSIGN_EXPRESSION, NODE_OR_ELEMENT_ASSIGN, 6, 4, PARSER_DEREFERENCE_EXPRESSION, TOKEN_OPEN_BRACKET, PARSER_EXPRESSION, TOKEN_CLOSE_BRACKET, TOKEN_ORE, PARSER_ASSIGN_EXPRESSION},
	{PARSER_ASSIGN_EXPRESSION, NODE_LSHIFT_ELEMENT_ASSIGN, 6, 4, PARSER_DEREFERENCE_EXPRESSION, TOKEN_OPEN_BRACKET, PARSER_EXPRESSION, TOKEN_CLOSE_BRACKET, TOKEN_LSHIFTE, PARSER_ASSIGN_EXPRESSION},
	{PARSER_ASSIGN_EXPRESSION, NODE_RSHIFT_ELEMENT_ASSIGN, 6, 4, PARSER_DEREFERENCE_EXPRESSION, TOKEN_OPEN_BRACKET, PARSER_EXPRESSION, TOKEN_CLOSE_BRACKET, TOKEN_RSHIFTE, PARSER_ASSIGN_EXPRESSION},

	{PARSER_ASSIGN_EXPRESSION, NODE_SIMPLE_ASSIGN, 3, 1, PARSER_IDENTIFIER_EXPRESSION, TOKEN_ASSIGN, PARSER_ASSIGN_EXPRESSION},
	{PARSER_ASSIGN_EXPRESSION, NODE_ELEMENT_ASSIGN, 6, 4, PARSER_DEREFERENCE_EXPRESSION, TOKEN_OPEN_BRACKET, PARSER_EXPRESSION, TOKEN_CLOSE_BRACKET, TOKEN_ASSIGN, PARSER_ASSIGN_EXPRESSION},
	{PARSER_ASSIGN_EXPRESSION, NODE_NONE, 1, 0, PARSER_APPEND_EXPRESSION},

	{PARSER_APPEND_EXPRESSION, NODE_APPEND, 3, 1, PARSER_APPEND_EXPRESSION, TOKEN_APPEND, PARSER_BOOL_OR_EXPRESSION},
	{PARSER_APPEND_EXPRESSION, NODE_NONE, 1, 0, PARSER_BOOL_OR_EXPRESSION},

	{PARSER_BOOL_OR_EXPRESSION, NODE_BOOL_OR, 3, 1, PARSER_BOOL_OR_EXPRESSION, TOKEN_BOOL_OR, PARSER_BOOL_AND_EXPRESSION},
	{PARSER_BOOL_OR_EXPRESSION, NODE_NONE, 1, 0, PARSER_BOOL_AND_EXPRESSION},

	{PARSER_BOOL_AND_EXPRESSION, NODE_BOOL_AND, 3, 1, PARSER_BOOL_AND_EXPRESSION, TOKEN_BOOL_AND, PARSER_OR_EXPRESSION},
	{PARSER_BOOL_AND_EXPRESSION, NODE_NONE, 1, 0, PARSER_OR_EXPRESSION},

	{PARSER_OR_EXPRESSION, NODE_OR, 3, 1, PARSER_OR_EXPRESSION, TOKEN_OR, PARSER_XOR_EXPRESSION},
	{PARSER_OR_EXPRESSION, NODE_NONE, 1, 0, PARSER_XOR_EXPRESSION},

	{PARSER_XOR_EXPRESSION, NODE_XOR, 3, 1, PARSER_XOR_EXPRESSION, TOKEN_XOR, PARSER_AND_EXPRESSION},
	{PARSER_XOR_EXPRESSION, NODE_NONE, 1, 0, PARSER_AND_EXPRESSION},

	{PARSER_AND_EXPRESSION, NODE_AND, 3, 1, PARSER_AND_EXPRESSION, TOKEN_AND, PARSER_EQUALS_EXPRESSION},
	{PARSER_AND_EXPRESSION, NODE_NONE, 1, 0, PARSER_EQUALS_EXPRESSION},


	{PARSER_EQUALS_EXPRESSION, NODE_E, 3, 1, PARSER_EQUALS_EXPRESSION, TOKEN_E, PARSER_COMPARE_EXPRESSION},
	{PARSER_EQUALS_EXPRESSION, NODE_NE, 3, 1, PARSER_EQUALS_EXPRESSION, TOKEN_NE, PARSER_COMPARE_EXPRESSION},
	{PARSER_EQUALS_EXPRESSION, NODE_NONE, 1, 0, PARSER_COMPARE_EXPRESSION},

	{PARSER_COMPARE_EXPRESSION, NODE_L, 3, 1, PARSER_COMPARE_EXPRESSION, TOKEN_L, PARSER_COMPARE_STRING_EXPRESSION},
	{PARSER_COMPARE_EXPRESSION, NODE_LE, 3, 1, PARSER_COMPARE_EXPRESSION, TOKEN_LE, PARSER_COMPARE_STRING_EXPRESSION},
	{PARSER_COMPARE_EXPRESSION, NODE_G, 3, 1, PARSER_COMPARE_EXPRESSION, TOKEN_G, PARSER_COMPARE_STRING_EXPRESSION},
	{PARSER_COMPARE_EXPRESSION, NODE_GE, 3, 1, PARSER_COMPARE_EXPRESSION, TOKEN_GE, PARSER_COMPARE_STRING_EXPRESSION},
	{PARSER_COMPARE_EXPRESSION, NODE_NONE, 1, 0, PARSER_COMPARE_STRING_EXPRESSION},

	{PARSER_COMPARE_STRING_EXPRESSION, NODE_SCI_E, 3, 1, PARSER_COMPARE_STRING_EXPRESSION, TOKEN_SCIE, PARSER_CONCAT_EXPRESSION},
	{PARSER_COMPARE_STRING_EXPRESSION, NODE_SCI_NE, 3, 1, PARSER_COMPARE_STRING_EXPRESSION, TOKEN_SCINE, PARSER_CONCAT_EXPRESSION},
	{PARSER_COMPARE_STRING_EXPRESSION, NODE_SCI_L, 3, 1, PARSER_COMPARE_STRING_EXPRESSION, TOKEN_SCIL, PARSER_CONCAT_EXPRESSION},
	{PARSER_COMPARE_STRING_EXPRESSION, NODE_SCI_LE, 3, 1, PARSER_COMPARE_STRING_EXPRESSION, TOKEN_SCILE, PARSER_CONCAT_EXPRESSION},
	{PARSER_COMPARE_STRING_EXPRESSION, NODE_SCI_G, 3, 1, PARSER_COMPARE_STRING_EXPRESSION, TOKEN_SCIG, PARSER_CONCAT_EXPRESSION},
	{PARSER_COMPARE_STRING_EXPRESSION, NODE_SCI_GE, 3, 1, PARSER_COMPARE_STRING_EXPRESSION, TOKEN_SCIGE, PARSER_CONCAT_EXPRESSION},
	{PARSER_COMPARE_STRING_EXPRESSION, NODE_S_E, 3, 1, PARSER_COMPARE_STRING_EXPRESSION, TOKEN_SE, PARSER_CONCAT_EXPRESSION},
	{PARSER_COMPARE_STRING_EXPRESSION, NODE_S_NE, 3, 1, PARSER_COMPARE_STRING_EXPRESSION, TOKEN_SNE, PARSER_CONCAT_EXPRESSION},
	{PARSER_COMPARE_STRING_EXPRESSION, NODE_S_L, 3, 1, PARSER_COMPARE_STRING_EXPRESSION, TOKEN_SL, PARSER_CONCAT_EXPRESSION},
	{PARSER_COMPARE_STRING_EXPRESSION, NODE_S_LE, 3, 1, PARSER_COMPARE_STRING_EXPRESSION, TOKEN_SLE, PARSER_CONCAT_EXPRESSION},
	{PARSER_COMPARE_STRING_EXPRESSION, NODE_S_G, 3, 1, PARSER_COMPARE_STRING_EXPRESSION, TOKEN_SG, PARSER_CONCAT_EXPRESSION},
	{PARSER_COMPARE_STRING_EXPRESSION, NODE_S_GE, 3, 1, PARSER_COMPARE_STRING_EXPRESSION, TOKEN_SGE, PARSER_CONCAT_EXPRESSION},
	{PARSER_COMPARE_STRING_EXPRESSION, NODE_NONE, 1, 0, PARSER_CONCAT_EXPRESSION},

	{PARSER_CONCAT_EXPRESSION, NODE_CONCAT_STRING, 3, 1, PARSER_CONCAT_EXPRESSION, TOKEN_CONCAT, PARSER_SHIFT_EXPRESSION},
	{PARSER_CONCAT_EXPRESSION, NODE_NONE, 1, 0, PARSER_SHIFT_EXPRESSION},

	{PARSER_SHIFT_EXPRESSION, NODE_LSHIFT, 3, 1, PARSER_SHIFT_EXPRESSION, TOKEN_LSHIFT, PARSER_ADD_EXPRESSION},
	{PARSER_SHIFT_EXPRESSION, NODE_RSHIFT, 3, 1, PARSER_SHIFT_EXPRESSION, TOKEN_RSHIFT, PARSER_ADD_EXPRESSION},
	{PARSER_SHIFT_EXPRESSION, NODE_NONE, 1, 0, PARSER_ADD_EXPRESSION},

	{PARSER_ADD_EXPRESSION, NODE_ADD, 3, 1, PARSER_ADD_EXPRESSION, TOKEN_ADD, PARSER_MUL_EXPRESSION},
	{PARSER_ADD_EXPRESSION, NODE_SUB, 3, 1, PARSER_ADD_EXPRESSION, TOKEN_SUB, PARSER_MUL_EXPRESSION},
	{PARSER_ADD_EXPRESSION, NODE_NONE, 1, 0, PARSER_MUL_EXPRESSION},

	{PARSER_MUL_EXPRESSION, NODE_MUL, 3, 1, PARSER_MUL_EXPRESSION, TOKEN_MUL, PARSER_PREFIX_EXPRESSION},
	{PARSER_MUL_EXPRESSION, NODE_MOD, 3, 1, PARSER_MUL_EXPRESSION, TOKEN_MOD, PARSER_PREFIX_EXPRESSION},
	{PARSER_MUL_EXPRESSION, NODE_DIV, 3, 1, PARSER_MUL_EXPRESSION, TOKEN_DIV, PARSER_PREFIX_EXPRESSION},
	{PARSER_MUL_EXPRESSION, NODE_NONE, 1, 0, PARSER_PREFIX_EXPRESSION},

	{PARSER_PREFIX_EXPRESSION, NODE_NOT, 2, 0, TOKEN_NOT, PARSER_PREFIX_EXPRESSION},
	{PARSER_PREFIX_EXPRESSION, NODE_NEG, 2, 0, TOKEN_SUB, PARSER_PREFIX_EXPRESSION},
	{PARSER_PREFIX_EXPRESSION, NODE_BIN_NEG, 2, 0, TOKEN_TILDE, PARSER_PREFIX_EXPRESSION},
	{PARSER_PREFIX_EXPRESSION, NODE_NONE, 2, 0, TOKEN_ADD, PARSER_PREFIX_EXPRESSION},
	{PARSER_PREFIX_EXPRESSION, NODE_NONE, 1, 0, PARSER_SIMPLE_EXPRESSION},

	//{PARSER_SIMPLE_EXPRESSION, NODE_NONE, 1, PARSER_FUNCTION_CALL},
	{PARSER_SIMPLE_EXPRESSION, NODE_INT, 1, 0, TOKEN_INT},
	{PARSER_SIMPLE_EXPRESSION, NODE_DOUBLE, 1, 0, TOKEN_DOUBLE},
	//{PARSER_SIMPLE_EXPRESSION, NODE_IDENTIFIER, 1, TOKEN_IDENTIFIER},
	{PARSER_SIMPLE_EXPRESSION, NODE_STRING, 1, 0, TOKEN_STRING},
	{PARSER_SIMPLE_EXPRESSION, NODE_NONE, 3, 1, TOKEN_OPEN_PAREN, PARSER_EXPRESSION, TOKEN_CLOSE_PAREN},

	{PARSER_SIMPLE_EXPRESSION, NODE_NONE, 1, 0, PARSER_SIMPLE_POSTMOD_EXPRESSION},
	//{PARSER_SIMPLE_EXPRESSION2, NODE_NONE, 1, 0, PARSER_SIMPLE_EXPRESSION},

	{PARSER_SIMPLE_POSTMOD_EXPRESSION, NODE_NONE, 1, 0, PARSER_SIMPLE_PREMOD_EXPRESSION},
	{PARSER_SIMPLE_POSTMOD_EXPRESSION, NODE_SIMPLE_POST_INC, 2, 1, PARSER_SIMPLE_POSTMOD_EXPRESSION, TOKEN_INC},
	{PARSER_SIMPLE_POSTMOD_EXPRESSION, NODE_SIMPLE_POST_DEC, 2, 1, PARSER_SIMPLE_POSTMOD_EXPRESSION, TOKEN_DEC},

	{PARSER_SIMPLE_EXPRESSION, NODE_NONE, 1, 0, PARSER_SIMPLE_DEREFERENCE_EXPRESSION},
	{PARSER_DEREFERENCE_EXPRESSION, NODE_NONE, 1, 0, PARSER_SIMPLE_DEREFERENCE_EXPRESSION},
	{PARSER_DEREFERENCE_EXPRESSION, NODE_DEREFERENCE, 1, 0, PARSER_IDENTIFIER_EXPRESSION},
	{PARSER_DEREFERENCE_EXPRESSION, NODE_SUB_REFERENCE, 4, 1, PARSER_DEREFERENCE_EXPRESSION, TOKEN_OPEN_BRACKET, PARSER_EXPRESSION, TOKEN_CLOSE_BRACKET},

	{PARSER_SIMPLE_PREMOD_EXPRESSION, NODE_DEREFERENCE, 1, 0, PARSER_IDENTIFIER_EXPRESSION},
	{PARSER_SIMPLE_PREMOD_EXPRESSION, NODE_SIMPLE_PRE_INC, 2, 0, TOKEN_INC, PARSER_SIMPLE_PREMOD_EXPRESSION},
	{PARSER_SIMPLE_PREMOD_EXPRESSION, NODE_SIMPLE_PRE_DEC, 2, 0, TOKEN_DEC, PARSER_SIMPLE_PREMOD_EXPRESSION},

	//{PARSER_SIMPLE_EXPRESSION, NODE_DEREFERENCE, 1, PARSER_IDENTIFIER_EXPRESSION},

	//{PARSER_INDEX_EXPRESSION, NODE_SUB_REFERENCE, 4, PARSER_INDEX_EXPRESSION, TOKEN_OPEN_BRACKET, PARSER_EXPRESSION, TOKEN_CLOSE_BRACKET},
	//{PARSER_SIMPLE_EXPRESSION, NODE_SUB_REFERENCE, 4, PARSER_SIMPLE_EXPRESSION, TOKEN_OPEN_BRACKET, PARSER_EXPRESSION, TOKEN_CLOSE_BRACKET},


	//{PARSER_SIMPLE_EXPRESSION, NODE_SUB_REFERENCE, 4, PARSER_SIMPLE_EXPRESSION, TOKEN_OPEN_BRACKET, PARSER_EXPRESSION, TOKEN_CLOSE_BRACKET},

	//{PARSER_ELEMENT_PREMOD_IDENTIFIER, NODE_NONE, 1, PARSER_SIMPLE_EXPRESSION},
	//{PARSER_ELEMENT_PREPREMOD_EXPRESSION, NODE_SUB_REFERENCE, 4, PARSER_ELEMENT_PREPREMOD_EXPRESSION, TOKEN_OPEN_BRACKET, PARSER_EXPRESSION, TOKEN_CLOSE_BRACKET},
	//{PARSER_ELEMENT_PREPREMOD_EXPRESSION, NODE_SUB_REFERENCE, 4, PARSER_ELEMENT_PREMOD_IDENTIFIER, TOKEN_OPEN_BRACKET, PARSER_EXPRESSION, TOKEN_CLOSE_BRACKET},
	//{PARSER_ELEMENT_PREMOD_IDENTIFIER, NODE_DEREFERENCE, 1, PARSER_IDENTIFIER_EXPRESSION},


	{PARSER_ELEMENT_PREMOD_EXPRESSION, NODE_SUB_REFERENCE, 4, 1, PARSER_DEREFERENCE_EXPRESSION, TOKEN_OPEN_BRACKET, PARSER_EXPRESSION, TOKEN_CLOSE_BRACKET},
	{PARSER_ELEMENT_PREMOD_EXPRESSION, NODE_ELEMENT_PRE_INC, 2, 0, TOKEN_INC, PARSER_ELEMENT_PREMOD_EXPRESSION},
	{PARSER_ELEMENT_PREMOD_EXPRESSION, NODE_ELEMENT_PRE_DEC, 2, 0, TOKEN_DEC, PARSER_ELEMENT_PREMOD_EXPRESSION},

	{PARSER_ELEMENT_POSTMOD_EXPRESSION, NODE_NONE, 1, 0, PARSER_ELEMENT_PREMOD_EXPRESSION},
	{PARSER_ELEMENT_POSTMOD_EXPRESSION, NODE_ELEMENT_POST_INC, 2, 1, PARSER_ELEMENT_POSTMOD_EXPRESSION, TOKEN_INC},
	{PARSER_ELEMENT_POSTMOD_EXPRESSION, NODE_ELEMENT_POST_DEC, 2, 1, PARSER_ELEMENT_POSTMOD_EXPRESSION, TOKEN_DEC},

	{PARSER_SIMPLE_EXPRESSION, NODE_NONE, 1, 0, PARSER_ELEMENT_POSTMOD_EXPRESSION},
	//{PARSER_ELEMENT_PREMOD_EXPRESSION, NODE_NONE, 1, PARSER_ELEMENT_PREPREMOD_EXPRESSION},
	//{PARSER_ELEMENT_PREMOD_EXPRESSION, NODE_SUB_REFERENCE, 4, PARSER_SIMPLE_EXPRESSION, TOKEN_OPEN_BRACKET, PARSER_EXPRESSION, TOKEN_CLOSE_BRACKET},

	//{SIMPLE_EXPRESSION, NODE_DEREFERENCE, 4, PARSER_IDENTIFIER_EXPRESSION, TOKEN_OPEN_BRACKET, PARSER_EXPRESSION, TOKEN_CLOSE_BRACKET},
	{PARSER_IDENTIFIER_EXPRESSION, NODE_STRUCT_REFERENCE, 3, 2, PARSER_DEREFERENCE_EXPRESSION, TOKEN_PERIOD, TOKEN_IDENTIFIER},
	{PARSER_IDENTIFIER_EXPRESSION, NODE_STRUCT_REFERENCE, 4, 2, PARSER_DEREFERENCE_EXPRESSION, TOKEN_PERIOD, TOKEN_MOD, TOKEN_IDENTIFIER},

	//{PARSER_IDENTIFIER_EXPRESSION, NODE_STRUCT_REFERENCE, 3, 2, PARSER_DEREFERENCE_EXPRESSION, TOKEN_PERIOD, TOKEN_IDENTIFIER},

	{PARSER_IDENTIFIER_EXPRESSION, NODE_STRUCT_THIS_REFERENCE, 2, 0, TOKEN_MOD, TOKEN_IDENTIFIER},
	{PARSER_IDENTIFIER_EXPRESSION, NODE_LOCAL_REFERENCE, 2, 0, TOKEN_DOLLAR, TOKEN_IDENTIFIER},
	{PARSER_IDENTIFIER_EXPRESSION, NODE_GLOBAL_REFERENCE, 1, 0, TOKEN_IDENTIFIER},
	//{PARSER_INDEX_EXPRESSION, NODE_REFERENCE, 4, PARSER_INDEX_EXPRESSION, TOKEN_OPEN_BRACKET, PARSER_EXPRESSION, TOKEN_CLOSE_BRACKET},
	//*/
};

#define NUM_RULES (sizeof(rules)/sizeof(Rule))

struct Item {
	int transition;
	int rule;
	int position;
};

struct ItemSet {
	int numItems;
	Item items[1];
};

int CalcClosure(ItemSet **s, int *mem, int id) {
	int k = s[0]->numItems;
	int maxSize = k;
	int numCare = k;
	for (int j=0; j<numCare; j++) {
		int position = s[0]->items[j].position;
		const Rule *rule = &rules[s[0]->items[j].rule];
		if (position >= rule->numTerms) continue;
		int term = rule->terms[position];
		for (int i=1; i<NUM_RULES; i++) {
			if (term != rules[i].result)
				continue;
			if (mem[i] == id)
				break;
			if (k < maxSize || (srealloc(s[0], sizeof(ItemSet)+sizeof(Item)*(maxSize+16)) && (maxSize+=16))) {
				mem[i] = id;
				int q;
				for (q = 0; q<numCare; q++) {
					if (rules[s[0]->items[q].rule].terms[s[0]->items[q].position] == rules[i].terms[0]) {
						break;
					}
				}
				int out = k;
				if (q == numCare && rules[i].terms[0] > TOKEN_END) {
					s[0]->items[k] = s[0]->items[numCare];
					out = numCare++;
				}
				s[0]->items[out].rule = i;
				s[0]->items[out].position = 0;
				s[0]->items[out].transition = -1;
				s[0]->numItems = ++k;
			}
		}
	}
	return 1;
}

void CleanupTree(ParseTree *t) {
	if (t->val) free(t->val);
	for (int i=0; i<t->numChildren; i++) {
		if (t->children[i])
			CleanupTree(t->children[i]);
	}
	free(t);
}

void ParseError(Token *token) {
	errorPrintf(2, "Parse error, line %i:\r\n", token->line);
	errors++;
	DisplayErrorLine(token->start, token->pos);
}

int *actionTable = 0;

ParseTree * ParseScript(Token *tokens, int numTokens) {
	int i;
	if (numTokens == 1) {
		CleanupTokens(tokens, numTokens);
		return 0;
	}
	if (!actionTable) {
		int numSets = 0;
		ItemSet **set = (ItemSet**) malloc(sizeof(ItemSet*)*1000);
		ItemSet **newSets = (ItemSet**) malloc(sizeof(ItemSet*)*1000);
		if (!set || !newSets) {
			free(set);
			free(newSets);
			return 0;
		}
		int changed = 1;
		int *followSets = (int*) calloc(1, PARSER_NONE * sizeof(int) * PARSER_NONE);
		int *firstSets = (int*) calloc(1, PARSER_NONE * sizeof(int) * PARSER_NONE);
		for (i=0; i<PARSER_SCRIPT; i++) {
			firstSets[PARSER_NONE*i+i] = 1;
		}
		while (changed) {
			changed = 0;
			for (int r = 0; r<NUM_RULES; r++) {
				int R = rules[r].result;
				int F = rules[r].terms[0];
				for (i=0; i<PARSER_NONE;i++) {
					if (firstSets[F*PARSER_NONE+i] && !firstSets[R*PARSER_NONE+i]) {
						firstSets[R*PARSER_NONE+i] = 1;
						changed = 1;
					}
				}
			}
		}
		changed = 1;
		while (changed) {
			changed = 0;
			for (int r = 0; r<NUM_RULES; r++) {
				for (int p=0; p<rules[r].numTerms-1; p++) {
					ScriptTokenType F = rules[r].terms[p];
					ScriptTokenType S = rules[r].terms[p+1];
					for (i=0; i<PARSER_NONE;i++) {
						if (firstSets[S*PARSER_NONE+i] && !followSets[F*PARSER_NONE+i]) {
							followSets[F*PARSER_NONE+i] = 1;
							changed = 1;
						}
					}
				}
				ScriptTokenType F = rules[r].terms[rules[r].numTerms-1];
				ScriptTokenType S = rules[r].result;
				for (i=0; i<PARSER_NONE;i++) {
					if (followSets[S*PARSER_NONE+i] && !followSets[F*PARSER_NONE+i]) {
						followSets[F*PARSER_NONE+i] = 1;
						changed = 1;
					}
				}
			}
		}

		set[numSets++] = (ItemSet *) malloc(sizeof(ItemSet));
		set[0]->numItems = 1;
		set[0]->items[0].position = 0;
		set[0]->items[0].rule = 0;
		int closureId = 0;
		int closureNotes[sizeof(rules) / sizeof(Rule)];
		for (i=0; i<sizeof(closureNotes)/sizeof(int);i++)
			closureNotes[i] = -1;
		//memset(closureNotes, -1, sizeof(closureNotes));
		CalcClosure(&set[0], closureNotes, closureId++);

		for (i=0; i<numSets; i++) {
			int numNewSets = 0;
			int j;
			for (j=0; j<set[i]->numItems; j++) {
				set[i]->items[j].transition = -1;
				if (set[i]->items[j].position != rules[set[i]->items[j].rule].numTerms) {
					int q;
					for (q=0; q<numNewSets; q++) {
						if (rules[set[i]->items[j].rule].terms[set[i]->items[j].position] == rules[newSets[q]->items[0].rule].terms[newSets[q]->items[0].position-1]) {
							srealloc(newSets[q] , sizeof(ItemSet) + sizeof(Item) * newSets[q]->numItems);
							break;
						}
					}
					if (q==numNewSets) {
						newSets[q] = (ItemSet*) malloc(sizeof(ItemSet));
						newSets[q]->numItems = 0;
						numNewSets++;
					}
					newSets[q]->items[newSets[q]->numItems].position = set[i]->items[j].position+1;
					newSets[q]->items[newSets[q]->numItems].rule = set[i]->items[j].rule;
					newSets[q]->items[newSets[q]->numItems].transition = j;
					newSets[q]->numItems++;
				}
			}
			for (j=0; j<numNewSets; j++) {
				CalcClosure(&newSets[j], closureNotes, closureId++);
				int q = 0;
				for (q = 0; q<numSets; q++) {
					int match = 0;
					if (set[q]->numItems == newSets[j]->numItems)
					while (match < newSets[j]->numItems) {
						int f;
						for (f = 0; f<set[q]->numItems; f++) {
							if (set[q]->items[f].rule == newSets[j]->items[match].rule &&
								set[q]->items[f].position == newSets[j]->items[match].position)
								break;
						}
						if (f == set[q]->numItems) break;
						match++;
					}
					if (match == newSets[j]->numItems) break;
				}
				for (int w=0; w<newSets[j]->numItems; w++)
					if (newSets[j]->items[w].transition >= 0)
						set[i]->items[newSets[j]->items[w].transition].transition = q;
				if (q<numSets) {
					free(newSets[j]);
				}
				else {
					set[numSets++] = newSets[j];
				}
			}
		}
		free(newSets);

		actionTable = (int*) malloc(sizeof(int) * numSets*PARSER_NONE);
		for (i=numSets*PARSER_NONE-1; i>=0;i--) {
			actionTable[i] = -1;
		}

		for (i=0; i<numSets; i++) {
			int reduce = 0;
			for (int w=0; w<set[i]->numItems; w++) {
				if (set[i]->items[w].position < rules[set[i]->items[w].rule].numTerms) {
					int term = rules[set[i]->items[w].rule].terms[set[i]->items[w].position];
					if (actionTable[i*PARSER_NONE+term]>=0&& (actionTable[i*PARSER_NONE+term]&0xFFFFFF)-set[i]->items[w].transition !=0) {
						i=i;
					}
					actionTable[i*PARSER_NONE+term] = set[i]->items[w].transition;
					if (term < PARSER_SCRIPT) {
						if (term != TOKEN_END)
							actionTable[i*PARSER_NONE+term] |= SHIFT;
						else
							actionTable[i*PARSER_NONE+term] = ACCEPT;
					}
					/*
					if (rules[set[i]->items[w].rule].terms[set[i]->items[w].position] < S) {
						actionTable[i*PARSER_NONE+set[i]->items[w].rule] = set[i]->items[w].transition;
						else 
							actionTable[i*PARSER_NONE+set[i]->items[w].rule] |= SHIFT;
					}//*/
				}
				else {
					/*if (reduce && reduce != set[i]->items[w].rule) {
						reduce = reduce;
					}
					reduce = set[i]->items[w].rule;
					//*/
					reduce = set[i]->items[w].rule;
					for (int x = 0; x<PARSER_SCRIPT; x++) {
						if (followSets[rules[reduce].result*PARSER_NONE+x]) {
							if (actionTable[x+i*PARSER_NONE] == -1) {
								actionTable[x+i*PARSER_NONE] = REDUCE | reduce;
							}
							else {
								i=i;
							}
						}
					}
				}
			}
			/*if (reduce) {
				for (int w = 0; w<PARSER_SCRIPT; w++) {
					if (actionTable[w+i*PARSER_NONE] == -1) {
						actionTable[w+i*PARSER_NONE] = REDUCE | reduce;
					}
				}
			}*/
		}
		free(followSets);
		free(firstSets);
		for (i=0; i<numSets; i++) {
			free(set[i]);
		}
		free(set);
	}

	int maxStackSize = 1000;
	int *stack = (int*) malloc(sizeof(int) * 1000);
	//int output[1000];
	stack[0] = 0;
	//int outputLen = 0;
	int stackSize = 1;
	int pos = 0;
	int accept = 0;
	int ParseTrees = 0;

	int maxParseTrees = 1000;
	ParseTree **tree = (ParseTree**) malloc(sizeof(ParseTree*) * 1000);
	int error = 0;
	while (pos < numTokens && stackSize > 0) {
		int row = stack[stackSize-1];
		ScriptTokenType token = tokens[pos].type;
		int i = actionTable[token+row*PARSER_NONE];
		if (i==-1) {
			if (pos && !tokens[pos].line) {
				ParseError(tokens+pos-1);
			}
			else {
				ParseError(tokens+pos);
			}
			error = 1;
			break;
		}
		else if ((i&ACTION_MASK) == SHIFT) {
			i &= ~SHIFT;
			if (stackSize == maxStackSize) {
				if (!srealloc(stack, sizeof(int) * (maxStackSize*=2))) {
					ParseError(tokens+pos);
					error = 1;
					break;
				}
			}
			if (ParseTrees == maxParseTrees) {
				if (!srealloc(tree, sizeof(ParseTree*) * (maxParseTrees*=2))) {
					ParseError(tokens+pos);
					error = 1;
					break;
				}
			}
			stack[stackSize++] = i;
			tree[ParseTrees] = (ParseTree*) calloc(1, sizeof(ParseTree));
			if (!tree[ParseTrees]) {
				ParseError(tokens+pos);
				error = 1;
				break;
			}
			{
				tree[ParseTrees]->line = tokens[pos].line;
				tree[ParseTrees]->pos = tokens[pos].pos;
				tree[ParseTrees]->start = tokens[pos].start;
			}
			// Temporary node.  Will be eaten by another.
			tree[ParseTrees]->type = NODE_LEAF;
			tree[ParseTrees]->numChildren = 0;
			tree[ParseTrees]->intVal = tokens[pos].intVal;
			tree[ParseTrees++]->val = tokens[pos].val;
			tokens[pos].val = 0;

			pos++;
		}
		else if ((i&ACTION_MASK) == REDUCE) {
			i &= ~REDUCE;
			// output[outputLen++] = i;
			ParseTree *children[5] = {0,0,0,0,0};
			int numChildren = 0;
			int nline;
			unsigned char *npos;
			unsigned char *nstart;
			for (int j = 0; j<rules[i].numTerms; j++) {
				if (j == rules[i].operatorPos) {
					nline = tree[ParseTrees - rules[i].numTerms+j]->line;
					npos = tree[ParseTrees - rules[i].numTerms+j]->pos;
					nstart = tree[ParseTrees - rules[i].numTerms+j]->start;
				}
				if (rules[i].terms[j] <= TOKEN_IDENTIFIER || rules[i].terms[j]>=TOKEN_END) {
					children[numChildren++] = tree[ParseTrees - rules[i].numTerms+j];
				}
				else {
					free(tree[ParseTrees - rules[i].numTerms+j]);
					tree[ParseTrees - rules[i].numTerms+j] = 0;
				}
			}
			ParseTrees -= rules[i].numTerms;
			if (rules[i].nodeType == NODE_NONE) {
				tree[ParseTrees] = children[0];
			}
			else {
				if (numChildren && children[0]->type == NODE_LEAF) {
					tree[ParseTrees] = children[0];
				}
				else {
					tree[ParseTrees] = (ParseTree*) calloc(1, sizeof(ParseTree));
					if (!tree[ParseTrees]) {
						ParseError(tokens+pos);
						error = 1;
						break;
					}
					tree[ParseTrees]->children[0] = children[0];
				}
				tree[ParseTrees]->numChildren = numChildren;
				tree[ParseTrees]->children[1] = children[1];
				tree[ParseTrees]->children[2] = children[2];
				tree[ParseTrees]->children[3] = children[3];
				tree[ParseTrees]->children[4] = children[4];
				tree[ParseTrees]->type = rules[i].nodeType;
			}
			tree[ParseTrees]->line = nline;
			tree[ParseTrees]->pos = npos;
			tree[ParseTrees]->start = nstart;
			ParseTrees++;
			stackSize -= rules[i].numTerms;
			if (stackSize <= 0) {
				ParseError(tokens+pos);
				error = 1;
				break;
			}
			else {
				int w = rules[i].result;
				int t = actionTable[stack[stackSize-1]*PARSER_NONE + w];
				if (t >= 0 && t < 0x1000000) {
					stack[stackSize++] = t;
				}
				else {
					ParseError(tokens+pos);
					error = 1;
					break;
				}
			}
		}
		else if (i == ACCEPT) {
			accept = 1;
			break;
		}
		else {
			ParseError(tokens+pos);
			error = 1;
			break;
		}
	}


	free(stack);
	CleanupTokens(tokens, numTokens);
	if (error) {
		for (int i=0; i<ParseTrees; i++) {
			CleanupTree(tree[i]);
		}
		free(tree);
		return 0;
	}
	ParseTree *out = tree[0];
	free(tree);
	return out;
}

void CleanupParser() {
	free(actionTable);
	actionTable = 0;
}