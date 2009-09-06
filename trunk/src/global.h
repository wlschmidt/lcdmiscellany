//#define _CRT_SECURE_NO_DEPRECATE
#pragma once
#define _UNICODE
#define UNICODE
#define _WIN32_WINNT 0x0600
#define WINVER _WIN32_WINNT

//*
//#define WIN32_LEAN_AND_MEAN

//#define STRSAFE_NO_DEPRECATE

#define VC_EXTRALEAN
#define NOMCX
#define NOKANJI
#define NOCRYPT
#define NOSCROLL
#define NOSOUND
#define NOSYSMETRICS
//*/

#define VERSION_STRING_SHORT "0.4.5.0"

#ifdef X64
#define BITS 64
#define VERSION_STRING_LONG VERSION_STRING_SHORT " 64-bit"
#else
#define BITS 32
#define VERSION_STRING_LONG VERSION_STRING_SHORT " 32-bit"
#endif

#define APP_NAME "LCD Miscellany " VERSION_STRING_LONG

#define VERSION {0,4,5,0, BITS}

#ifdef NDEBUG

// /Og (global optimizations), /Os (favor small code), /Oy (no frame pointers)
#pragma optimize("gyit",on)
#if (_MSC_VER<1300)
//	#pragma comment(linker,"/STACK:256000")
	#pragma comment(linker,"/RELEASE")
	#pragma comment(linker,"/opt:nowin98")
	//#pragma comment(linker,"/arch")
	//#pragma comment(linker,"/g7")
	//#pragma comment(linker,"/OPT:REF")

#endif
#endif
//#define CDECL __cdecl
//#include <WTYPES.H>
//*/
#define _WINSOCKAPI_
#include <windows.h>
//#include <Winsock2.h>
//#include <Ws2tcpip.h>
//#include <dshow.h>
//#include <Vfw.h>
//#include "resource.h"
//#include "util.h"

extern HINSTANCE ghInst;
extern HWND ghWnd;
extern HWND nextClipboardListener;
// extern HHOOK hook;
// extern int gTick;
#ifndef GLOBAL_H
#define GLOBAL_H

#define VK_BROWSER_BACK        0xA6
#define VK_BROWSER_FORWARD     0xA7
#define VK_BROWSER_REFRESH     0xA8
#define VK_BROWSER_STOP        0xA9
#define VK_BROWSER_SEARCH      0xAA
#define VK_BROWSER_FAVORITES   0xAB
#define VK_BROWSER_HOME        0xAC

#define VK_VOLUME_MUTE         0xAD
#define VK_VOLUME_DOWN         0xAE
#define VK_VOLUME_UP           0xAF
#define VK_MEDIA_NEXT_TRACK    0xB0
#define VK_MEDIA_PREV_TRACK    0xB1
#define VK_MEDIA_STOP          0xB2
#define VK_MEDIA_PLAY_PAUSE    0xB3
#define VK_LAUNCH_MAIL         0xB4
#define VK_LAUNCH_MEDIA_SELECT 0xB5
#define VK_LAUNCH_APP1         0xB6
#define VK_LAUNCH_APP2         0xB7


#define WMU_BUTTONS			(WM_APP + 0x001)
#define WMU_BUTTONS_HID		(WM_APP + 0x010)
//#define WMU_EXEC			(WM_APP + 0x010)

#define WMU_CALL_PROC		(WM_APP + 0x020)

#define WMU_VISTA_VOLUME_CHANGE		(WM_APP + 0x100)
#define WMU_VISTA_AUDIO_DEV_CHANGE	(WM_APP + 0x101)
//#define WMU_FOUND_PAGE		(WM_APP + 0x100)
//#define WMU_LOCAL_NAMES		(WM_APP + 0x101)
#define WMA_ICON_MESSAGE		(WM_APP + 0x110)
#define WMU_HTTP_SOCKET			(WM_APP + 0x200)
#define WMU_PING_SOCKET			(WM_APP + 0x201)
//#define WMU_START_CONNECTING	(WM_APP + 0x201)
#define WMU_DNS_DONE			(WM_APP + 0x202)
#define WMU_SCRIPTED_SOCKET		(WM_APP + 0x203)
#define WMU_DSHOW_EVENT			(WM_APP + 0x300)
#define WMU_KEYBOARD_MESSAGE	(WM_APP + 0x301)
#define WMU_TIMER_PROC_DONE		(WM_APP + 0x302)
#define WMU_TRIGGER_EVENT		(WM_APP + 0x303)
#define WMU_TRIGGER_EVENT_FULL	(WM_APP + 0x304)
#define WMU_EATEN_KEY			(WM_APP + 0x305)

#define WMA_LCD_DEVICE_CHANGE	 (WM_APP + 0x310)
#define WMA_TRIGGER_EVEN_BY_NAME (WM_APP + 0x311)

#include "globals.h"




//void IpChanged(int flags);

#endif

#include "resource.h"

/*
#ifdef free
#undef free
#endif

__forceinline void goatfree(void **x) {
	static int count = 0;
	count++;
	free(*x);
	if (*x)
		*x = 0;
}

__forceinline void nfree(void *x) {
	free(x);
}

//#define free(x) goatfree((void**)&x)
//*/

void QueueRedraw();