#include "File.h"
#include "../unicode.h"
#include "../global.h"
#include <malloc.h>

void MyMoveFile(ScriptValue &s, ScriptValue *args) {
	wchar_t *from = UTF8toUTF16Alloc(args[0].stringVal->value);
	wchar_t *to = UTF8toUTF16Alloc(args[1].stringVal->value);
	if (from && to) {
		CreateIntValue(s, MoveFileExW(from, to, MOVEFILE_COPY_ALLOWED));
	}
	free(from);
	free(to);
}

void MyCopyFile(ScriptValue &s, ScriptValue *args) {
	wchar_t *from = UTF8toUTF16Alloc(args[0].stringVal->value);
	wchar_t *to = UTF8toUTF16Alloc(args[1].stringVal->value);
	if (from && to) {
		CreateIntValue(s, CopyFileW(from, to, 1));
	}
	free(from);
	free(to);
}

void MyDeleteFile(ScriptValue &s, ScriptValue *args) {
	wchar_t *file = UTF8toUTF16Alloc(args[0].stringVal->value);
	if (file) {
		if (DeleteFileW(file) || RemoveDirectoryW(file)) {
			CreateIntValue(s, 1);
		}
		free(file);
	}
}

