#define _WINSOCKAPI_
#include <Windows.h>

#ifndef DLL_TYPES_H
#define DLL_TYPES_H

// Set if needs to be freed by LCD Misc using the dll's free function.
// Only valid for strings, lists, and dicts.

#define FLAG_NEED_FREE_DLL  1

// Don't touch this flag
#define FLAG_NEED_FREE_INTERNAL  2

// Corresponds to null.  All other values in the structure should be 0.
#define SCRIPT_NULL_DLL 0xFF

// 64-bit integer.  intValue is valid.
#define SCRIPT_INT 1

// 64-bit double.  doubleCalue is valid.
#define SCRIPT_DOUBLE 2

// utf8 string or binary data.  stringValue is valid.  These are what
// is used internally, so using them is faster, but it shouldn't matter
// much unless you're making a lot of dll calls and have a relatively short
// script.
#define SCRIPT_STRING 4

// list.  listValue is valid.
#define SCRIPT_LIST 8

// dictionary.  dictValue is valid.  Dicts are not sorted.
#define SCRIPT_DICT 16

// wide character string (utf16).  wideStringValue is valid.  Must
// contain no nulls and must be null terminated.
#define SCRIPT_WIDE_STRING 32

// ascii character string.  asciiStringValue is valid.  Must
// contain no nulls and must be null terminated.
#define SCRIPT_ASCII_STRING 64

struct ScriptVal;
struct ScriptDictEntry;

struct ScriptList {
	int numVals;
	ScriptVal *vals;
	int PushBack(ScriptVal *value);
};

// UTF8 string or binary data.  Must be null terminated, though len
// should not count that null.
struct ScriptString {
	int len;
	unsigned char *value;
};

// ASCII string.  Must be null terminated, though len  should not count
// that null.
struct ScriptAsciiString {
	int len;
	char *value;
};

// UTF16 string.  Must be null terminated, though len  should not count
// that null.
struct ScriptWideString {
	int len;
	wchar_t *value;
};


// Dictionary.  Note:  Duplicate entries will not appear from dictionaries
// received from a script, but they may appear in dictionaries passed back
// to the script.  Later entries will overwrite earlier ones.  The implementation
// of Add takes advantage of this fact instead of dealing with multiple string
// types itself.
struct ScriptDict {
	int numEntries;
	ScriptDictEntry *entries;
	int Add(ScriptVal *key, ScriptVal *val);
};

// Note:  You can use ScriptVals that are received as arguments as output values.
// However: You must copy them exactly.  You must not modify lists, strings, or dicts
// and type, flags, and both reserved values must be exactly the same (Even the
// FLAG_NEED_FREE_INTERNAL flag.  It will not result in calling free a second time).
// In future versions, using arguments this way may save some CPU time.

// Note:  This is not how values are represented internally.  It's quite similar, but a bit
// simpler (no reference count.  Dicts/lists use two pointers, etc).

// Strings of all types should be null terminated (that is, value[len] should be 0).
// only UTF8 strings (SCRIPT_STRING) can contain binary data.  The other types should
// onle contain a single string, with no null values.

struct ScriptVal {
	unsigned char type;
	// Must be 0.  May eventually be used for more types, if needed.
	unsigned char reserved1;

	unsigned char flags;
	// Must be 0.  May eventually be used for more flags, if needed.
	unsigned char reserved2;
	union {
		ScriptDict dictValue;
		ScriptList listValue;
		ScriptString stringValue;
		ScriptAsciiString asciiStringValue;
		ScriptWideString wideStringValue;
		double doubleValue;
		__int64 intValue;
	};
};

struct ScriptDictEntry {
	ScriptVal key;
	ScriptVal value;
};


void MakeStaticStringValue(ScriptVal *s, wchar_t *value);
int MakeStringValue(ScriptVal *s, wchar_t *value);
void MakeStaticStringValue(ScriptVal *s, char *value);
int MakeStringValue(ScriptVal *s, char *value);
void MakeStaticStringValue(ScriptVal *s, unsigned char *value);
int MakeStringValue(ScriptVal *s, unsigned char *value);
void MakeIntValue(ScriptVal *s, __int64 value);
void MakeDoubleValue(ScriptVal *s, double value);
void MakeDictValue(ScriptVal *s);



typedef void (_cdecl* FreeProc) (void *);
typedef void (CALLBACK* DllProc) (ScriptVal *in, ScriptVal *out);

struct AppInfo {
	// sizeof(DataPassedToDll).  May increase in future versions.
	int size;
	// LCD Miscellany version.  0.2.9 rev 14 would be {0,2,9,14}.
	unsigned char version[4];
	// Updated whenever dll specs change significantly.
	// I intend to try and keep things backward compatible.
	int maxDllVersion;
};

struct DllInfo {
	int size;

	// Version that this dll is using.  Must be between 1 and AppInfo.dllVersion or
	// dll will not be used.
	int dllVersion;

	// Used on lists/strings/dicts when NEED_FREE bit is set.  Can be null if bit
	// is never set.  Allows dll to be compiled with a different version of MSVC.
	FreeProc free;
};

typedef void (CALLBACK* InitDll) (AppInfo *in, DllInfo *out);

#endif
