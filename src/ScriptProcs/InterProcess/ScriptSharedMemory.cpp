#include "ScriptSharedMemory.h"
#include "../../unicode.h"
#include "../../global.h"

void GetSharedMemory(ScriptValue &s, ScriptValue *args) {
	HANDLE hSData = 0;
	//if (0 && args->stringVal->len <= 4000) {
	//}
	//else {
	wchar_t *name =  UTF8toUTF16Alloc(args->stringVal->value);
	if (name) {
		hSData=OpenFileMappingW(FILE_MAP_READ, FALSE, name);
		free(name);
	}
	//}

	if (hSData==0)
		return;

	MEMORY_BASIC_INFORMATION m;
	unsigned char *data;
	data = (unsigned char*)MapViewOfFile(hSData, FILE_MAP_READ, 0, 0, 0);
	if (data == 0) {
		CloseHandle(hSData);
		return;
	}

	if (VirtualQuery(data, &m, sizeof(m)) == sizeof(m) && m.RegionSize >= 0 && m.RegionSize <= (1<<30)) {
		CreateStringValue(s, data, (int)m.RegionSize);
	}

	UnmapViewOfFile(data);
	CloseHandle(hSData);
}
