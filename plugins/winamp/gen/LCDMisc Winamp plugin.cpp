#define GEN_NAME "LCD Miscellany WinAmp Plugin 0.3"

#ifdef NDEBUG
// /Og (global optimizations), /Os (favor small code), /Oy (no frame pointers)
#pragma optimize("gsy",on)

#if (_MSC_VER<1300)
	#pragma comment(linker,"/RELEASE")
	#pragma comment(linker,"/opt:nowin98")

#endif
#endif

#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
// Needed for sprintf
#include <stdio.h>

#include "GEN.H"
#include "wa_ipc.h"

int init();
void quit();
void config() {
	MessageBox(0, "Nothing to configure.", GEN_NAME, MB_OK);
}

int timerId = 0;
unsigned char version[2] = {0,0};

HANDLE sharedMemory = 0;

typedef struct {
	short version;
	short state;
	short vol;
	short bal;
	HWND hWnd;
	int pos;
	int duration;
	short mode;
	short year;
	int track;
	int tracks;
	char name[4];
} PlayingData;

PlayingData *data = 0;

WNDPROC wndProcOld = 0;

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, void* lpvReserved) {
	if (fdwReason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(hInstance);
	}
    return 1;
}

winampGeneralPurposePlugin plugin = {
	GPPHDR_VER,
	GEN_NAME,
	init,
	config,
	quit,
};

void quit() {
	KillTimer(0, timerId);
	timerId = 0;
	if (sharedMemory && sharedMemory != INVALID_HANDLE_VALUE) {
		if (data) {
			UnmapViewOfFile(data);
			data = 0;
		}
		CloseHandle(sharedMemory);
		sharedMemory = 0;
	}
	if (wndProcOld) {
		SetWindowLong(plugin.hwndParent, GWL_WNDPROC, (LONG)wndProcOld);
	}
}

int ToUTF8(wchar_t *src, char *dest, int destLen, int *outLen) {
	int i;
	if (!src || (((int)src) &1)|| !src[0]) return 0;
	i = WideCharToMultiByte(CP_UTF8, 0, src, -1, dest, destLen, 0, 0);
	if (i>0) {
		*outLen = i;
		return 1;
	}
	return 0;
}

void GetUTF8(char *dest, int destLen, int *outLen, int arg3, int arg4w, int arg4a) {
	// Not sure about version numbers...Winamp has no complete plugin specs.
	char *name;
	int len = 0;
	if (version[0] > 5 || (version[0] == 5 && version[1] >= 3)) {
		wchar_t *name = (wchar_t*)SendMessage(plugin.hwndParent, WM_WA_IPC, arg3, arg4w);
		if (ToUTF8(name, dest, destLen, outLen)) return;
	}
	name = (char*)SendMessage(plugin.hwndParent, WM_WA_IPC, arg3, arg4a);

	if (name) len = (int) strlen(name);
	if (len >= destLen-1) {
		len = destLen-1;
	}
	memcpy(dest, name, len);
	dest[len] = 0;
	*outLen = len+1;
}

void CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
	static char versionString[16];
	union {
		PlayingData p;
		char junk[4096];
	};
	extendedFileInfoStruct s;
	char *file;
	int index, pos, pos2;
	wchar_t temp[MAX_PATH * 8];
	static HWND lcdMisc = 0;
	if (!data || !timerId) return;
	if (!version[0]) {
		LRESULT res = SendMessage(plugin.hwndParent, WM_WA_IPC,0,IPC_GETVERSION);
		data->mode = -1;
		version[0] = (unsigned char)(res>>12);
		if (version[0] < 1 || version[0] >= 0x80) {
			version[0] = 0;
			return;
		}
		if (version[0] > 2 || (res & 0xF0)) version[1] = (unsigned char)res;
		// Erm... Don't look at me.
		else version[1] = (unsigned char)(res + (res >> 4));
		sprintf(versionString, "%i.%i", version[0], version[1]);
	}
	p.hWnd = plugin.hwndParent;
	p.vol = (short) SendMessage(plugin.hwndParent, WM_WA_IPC,-666,IPC_SETVOLUME);
	p.bal = (char) SendMessage(plugin.hwndParent, WM_WA_IPC,-666,IPC_SETPANNING);
	p.version = 1;
	p.mode = ((short) SendMessage(plugin.hwndParent, WM_WA_IPC,0,IPC_GET_SHUFFLE))<<1;
	if (!p.mode)
		p.mode = (short) SendMessage(plugin.hwndParent, WM_WA_IPC,0,IPC_GET_REPEAT);
	else if (version[0] == '2' && !SendMessage(plugin.hwndParent, WM_WA_IPC,0,IPC_GET_REPEAT)) {
		// Shuffle but not repeat = no shuffle on 2.95, but not in 5.50.  When did it change?
		// No clue!  True brilliance!  Really, allowing shuffle with and without repeat in the
		// first place was just bad design.  Should be one param with 3 values.  Better than
		// VLC, though, which has *4* distinct binary values for repeat/shuffle modes, all
		// independent...
		p.mode = 0;
	}
	p.state = (short) SendMessage(plugin.hwndParent, WM_WA_IPC,0,IPC_ISPLAYING);
	if (p.state == 3) {
		p.state = 1;
	}
	else if (p.state == 1) p.state = 2;
	p.duration = (int) SendMessage(plugin.hwndParent, WM_WA_IPC,1,IPC_GETOUTPUTTIME);
	if (p.state)
		p.pos = (int) SendMessage(plugin.hwndParent, WM_WA_IPC,0,IPC_GETOUTPUTTIME);
	else
		p.pos = 0;
	index = SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETLISTPOS);
	p.track = index+1;
	p.tracks = SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETLISTLENGTH);

	GetUTF8(p.name, 1024*2, &pos, index, IPC_GETPLAYLISTTITLE+1, IPC_GETPLAYLISTTITLE);

	file = p.name+pos;
	GetUTF8(p.name+pos, 4080-sizeof(p)-pos, &pos2, index, IPC_GETPLAYLISTFILE+3, IPC_GETPLAYLISTFILE);
	pos += pos2;
	if (!MultiByteToWideChar(CP_UTF8, 0, file, -1, temp, sizeof(temp)/2)) {
		p.year = 0;
		p.name[pos] = 0;
	}
	else {
		wchar_t artist[2048];
		int r = 0;
		int lenLeft;
		s.retlen = 2048;
		s.filename = (char*)temp;
		s.ret = (char*) artist;
		// Just in case...
		artist[0] = 0;
		// No clue about the 13.  No specs, dangit!
		if (version[0] > 5 || (version[0] == 5 && version[1] >= 3)) {
			s.metadata = (char*)L"artist";
			memset(artist, 0, sizeof(artist));
			r = SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM) &s, 3026);
		}
		lenLeft = 4080-(pos+sizeof(p)-4);
		if (!r || !ToUTF8(artist, p.name+pos, lenLeft, &pos2)) {
			s.filename = file;
			s.metadata = "artist";
			memset(artist, 0, sizeof(artist));
			r = SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM) &s, IPC_GET_EXTENDED_FILE_INFO);
			if (!r) {
				p.name[pos] = 0;
				pos++;
			}
			else {
				int len = strlen(s.ret);
				if (len >= lenLeft) len = lenLeft-1;
				memcpy(p.name+pos, s.ret, len);
				p.name[pos+len] = 0;
				pos += len+1;
			}

			s.metadata = "year";
			r = SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM) &s, IPC_GET_EXTENDED_FILE_INFO);
			if (r) {
				p.year = atoi(s.ret);
			}
			else {
				p.year = 0;
			}
		}
		else {
			pos += pos2;
			s.metadata = (char*)L"year";
			r = SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM) &s, 3026);
			if (r) {
				p.year = _wtoi((wchar_t*)s.ret);
			}
			else {
				p.year = 0;
			}
		}
	}
	pos2 = p.pos;
	p.pos = ((PlayingData*)data)->pos;
	if (memcmp(&p, data, pos+sizeof(p)-4)) {
		p.pos = pos2;
		memcpy(data, &p, pos+sizeof(p)-4);
		memcpy(data->name+pos, versionString, 16);
		if (!lcdMisc || !IsWindow(lcdMisc)) {
			lcdMisc = FindWindowW(L"LCD Miscellany",  L"Text no one should see.");
		}
		if (lcdMisc) {
			PostMessage(lcdMisc, WM_USER, 0, 0);
		}
	}
	else {
		p.pos = pos2;
		memcpy(data, &p, pos+sizeof(p)-4);
	}
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	LRESULT out = 0;
	if (wndProcOld)
		out = CallWindowProc(wndProcOld, hWnd, uMsg, wParam, lParam);
	if ((uMsg == WM_WA_IPC && (lParam == IPC_CB_MISC || lParam == IPC_SET_SHUFFLE || lParam == IPC_SET_REPEAT)) ||
		(uMsg == WM_COMMAND)) {
		TimerProc(0,0,0,0);
	}
	return out;
}

int init() {
	if (timerId || sharedMemory) {
		// ??
		quit();
	}
	timerId = SetTimer(0, 0, 1000, TimerProc);
	if (!timerId) return 1;
	sharedMemory = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, 4096, "LCDMisc Winamp");
	if (!sharedMemory) {
		quit();
		return 1;
	}
	data = (PlayingData*)MapViewOfFile(sharedMemory, FILE_MAP_WRITE, 0,0, 4096);
	if (!data) {
		quit();
		return 1;
	}
	memset(data, 0, sizeof(*data));
	data->mode = -1;
	wndProcOld = (WNDPROC)SetWindowLong(plugin.hwndParent, GWL_WNDPROC, (LONG)WndProc);
	return 0;
}


winampGeneralPurposePlugin * __stdcall winampGetGeneralPurposePlugin() {
	return &plugin;
}
