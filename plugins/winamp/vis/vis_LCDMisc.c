#ifdef NDEBUG
// /Og (global optimizations), /Os (favor small code), /Oy (no frame pointers)
#pragma optimize("gsy",on)

#if (_MSC_VER<1300)
	#pragma comment(linker,"/RELEASE")
	#pragma comment(linker,"/opt:nowin98")

#endif
#endif

#define VIS_NAME "LCD Miscellany WinAmp Visualization Plugin 0.2"
#include <Winsock2.h>
#include <Windows.h>
#include "VIS.h"


BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, void* lpvReserved) {
	if (fdwReason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(hInstance);
	}
    return 1;
}


winampVisModule *GetModule(int which);

struct sockaddr_in addr;

__declspec(dllexport) winampVisHeader * __cdecl winampVisGetHeader() {
	static winampVisHeader hdr = { VIS_HDRVER, VIS_NAME, GetModule };
	return &hdr;
}

void Config(struct winampVisModule *this_mod) {
	MessageBoxA(0, "Nothing to configure.", VIS_NAME, MB_OK);
}

SOCKET csocket = INVALID_SOCKET;
int wsaInit = 0;

int Init(struct winampVisModule *this_mod) {
	WSADATA WSAData;
	if (wsaInit) return 0;
	if (0 != WSAStartup(0x202, &WSAData)) {
		return 1;
	}
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(33495);
	wsaInit = 1;
	return 0;
}

typedef struct {
	// not null terminated.
	char header[20];
	char spectrum[576];
	char waveform[576];
} VisualizationData;

int Render(struct winampVisModule *this_mod) {
	if (csocket == INVALID_SOCKET) {
		if (wsaInit) {
			csocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		}
		if (csocket == INVALID_SOCKET) return 0;
	}
	if (this_mod->spectrumNch > 0 && this_mod->waveformNch > 0) {
		VisualizationData data;

		memcpy(data.header, "WAMP LCDMisc Viz 0.2", 20);
		memcpy(data.spectrum, this_mod->spectrumData[0], 576);
		memcpy(data.waveform, this_mod->waveformData[0], 576);
		if (sendto(csocket, (char*)&data, sizeof(data), 0, (SOCKADDR *) &addr, sizeof(addr)) == SOCKET_ERROR) {
			closesocket(csocket);
			csocket = INVALID_SOCKET;
		}
	}
	return 0;
}
void Quit(struct winampVisModule *this_mod) {
	if (csocket != INVALID_SOCKET) {
		sendto(csocket, "WAMP LCDMisc Viz 0.2", 20, 0, (SOCKADDR *) &addr, sizeof(addr));
		closesocket(csocket);
		csocket = INVALID_SOCKET;
	}
	if (wsaInit) {
		WSACleanup();
		wsaInit = 0;
	}
}

winampVisModule mod = {
	VIS_NAME,
	NULL,NULL,
	0, 0,
	15, 70,
	1, 1,
	{0}, {0},
	Config,
	Init,
	Render,
	Quit,
	0
};

winampVisModule *GetModule(int which) {
	if (which == 0) return &mod;
	return 0;
}
