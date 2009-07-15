#include "..\\Script.h"
#include "..\\SymbolTable.h"

#include "ScriptDrawing.h"


#include "Networking/ScriptEmail.h"
#include "Networking/ScriptIP.h"

#include "ScriptMisc.h"
#include "File.h"

#include "Time/ScriptTime.h"
#include "Time/ScriptTimer.h"
#include "Time/ScriptWait.h"

#include "Util/ScriptTypes.h"
#include "Util/ScriptStringParser.h"
#include "Util/RegExp.h"
#include "Util/BencodeScript.h"
#include "Util/Math.h"
#include "Util/List.h"

#include "InterProcess/ScriptWindowManagement.h"
#include "InterProcess/DDE.h"


#include "InterProcess/ScriptProcessManagement.h"
#include "InterProcess/ScriptSharedMemory.h"
#include "InterProcess/ScriptDll.h"

//#include "InterProcess/Eval.h"

#include "Interface/MouseScript.h"
#include "Interface/MonitorScript.h"

#include "G15/G15Script.h"

#include "Event/Event.h"
#include "Event/Keyboard.h"

#include "Audio.h"

#pragma pack(push, 1)
struct ProcDescription {
	char *name;
	ScriptProc *proc;
	FunctionType type;
};
#pragma pack(pop)

int RegisterProcs2() {
	// Saves a little memory doing it this way.
	const static ProcDescription desc[] = {
		// Misc
		// *Must* be first.
		{"null", _null, C_direct},
		// *Must* be second.
		{"list", list, C_direct},


		{"Quit", Quit, C_noArgs},

		{"GetVram", GetVram, C_noArgs},
		{"CleanupDdraw", CleanupDdraw, C_noArgs},

		//{"GetSpeedFanData", GetSpeedFanData, C_noArgs},
		{"GetUsedMemory", GetUsedMemory, C_noArgs},
		{"GetTotalMemory", GetTotalMemory, C_noArgs},

		{"GetMemoryStatus", GetMemoryStatus, C_noArgs},

		{"FormatValue", FormatValue, C_double2ints},
		{"FormatSize", FormatSize, C_double3ints},
		//{"GetVolume", GetVolume, C_noArgs},
		{"GetDiskSpaceTotal", GetDiskSpaceTotal, C_string},
		{"GetDiskSpaceFree", GetDiskSpaceFree, C_string},

		{"GetDiskSpace", GetDiskSpace, C_string},
		//{"SetMenuOrder", SetMenuOrder, C_direct},
		//{"Alert", Alert, C_string},
		{"StartScreenSaver", StartScreenSaver, C_noArgs},
		{"MontorPower", MontorPower, C_int},
		{"UnloadFraps", UnloadFraps, C_noArgs},
		{"GetFrapsData", GetFrapsData, C_noArgs},

		{"GetVersion", GetVersion, C_noArgs},
		{"GetVersionString", GetVersionString, C_int},

		{"Suspend", Suspend, C_int},
		// Really only 3
		{"Shutdown", Shutdown, C_4ints},
		{"AbortShutdown", AbortShutdown, C_noArgs},
		{"LockSystem", LockSystem, C_noArgs},

		{"WriteLog", WriteLog, C_stringint},
		{"WriteLogLn", WriteLogLn, C_stringint},

		// Email
		{"GetEmailCount", GetEmailCount, C_noArgs},

		// Drawing
		{"ClearScreen", ClearScreen, C_noArgs},

		{"ClearRect", ClearRect, C_4ints},
		{"InvertRect", InvertRect, C_4ints},
		{"DrawRect", DrawRect, C_4ints},
		{"XorRect", XorRect, C_5ints},
		{"ColorRect", ColorRect, C_5ints},

		{"ColorLine", ColorLine, C_direct},
		{"DrawLine", DrawLine, C_4ints},
		{"ClearLine", ClearLine, C_4ints},
		{"InvertLine", InvertLine, C_4ints},

		{"DrawPixel", DrawPixel, C_2ints},
		{"ClearPixel", ClearPixel, C_2ints},
		{"InvertPixel", InvertPixel, C_2ints},

		{"GetHDC", GetHDC, C_noArgs},

		{"DrawClippedText", DrawClippedText, C_string5ints},
		{"DisplayText", DisplayText, C_string4ints},
		{"DisplayTextRight", DisplayTextRight, C_string4ints},
		{"DisplayTextCentered", DisplayTextCentered, C_string4ints},
		{"NeedRedraw", NeedRedraw, C_intstring},

		{"TextSize", TextSize, C_string},

		{"SetBgColor", SetBgColor, C_int},
		{"SetDrawColor", SetDrawColor, C_int},
		{"RGB", MyRGB, C_3ints},
		{"RGBA", MyRGBA, C_4ints},

		// Time
		{"Time", GetTime, C_noArgs},
		{"FormatTime", FormatTime, C_stringintstring},
		{"GetTickCount", ScriptGetTickCount, C_noArgs},
		// really only 3 ints, but I'm lazy.
		{"SetLocale", SetLocale, C_4ints},

		{"FormatText", FormatText, C_stringint},

		/*
		// Perf counters
		{"StartCounterListen", StartCounterListen, C_3strings},
		{"StopCounterListen", StopCounterListen, C_int},
		{"GetCounterValue", GetCounterValue, C_int},
		{"GetCounterValueList", GetCounterValueList, C_int},
		{"UpdateCounters", UpdateCounters},
		{"GetUpstream", GetUpstream, C_noArgs},
		{"GetDownstream", GetDownstream, C_noArgs},
		{"GetAllUp", GetAllUp, C_noArgs},
		{"GetAllDown", GetAllDown, C_noArgs},
		//*/

		// IP
		{"GetLocalIP", GetLocalIP, C_noArgs},
		{"GetLocalDNS", GetLocalDNS, C_noArgs},
#ifdef _DEBUG
		// So I don't spam the few ip servers while debugging.
		{"GetRemoteIP", GetLocalIP, C_noArgs},
#else
		{"GetRemoteIP", GetRemoteIP, C_noArgs},
#endif
		{"GetRemoteDNS", GetRemoteDNS, C_noArgs},

/*
		// Images

		{"LoadImage", MyLoadImage, C_stringdouble},
		{"FreeImage", FreeImage, C_int},
		{"DrawImage", DrawImage, C_7ints},
		{"ClearImage", ClearImage, C_7ints},
		{"InvertImage", InvertImage, C_7ints},
		//*/

		// WindowManagement
		{"GetForegroundWindow", GetForegroundWindow, C_noArgs},
		{"GetWindowText", MyGetWindowText, C_int},
		{"CloseWindow", CloseWindow, C_int},
		{"GetWindowModuleFileName", MyGetWindowModuleFileName, C_2ints},
		{"GetProcessFileName", MyGetProcessFileName, C_2ints},

		{"GetWindowProcessId", GetWindowProcessId, C_int},
		{"KillProcess", KillProcess, C_int},
		{"SetProcessPriority", ScriptSetProcessPriority, C_2ints},
		{"GetProcessPriority", ScriptGetProcessPriority, C_int},

		{"GetWindowRect", ScriptGetWindowRect, C_int},
		{"GetWindowClientRect", GetWindowClientRect, C_int},
		{"PostMessage", ScriptPostMessage, C_4ints},
		{"SendMessage", ScriptSendMessage, C_4ints},
		{"IsWindow", ScriptIsWindow, C_int},
		{"FindWindow", ScriptFindWindow, C_2strings2ints},
		{"EnumWindows", ScriptEnumWindows, C_noArgs},
		{"EnumChildWindows", ScriptEnumChildWindows, C_2ints},

		{"GetScriptWindow", GetScriptWindow, C_noArgs},

		// Timers
		{"CreateFastTimer", CreateFastTimer, C_string2intsobj},
		{"CreateTimer", CreateTimer, C_string2intsobj},
		{"ModifyFastTimer", ModifyFastTimer, C_2ints},
		{"ModifyTimer", ModifyTimer, C_2ints},
		{"StartTimer", StartTimer, C_2ints},
		{"StopTimer", StopTimer, C_int},
		{"KillTimer", KillTimer, C_int},

		// Types
		{"type", GetType, C_direct},
		{"IsNull", IsNull, C_direct},
		{"IsString", IsString, C_direct},
		{"IsInt", IsInt, C_direct},
		{"IsFloat", IsFloat, C_direct},
		{"IsList", IsList, C_direct},
		{"IsDict", IsDict, C_direct},
		{"IsObject", IsObject, C_direct},
		{"IsNumber", IsNum, C_direct},
		{"IsSimple", IsSimple, C_direct},
		{"IsCompound", IsCompound, C_direct},
		{"StackCount", StackCount, C_direct},
		{"Equals", Equals, C_2objs},

		// Waits
		{"Wait", (ScriptProc*)Wait, C_directwait},
		{"Sleep", Sleep, C_int},
		//{"MessageBoxWait", (ScriptProc*)MessageBoxWait, C_directwait},
		{"HttpGetWait", (ScriptProc*)HttpGetWait, C_directwait},
		{"ParseXML", ParseXML, C_string},
		{"PartialParseXML", PartialParseXML, C_stringint},
		{"FindNextXML", FindNextXML, C_direct},
		{"SpawnThread", SpawnThread, C_string2objs},


		// FileManagement
		{"Run", Run, C_2stringsintstring},
		{"RunSimple", RunSimple, C_2stringsint},

		// SharedMemory
		{"GetSharedMemory", GetSharedMemory, C_string},

		// StringParser
		{"ToUpper", ScriptToUpper, C_string},
		{"ToLower", ScriptToLower, C_string},

		{"ParseBinaryInt", ParseBinaryInt, C_string3ints},
		{"ParseBinaryInts", ParseBinaryInts, C_string4ints},
		{"ParseBinaryIntReverse", ParseBinaryIntReverse, C_string3ints},

		{"ParseBinaryFloat", ParseBinaryFloat, C_string2ints},

		{"ParseBinaryStringASCII", ParseBinaryStringASCII, C_string2ints},
		{"ParseBinaryStringUTF8", ParseBinaryStringUTF8, C_string2ints},
		{"ParseBinaryStringUTF16", ParseBinaryStringUTF16, C_string2ints},

		{"ParseBinaryBinary", ParseBinaryBinary, C_string2ints},

		{"UTF8toASCII", UTF8toASCII, C_string},
		{"UTF8toUTF16", UTF8toUTF16, C_string},
		{"UTF16toUTF8", UTF16toUTF8, C_string},
		{"ASCIItoUTF8", ASCIItoUTF8, C_string},

		{"SaveString", SaveString, C_3stringsint},
		{"GetString", GetString, C_3strings},

		{"strstr", MyStrstr, C_2strings2ints},
		{"stristr", MyStristr, C_2strings2ints},
		{"strsplit", strsplit, C_2strings3ints},
		{"strreplace", strreplace, C_3strings3ints},

		{"strinsert", strinsert, C_2stringsint},
		{"substring", substring, C_string2ints},

		{"strswap", strswap, C_2strings2ints},

		{"GetChar", GetChar, C_stringint},

		// Dll
		{"UnloadDll", UnloadDll, C_string},

		// Mouse
		{"SetCursorPos", ScriptSetCursorPos, C_2ints},
		{"GetCursorPos", ScriptGetCursorPos, C_noArgs},
		{"ClipCursor", ScriptClipCursor, C_4ints},

		{"MouseMove", MouseMove, C_5ints},
		{"MouseDown", MouseDown, C_int},
		{"MouseUp", MouseUp, C_int},
		{"MouseScroll", MouseScroll, C_2ints},

		{"ContextMenuWait", (ScriptProc*)ContextMenuWait, C_directwait},

		{"EnumDisplayMonitors", ScriptEnumDisplayMonitors, C_4ints},

		// Keyboard
		{"GetKeyState", ScriptGetKeyState, C_direct},
		{"KeyDown", KeyDown, C_direct},
		{"KeyUp", KeyUp, C_direct},
		{"UnregisterKeyEvent", UnregisterKeyEvent, C_direct},
		{"RegisterKeyEvent", RegisterKeyEvent, C_direct},

		// RegExp.
		{"RegExp", RegExp, C_2stringsint},

		// G15 scripts
		{"G15SetBacklightColor", G15SetBacklightColor, C_intstring},
		{"G15SetMLights", G15SetMLights, C_intstring},
		{"G15SetLCDLight", G15SetLCDLight, C_intstring},
		{"G15SetContrast", G15SetContrast, C_intstring},
		{"G15SetLight", G15SetLight, C_intstring},
		{"G15GetState", G15GetState, C_string},
		{"G15GetButtonsState", G15GetButtonsState, C_string},

		{"G15EnableHID", G15EnableHID, C_noArgs},
		{"G15DisableHID", G15DisableHID, C_noArgs},
		{"GetG15s", GetG15s, C_int},

		// Math

		{"abs", abs, C_direct},
		{"log", log, C_double},
		{"log10", log10, C_double},
		{"cos", cos, C_double},
		{"sin", sin, C_double},
		{"acos", acos, C_double},
		{"asin", asin, C_double},
		{"tan", tan, C_double},
		{"atan", atan, C_double},
		{"sqrt", sqrt, C_double},
		{"exp", exp, C_double},
		{"pow", pow, C_2doubles},

		{"rand", rand, C_2ints},
		{"randf", randf, C_2doubles},

		// lists
		{"clear", sqrt, C_direct},
		{"resize", resize, C_direct},
		{"realloc", realloc, C_direct},
		{"pop_range", pop_range, C_direct},
		{"pop", pop, C_direct},
		{"push_back", push_back, C_direct},
		{"range", range, C_direct},
		{"insert", insert, C_direct},
		{"dictlist", dictlist, C_obj},
		{"dictkeys", dictkeys, C_obj},
		{"dictvalues", dictvalues, C_obj},
		{"indexsort", indexsort, C_obj},
		{"indexstablesort", indexstablesort, C_obj},
		{"sort", sort, C_obj},


		// Bencode
		{"Bencode", Bencode, C_direct},
		{"BencodeExact", BencodeExact, C_direct},
		{"Bedecode", Bedecode, C_string},

		{"JSONdecode", JSONdecode, C_string},

		// Event
		{"SetEventHandler", SetEventHandler, C_2strings2objs},
		{"PostEvent", PostEvent, C_stringintobj},

		// Audio
		{"GetNumAudioDevices", GetNumMixers, C_noArgs},
		{"GetAudioValue", GetAudioValue, C_5ints},
		{"SetAudioValue", SetAudioValue, C_5ints},
		{"GetAudioType", GetAudioType, C_direct},
		{"GetAudioString", GetAudioString, C_direct},
		{"GetVistaMasterVolume", GetVistaMasterVolume, C_noArgs},

		// File.h
		{"MoveFile", MyMoveFile, C_2strings},
		{"CopyFile", MyCopyFile, C_2strings},
		{"DeleteFile", MyDeleteFile, C_string},

		// DDE
		{"DDEWait", (ScriptProc*)DDEWait, C_directwait},
		// Eval
		//{"Eval", Eval, C_string},
	};
	int res = 1;
	for (int i =0; i<sizeof(desc)/sizeof(ProcDescription); i++) {
		res &= RegisterProc((unsigned char*) desc[i].name, desc[i].proc, desc[i].type);
	}
	return res;
}

void CleanupProcs() {
	//CleanupCounters();
	CleanupIP();
	CleanupEmail();
	CleanupTimers();
	CleanupWaits();
	CleanupDlls();
	CleanupEvents();
}
