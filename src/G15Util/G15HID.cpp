#include "../global.h"
#include "G15HID.h"
#include "../malloc.h"
#include <Setupapi.h>

#include "../Screen.h"

#include "../StringUtil.h"
#include "../Unicode.h"
#include "../ScriptProcs/Event/Event.h"

extern "C"
{
#include <hidsdi.h>
}

//#include <Dbt.h>
//HDEVNOTIFY hDevNotify = 0;


struct Buffer {
	unsigned char junk[0x03e0];
};
GUID GUID_DEVINTERFACE_HID;
//= { 0x4d1e55b2L, 0xf16f, 0x11cf, { 0x88, 0xcb, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 } };

/*
    DEVICE("Logitech G15",0x46d,0xc222,G15_LCD|G15_KEYS),
    DEVICE("Logitech G11",0x46d,0xc225,G15_KEYS),
    DEVICE("Logitech Z-10",0x46d,0x0a07,G15_LCD|G15_KEYS|G15_DEVICE_IS_SHARED),
    DEVICE("Logitech G15 v2",0x46d,0xc227,G15_LCD|G15_KEYS|G15_DEVICE_5BYTE_RETURN),
    DEVICE("Logitech Gamepanel",0x46d,0xc251,G15_LCD|G15_KEYS|G15_DEVICE_IS_SHARED),
//*/

struct LogitechLCDDeviceInfo {
	unsigned short vid, pid, inputBytes, featureReportBytes, outputReportBytes, width, height, bpp;
	HID_DEVICE_TYPE type;
};

const LogitechLCDDeviceInfo DeviceTypes[] = {
	{0x46d, 0xc225, 9, 4, 992, 160, 43, 1, HID_LCD_G11        },
	{0x46d, 0xc222, 9, 4, 992, 160, 43, 1, HID_LCD_G15_V1     },
	{0x46d, 0xc227, 5, 4, 992, 160, 43, 1, HID_LCD_G15_V2     },
	{0x46d, 0x0a07, 9, 4, 992, 160, 43, 1, HID_LCD_Z10        },
	{0x46d, 0xc251, 9, 4, 992, 160, 43, 1, HID_LCD_GAME_PANEL },
	{0x46d, 0xc229, 4, 258, 0, 320,240,32, HID_LCD_G19        },
};

class HidDevice;

struct G15 {
	OVERLAPPED overlapped;
	OVERLAPPED overlappedWrite;
	ScriptValue name;
	HANDLE hFile;
	unsigned short deviceTypeIndex;
	unsigned char bytes[9];
	bool reading;
	bool writing;
	bool needFree;
	HidDevice *device;
	Buffer buffer;
	void Cleanup() {
		name.stringVal->Release();
		free(this);
	}
};


class HidDevice : public Device {
public:
	// Note:  Cleanup of the g15 object handled by other thread, when active, and by main thread when not.
	HidDevice(G15 *g15, int w, int h, int bpp) : Device(w, h, bpp, CONNECTION_DIRECT, DeviceTypes[g15->deviceTypeIndex].type, (char*)g15->name.stringVal->value) {
		this->g15 = g15;
	}
	int UpdateImage() {
		if (width == 160) {
			// Fail but do nothing.
			if (!g15->hFile || g15->writing) return 0;
			Buffer *buffer = &g15->buffer;
			unsigned char *junk = buffer->junk;
			memset(junk, 0, sizeof(Buffer));
			//memset(junk+1, 0, 31);
			//memset(junk + (160/8)*6+32, 0, sizeof(Buffer)-((160/8)*6+32));
			junk[0] = 3;
			Color4 *src = image;
			for (int y=0; y<43; y++) {
				unsigned char *dst = junk+(32 + y/8*160);
				for (int x=0; x<160; x++) {
					unsigned char color = (((short)src->r + (short)src->g + (short)src->b) < (128*3)-1);
					dst++[0] |= (color << (y&7));
					src++;
				}
			}
			g15->writing = 1;
			memset(&g15->overlappedWrite, 0, sizeof(OVERLAPPED));
			if (!WriteFile(g15->hFile, g15->buffer.junk, sizeof(Buffer), 0, &g15->overlappedWrite) && GetLastError() != ERROR_IO_PENDING) {
				g15->writing = 0;
			}
			return 1;
		}
		return 0;
		/*
			//memcpy(junk+32, screen.bmp.data, (43*160)/8);
			//junk[32] = 0xFF;
			//junk[35] = 0xFF;
			/*
			for (int y = 0; y<40; y+=8) {
				int offset = 32 + y*(160/8);
				for (int x=0; x<160;x++) {
					int xSource = x/32+y*(160/32);
					int shift = x%32;
					junk[offset+x] = ((screen.bmp.data[xSource]>>shift)&1) +
						(((screen.bmp.data[xSource+  160/32]>>shift)&1)<<1) +
						(((screen.bmp.data[xSource+2*160/32]>>shift)&1)<<2) +
						(((screen.bmp.data[xSource+3*160/32]>>shift)&1)<<3) +
						(((screen.bmp.data[xSource+4*160/32]>>shift)&1)<<4) +
						(((screen.bmp.data[xSource+5*160/32]>>shift)&1)<<5) +
						(((screen.bmp.data[xSource+6*160/32]>>shift)&1)<<6) +
						(((screen.bmp.data[xSource+7*160/32]>>shift)&1)<<7);
				}
			}//*/
	#ifdef GOAT
				for (int y = 0; y<40; y+=8) {
					int offset = 32 + y*(160/8);
					for (int x=0; x<160/8;x++) {
						int xSource = x+offset-32;
						unsigned int pairs1 = ((((unsigned char*)screen.bmp.data)[xSource+7*160/8]<<16) + (((unsigned char*)screen.bmp.data)[xSource+7*160/8]<<7) & ((0xAA<<16) + (0x55<<7))) +
							((((unsigned char*)screen.bmp.data)[xSource+6*160/8]<<15) + (((unsigned char*)screen.bmp.data)[xSource+6*160/8]<<6) & ((0xAA<<15) + (0x55<<6)));
						unsigned int pairs2 = ((((unsigned char*)screen.bmp.data)[xSource+5*160/8]<<14) + (((unsigned char*)screen.bmp.data)[xSource+5*160/8]<<5) & ((0xAA<<14) + (0x55<<5))) +
							((((unsigned char*)screen.bmp.data)[xSource+4*160/8]<<13) + (((unsigned char*)screen.bmp.data)[xSource+4*160/8]<<4) & ((0xAA<<13) + (0x55<<4)));
						unsigned int pairs3 = ((((unsigned char*)screen.bmp.data)[xSource+3*160/8]<<12) + (((unsigned char*)screen.bmp.data)[xSource+3*160/8]<<3) & ((0xAA<<12) + (0x55<<3))) +
							((((unsigned char*)screen.bmp.data)[xSource+2*160/8]<<11) + (((unsigned char*)screen.bmp.data)[xSource+2*160/8]<<2) & ((0xAA<<11) + (0x55<<2)));
						unsigned int pairs4 = ((((unsigned char*)screen.bmp.data)[xSource+1*160/8]<<10) + (((unsigned char*)screen.bmp.data)[xSource+1*160/8]<<1) & ((0xAA<<10) + (0x55<<1))) +
							((((unsigned char*)screen.bmp.data)[xSource+0*160/8]<< 9) + (((unsigned char*)screen.bmp.data)[xSource+0*160/8]<<0) & ((0xAA<< 9) + (0x55<<0)));
						unsigned int quad1 = (pairs1&((0xCC<<16)+(0xCC<<6))) + (pairs2&((0xCC<<14)+(0xCC<<4)));
						unsigned int quad2 = (pairs3&((0xCC<<12)+(0xCC<<2))) + (pairs4&((0xCC<<10)+(0xCC<<0)));
						unsigned int oct = (quad1 & ((0xF0<<16) + (0xF0<<6))) + (quad2 & ((0xF0<<12) + (0xF0<<2)));
						unsigned int oct2 = (quad1 & ((0xF0<<12) + (0xF0<<2))) + (quad2 & ((0x0F<<12) + (0x0F<<2)));
						quad1 = (pairs1&((0xCC<<14)+(0xCC<<4))) + (pairs2&((0xCC<<12)+(0xCC<<2)));
						quad2 = (pairs3&((0xCC<<10)+(0xCC<<0))) + (pairs4&((0xCC<< 8)+(0xCC>>2)));
						unsigned int oct3 = (quad1 & ((0xF0<<14) + (0xF0<<4))) + (quad2 & ((0xF0<<10) + (0xF0<<0)));
						unsigned int oct4 = (quad1 & ((0xF0<<10) + (0xF0<<0))) + (quad2 & ((0x0F<<10) + (0x0F<<0)));
						/*if (junk[offset+x*8+7] != (unsigned char) (oct>>16)) {
							offset=offset;
						}
						if (junk[offset+x*8+6] != (unsigned char) (oct>>6)) {
							offset=offset;
						}
						if (junk[offset+x*8+3] != (unsigned char) (oct2>>12)) {
							offset=offset;
						}
						if (junk[offset+x*8+2] != (unsigned char) (oct2>>2)) {
							offset=offset;
						}
						if (junk[offset+x*8+5] != (unsigned char) (oct3>>14)) {
							offset=offset;
						}
						if (junk[offset+x*8+4] != (unsigned char) (oct3>>4)) {
							offset=offset;
						}
						if (junk[offset+x*8+1] != (unsigned char) (oct4>>10)) {
							offset=offset;
						}
						if (junk[offset+x*8+0] != (unsigned char) (oct4>>0)) {
							offset=offset;
						}
						//*/
						*(unsigned int*)&junk[offset+8*x] =
							((oct4&0xFF) + ((oct4>>2)&0xFF00)) + (((oct2<<14)&0xFF0000) + ((oct2<<12)&0xFF000000));
						*(unsigned int*)&junk[offset+8*x+4] =
							(((oct3>>4)&0xFF) + ((oct3>>6)&0xFF00)) + (((oct<<10)&0xFF0000) + ((oct<<8)&0xFF000000));
						/*
						junk[offset+x*8+7] = (unsigned char) (oct>>16);
						junk[offset+x*8+6] = (unsigned char) (oct>>6);
						junk[offset+x*8+3] = (unsigned char) (oct2>>12);
						junk[offset+x*8+2] = (unsigned char) (oct2>>2);
						//junk[offset+x*8+3] = (unsigned char) (oct>>6);
						//*/
					}
				}
				/*
				for (int y = 0; y<40; y+=8) {
					int offset = 32 + y*(160/8);
					for (int x=0; x<160;x++) {
						int xSource = x/32+y*(160/32);
						int shift = x%32;
						junk[offset+x] = ((screen.bmp.data[xSource]>>shift)&1) +
							(((screen.bmp.data[xSource+  160/32]>>shift)&1)<<1) +
							(((screen.bmp.data[xSource+2*160/32]>>shift)&1)<<2) +
							(((screen.bmp.data[xSource+3*160/32]>>shift)&1)<<3) +
							(((screen.bmp.data[xSource+4*160/32]>>shift)&1)<<4) +
							(((screen.bmp.data[xSource+5*160/32]>>shift)&1)<<5) +
							(((screen.bmp.data[xSource+6*160/32]>>shift)&1)<<6) +
							(((screen.bmp.data[xSource+7*160/32]>>shift)&1)<<7);
					}
				}//*/
				int offset = 32 + 40*(160/8);
				for (int x=0; x<160;x+=4) {
					int xSource = x/32+40*(160/32);
					int shift = x%32;
					*(unsigned int*)&junk[offset+x] =
						(((screen.bmp.data[xSource]>>shift)&1) +
						 (((screen.bmp.data[xSource+  160/32]>>shift)&1)<<1) +
						 (((screen.bmp.data[xSource+2*160/32]>>shift)&1)<<2)) +
						((((screen.bmp.data[xSource]>>(shift+1))&1) +
						  (((screen.bmp.data[xSource+  160/32]>>(shift+1))&1)<<1) +
						  (((screen.bmp.data[xSource+2*160/32]>>(shift+1))&1)<<2))<<8)+
						((((screen.bmp.data[xSource]>>(shift+2))&1) +
						  (((screen.bmp.data[xSource+  160/32]>>(shift+2))&1)<<1) +
						  (((screen.bmp.data[xSource+2*160/32]>>(shift+2))&1)<<2))<<16)+
						((((screen.bmp.data[xSource]>>(shift+3))&1) +
						  (((screen.bmp.data[xSource+  160/32]>>(shift+3))&1)<<1) +
						  (((screen.bmp.data[xSource+2*160/32]>>(shift+3))&1)<<2))<<24);
				}

				for (int i=start; i<numG15s;i++) {
					if (G15s[i]->hFile && !(G15s[i]->writing)) {
						if (i!=start) memcpy(&G15s[i]->buffer, &G15s[start]->buffer, sizeof(Buffer));
						G15s[i]->writing = 1;
						memset(&G15s[i]->overlappedWrite, 0, sizeof(OVERLAPPED));
						if (!WriteFile(G15s[i]->hFile, G15s[i]->buffer.junk, sizeof(Buffer), 0, &G15s[i]->overlappedWrite) && GetLastError() != ERROR_IO_PENDING) {
							G15s[i]->writing = 0;
						}
					}
					/*
					else {
						if (i==numG15s-1) free(buffer);
					}
					/*
					if (i < numG15s-1) {
					}
					int q;
					if (G15s[i]->hFile) {
						q=WriteFile(G15s[i]->hFile, junk, sizeof(junk), 0, &G15s[i]->overlappedWrite);
						int w = GetLastError();
						w=w;
					}
					/*if (G15s[i]->hFile && G15s[i]->inUse < 2 && (q=WriteFileEx(G15s[i]->hFile, junk, sizeof(junk), &G15s[i]->overlappedWrite, G15WriteCompletion))) {
						int w = GetLastError();
						if (!w)
							G15s[i]->inUse++;
					}//*/
				}
			}
		}
		return 0;
		#endif
	}
};

HANDLE hThread = 0;
HANDLE hIOPort = 0;
/*
// Signal for thread to do something.
HANDLE hEvent = 0;
// Signal that 
HANDLE hEventProcessed = 0;
G15 *readFrom = 0;
Buffer *buffer = 0;
//*/

int g15enabledCount = 0;

void G15DisableHID(ScriptValue &s, ScriptValue *args) {
	if (g15enabledCount) {
		if (!--g15enabledCount) {
			int i;
			for (i=0; i<numDevices; i++) {
				if (devices[i]->cType != CONNECTION_DIRECT) continue;
				PostQueuedCompletionStatus(hIOPort, 0, (ULONG_PTR)((HidDevice*)devices[i])->g15, 0);
			}
			/*for (int i=0; i<numG15s; i++) {
				if (G15s[i]->hFile) {
					//G15s[i]->needNuke = 1;
					//CancelIo(G15s[i]->hFile);
				}
			}*/
			if (hThread) {
				PostQueuedCompletionStatus(hIOPort, 0, 0, 0);
				WaitForSingleObject(hThread, 5000);
				//SetEvent(hEvent);
				CloseHandle(hThread);
				CloseHandle(hIOPort);
				hThread = 0;
				hIOPort = 0;
			}
			for (i=0; i<numDevices; i++) {
				if (devices[i]->cType != CONNECTION_DIRECT) continue;
				devices[i]->gKeyState = 0;
			}
		}
		CreateIntValue(s, g15enabledCount);
	}
}


#define G15LCD1		0
#define G15LCD2		1
#define G15LCD3		2
#define G15LCD4		3
#define G15LCD5		4
#define G15LCD6		5

#define G15LCD_LEFT		0
#define G15LCD_RIGHT	1
#define G15LCD_OK		2
#define G15LCD_CANCEL	3
#define G15LCD_UP		4
#define G15LCD_DOWN		5

#define G15LCDR			6

#define G15M1		 8
#define G15M2		 9
#define G15M3		10
#define G15MR		11

#define G15LIGHT	12

#define G15G1		14
#define G15G2		15
#define G15G3		16
#define G15G4		17
#define G15G5		18
#define G15G6		19
#define G15G7		20
#define G15G8		21
#define G15G9		22
#define G15G10		23
#define G15G11		24
#define G15G12		25
#define G15G13		26
#define G15G14		27
#define G15G15		28
#define G15G16		29
#define G15G17		30
#define G15G18		31

extern int connection;

DWORD WINAPI ThreadProc(void* lpParameter) {
	static bool closeHandles;
	closeHandles = 0;
	DWORD bytes;
	G15 *g15;
	OVERLAPPED *overlapped;
	while (1) {
		int res = GetQueuedCompletionStatus(hIOPort, &bytes, (ULONG_PTR*)&g15, &overlapped, INFINITE);
		if (g15) {
			if (g15->hFile && !g15->needFree) {
				if (&g15->overlapped == overlapped) {
					if (res) {
						unsigned char *b = g15->bytes;
						if (bytes > 0 && b[0] == 2) {
							int happy = 0;
							unsigned int state;
							if (bytes == 9 && DeviceTypes[g15->deviceTypeIndex].inputBytes == 9) {
								happy = 1;
								state =
									(((b[2]>>7)&1) << G15LCD1) +
									(((b[3]>>7)&1) << G15LCD2) +
									(((b[4]>>7)&1) << G15LCD3) +
									(((b[5]>>7)&1) << G15LCD4) +
									(((b[8]>>7)&1) << G15LCDR) +

									(((b[6]>>0)&1) << G15M1) +
									(((b[7]>>1)&1) << G15M2) +
									(((b[8]>>2)&1) << G15M3) +
									(((b[7]>>6)&1) << G15MR) +

									(((b[1]>>7)&1) << G15LIGHT) +

									(((b[1]>>0)&1) << G15G1) +
									(((b[2]>>1)&1) << G15G2) +
									(((b[3]>>2)&1) << G15G3) +

									(((b[4]>>3)&1) << G15G4) +
									(((b[5]>>4)&1) << G15G5) +
									(((b[6]>>5)&1) << G15G6) +

									(((b[2]>>0)&1) << G15G7) +
									(((b[3]>>1)&1) << G15G8) +
									(((b[4]>>2)&1) << G15G9) +

									(((b[5]>>3)&1) << G15G10) +
									(((b[6]>>4)&1) << G15G11) +
									(((b[7]>>5)&1) << G15G12) +

									(((b[1]>>2)&1) << G15G13) +
									(((b[2]>>3)&1) << G15G14) +
									(((b[3]>>4)&1) << G15G15) +

									(((b[4]>>5)&1) << G15G16) +
									(((b[5]>>6)&1) << G15G17) +
									(((b[8]>>6)&1) << G15G18);
							}
							else if (bytes == 5 && DeviceTypes[g15->deviceTypeIndex].inputBytes == 5) {
								happy = 1;
								state =
									(((b[2]>>1)&1) << G15LCD1) +
									(((b[2]>>2)&1) << G15LCD2) +
									(((b[2]>>3)&1) << G15LCD3) +
									(((b[2]>>4)&1) << G15LCD4) +
									(((b[2]>>7)&1) << G15LCDR) +

									(((b[1]>>6)&1) << G15M1) +
									(((b[1]>>7)&1) << G15M2) +
									(((b[2]>>5)&1) << G15M3) +
									(((b[2]>>6)&1) << G15MR) +

									(((b[2]>>0)&1) << G15LIGHT) +

									(((b[1]>>0)&1) << G15G1) +
									(((b[1]>>1)&1) << G15G2) +
									(((b[1]>>2)&1) << G15G3) +

									(((b[1]>>3)&1) << G15G4) +
									(((b[1]>>4)&1) << G15G5) +
									(((b[1]>>5)&1) << G15G6);
							}
							else if (bytes == 4 && DeviceTypes[g15->deviceTypeIndex].inputBytes == 4) {
								happy = 1;
								state =
									(((b[2]>>4)&1) << G15M1) +
									(((b[2]>>5)&1) << G15M2) +
									(((b[2]>>6)&1) << G15M3) +
									(((b[2]>>7)&1) << G15MR) +

									(((b[3]>>3)&1) << G15LIGHT) +

									(((b[1]>>0)&1) << G15G1) +
									(((b[1]>>1)&1) << G15G2) +
									(((b[1]>>2)&1) << G15G3) +

									(((b[1]>>3)&1) << G15G4) +
									(((b[1]>>4)&1) << G15G5) +
									(((b[1]>>5)&1) << G15G6) +

									(((b[1]>>6)&1) << G15G7) +
									(((b[1]>>7)&1) << G15G8) +

									(((b[2]>>0)&1) << G15G9) +
									(((b[2]>>1)&1) << G15G10)+
									(((b[2]>>2)&1) << G15G11)+
									(((b[2]>>3)&1) << G15G12);
							}
							if (happy) {
								PostMessage(ghWnd, WMU_BUTTONS_HID, (LPARAM) state, (WPARAM)g15);
							}
							memset(&g15->overlapped, 0, sizeof(OVERLAPPED));
						}

						if (ReadFile(g15->hFile, g15->bytes, 9, 0, &g15->overlapped) || GetLastError() == ERROR_IO_PENDING)
							continue;
					}
					g15->reading = 0;
				}
				else if (&g15->overlappedWrite == overlapped) {
					//free (g15->buffer);
					//g15->buffer = 0;
					g15->writing = 0;
					continue;
				}
			}
			else {
				/* Close message */
			}
			if (&g15->overlapped == overlapped) {
				g15->reading = 0;
			}
			else if (&g15->overlappedWrite == overlapped) {
				//free (g15->buffer);
				//g15->buffer = 0;
				g15->writing = 0;
			}
			if (g15->hFile) {
				CloseHandle(g15->hFile);
				g15->hFile = 0;
			}
			if (g15->needFree && !g15->reading && !g15->writing) {
				g15->Cleanup();
			}
		}
		else {
			break;
		}
		/*
		int res = WaitForSingleObjectEx(hEvent, INFINITE, 1);
		if (res == WAIT_OBJECT_0) {
			if (readFrom) {
				readFrom->inUse++;
				if (!ReadFileEx(readFrom->hFile, readFrom->bytes, 9, &readFrom->overlapped, G15ReadCompletion)) {
					CloseHandle(readFrom->hFile);
					readFrom->hFile = 0;
				}
				readFrom = 0;
				continue;
			}
			else if (buffer) {
				1
			}
			else {
				closeHandles = 1;
				for (int i=0; i<numG15s; i++) {
					if (G15s[i]->hFile) {
						G15s[i]->needNuke = 1;
						CancelIo(G15s[i]->hFile);
					}
				}
			}
		}
		else {
			if (closeHandles) {
				int i = 0;
				while (i<numG15s) {
					if (G15s[i]->hFile) break;
					G15s[i]->needNuke = 0;
					i++;
				}
				if (i == numG15s) break;
			}
		}
		//*/
	}
	// Cleanup pending operations.
	while (GetQueuedCompletionStatus(hIOPort, &bytes, (ULONG_PTR*)&g15, &overlapped, 0) || GetLastError() != WAIT_TIMEOUT) {
		if (&g15->overlapped == overlapped) {
			g15->reading = 0;
		}
		else if (&g15->overlappedWrite == overlapped) {
			//free (g15->buffer);
			//g15->buffer = 0;
			g15->writing = 0;
		}
		if (g15->hFile) {
			CloseHandle(g15->hFile);
			g15->hFile = 0;
		}
		if (g15->needFree && !g15->reading && !g15->writing) {
			g15->Cleanup();
		}
	}
	return 0;
}

HANDLE GetG15Handle(StringValue *s, G15 *g15=0) {
	HANDLE h;
	if (g15) {
		if (!hThread) return 0;
		if (!g15->hFile) {
			h = CreateFileA((char*)s->value,
						GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
						0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0);
			if (h) {
				memset(&g15->overlapped, 0, sizeof(g15->overlapped));
				memset(&g15->overlappedWrite, 0, sizeof(g15->overlapped));
				g15->reading = 1;
				g15->writing = 0;
				g15->hFile = h;
				if (!CreateIoCompletionPort(h, hIOPort, (ULONG_PTR)g15, 1) || ReadFile(h, g15->bytes, 9, 0, &g15->overlapped) || GetLastError() != ERROR_IO_PENDING) {
					g15->hFile = 0;
					g15->reading = 0;
					CloseHandle(h);
					return 0;
				}
				/*
				readFrom = g15;
				SetEvent(hEvent);
				while (readFrom) {
					Sleep(10);
				}
				/*

				if (!ReadFileEx(h, g15->bytes, 9, &g15->overlapped, G15ReadCompletion)) {
					CloseHandle(h);
					h = 0;
				}
				else {
					g15->inUse++;
				}
				//*/
			}
		}
	}
	else {
		h = CreateFileA((char*)s->value,
					GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
					0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	}
	return h;
}

int ListG15s() {
	HidDevice **oldHids = (HidDevice **) malloc(sizeof(HidDevice*) * numDevices);
	int numOldHids = 0;
	for (int i = 0; i<numDevices; i++) {
		if (devices[i]->cType == CONNECTION_DIRECT) {
			oldHids[numOldHids] = (HidDevice *)devices[i];
			numOldHids++;
		}
	}
	int changed = 0;
	HDEVINFO hdev = SetupDiGetClassDevs(&GUID_DEVINTERFACE_HID, 0, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (hdev != INVALID_HANDLE_VALUE) {
		SP_DEVICE_INTERFACE_DATA devInterfaceData;
		devInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
		int i = 0;
		static DWORD memSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA) + 200;
		ScriptValue sv;
		if (AllocateStringValue(sv, (5+memSize) * 2)) {
			SP_DEVICE_INTERFACE_DETAIL_DATA *devInterfaceDetails = (SP_DEVICE_INTERFACE_DETAIL_DATA *) malloc(memSize);
			if (devInterfaceDetails && sv.stringVal->len) {
				while (SetupDiEnumDeviceInterfaces(hdev, 0, &GUID_DEVINTERFACE_HID, i, &devInterfaceData)) {
					DWORD size = 0;
					devInterfaceDetails->cbSize = sizeof(*devInterfaceDetails);
					SP_DEVINFO_DATA devInfoData;
					devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
					if (!SetupDiGetDeviceInterfaceDetail(hdev, &devInterfaceData, devInterfaceDetails, memSize, &size, &devInfoData)) {
						if (size == memSize || !srealloc(devInterfaceDetails, size+20) || !ResizeStringValue(sv, (5+memSize)*2))
							continue;
						memSize = size+20;
						devInterfaceDetails->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
						if (!SetupDiGetDeviceInterfaceDetail(hdev, &devInterfaceData, devInterfaceDetails, memSize, 0, &devInfoData)) continue;
					}
					HANDLE hfile = CreateFile(devInterfaceDetails->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
					if (hfile != INVALID_HANDLE_VALUE) {
						HIDD_ATTRIBUTES attributes;
						attributes.Size = sizeof(attributes);
						if (HidD_GetAttributes(hfile, &attributes)) {
							int type = sizeof(DeviceTypes)/sizeof(DeviceTypes[0])-1;
							for (; type >= 0; type --) {
								if (attributes.VendorID == DeviceTypes[type].vid &&
									attributes.ProductID == DeviceTypes[type].pid) break;
							}
							if (type >= 0) {
								PHIDP_PREPARSED_DATA pData;
								HIDP_CAPS caps;
								if (HidD_GetPreparsedData(hfile, &pData)) {
									if (HIDP_STATUS_SUCCESS == HidP_GetCaps(pData, &caps)) {
										if (caps.FeatureReportByteLength == DeviceTypes[type].featureReportBytes && caps.OutputReportByteLength == DeviceTypes[type].outputReportBytes && caps.InputReportByteLength == DeviceTypes[type].inputBytes) {
											int j = 0;
											sv.stringVal->len = UTF16toUTF8(sv.stringVal->value, devInterfaceDetails->DevicePath);
											for (;j<numOldHids; j++) {
												if (!scriptstricmp(sv.stringVal, oldHids[j]->id))
													break;
											}
											if (j < numOldHids) {
												oldHids[j] = oldHids[--numOldHids];
											}
											else {
												G15 *g15 = (G15*) calloc(sizeof(G15), 1);
												if (g15 && CreateStringValue(g15->name, sv.stringVal->value, sv.stringVal->len)) {
													changed = 1;
													g15->deviceTypeIndex = type;
													AddDevice(new HidDevice(g15, DeviceTypes[type].width, DeviceTypes[type].height, DeviceTypes[type].bpp));
												}
												else {
													free(g15);
												}
											}
										}
									}
									HidD_FreePreparsedData(pData);
								}
							}
						}
						if (hfile) CloseHandle(hfile);
					}
					i++;
				}
				free(devInterfaceDetails);
			}
			sv.stringVal->Release();
		}
		//errorPrintf(0, "\n");
		SetupDiDestroyDeviceInfoList(hdev);
	}
	for (int i=0; i<numOldHids; i++) {
		changed = 1;
		if (hThread && oldHids[i]->g15->hFile) {
			oldHids[i]->g15->needFree = 1;
			PostQueuedCompletionStatus(hIOPort, 0, (ULONG_PTR)oldHids[i]->g15, 0);
		}
		else
			oldHids[i]->g15->Cleanup();
		DeleteDevice(oldHids[i]);
	}
	free(oldHids);
	if (g15enabledCount) {
		for (int i=0; i<numDevices; i++) {
			if (devices[i]->cType != CONNECTION_DIRECT) continue;
			HidDevice *hid = (HidDevice*)devices[i];
			if (!hid->g15->hFile) {
				hid->g15->hFile = GetG15Handle(hid->g15->name.stringVal, hid->g15);
				/*
				char temp[90];
				OVERLAPPED overlapped;
				memset(&overlapped, 0, sizeof(overlapped));

				//HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
				//overlapped.hEvent = hEvent;

				while (1) {
					DWORD read = 0;
					int res = ReadFile(G15s[i]->hFile, temp, 9, &read, 0);
					//DWORD dw = WaitForSingleObject(hEvent, 20000);
					res = res;
					if (res || read) {
						read = read;
					}
				}//*/
			}
		}
	}
	return changed;
}

void InitHID() {
	HidD_GetHidGuid(&GUID_DEVINTERFACE_HID);
	ListG15s();
}

void UninitHID() {
	int t = 0;
	while (g15enabledCount) {
		ScriptValue s;
		G15DisableHID(s, &s);
	}
	for (int i=numDevices-1; i>=0; i--) {
		if (devices[i]->cType != CONNECTION_DIRECT) continue;
		((HidDevice*)devices[i])->g15->Cleanup();
		DeleteDevice(devices[i]);
	}
}

void UpdateHID() {
	ListG15s();
}


int G15sSetFeature(unsigned char* bytes, int size, ScriptValue &kb) {
	//HDEVINFO hdev = SetupDiGetClassDevs(&GUID_DEVINTERFACE_HID, 0, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	int count = 0;
	int index = GetG15Index(&kb);
	if (index<0) return 0;
	if (!index) {
		for (int i=0; i<numDevices; i++) {
			if (devices[i]->cType != CONNECTION_DIRECT) continue;
			G15 *g15 = ((HidDevice*)devices[i])->g15;
			if (DeviceTypes[g15->deviceTypeIndex].featureReportBytes != size) continue;
			HANDLE hFile = g15->hFile;
			if (!hFile) {
				hFile = GetG15Handle(g15->name.stringVal);
			}
			if (hFile) {
				count += HidD_SetFeature(hFile, bytes, size);
				if (!g15->hFile) {
					CloseHandle(hFile);
				}
			}
		}
	}
	else {
		index --;
		if (devices[index]->cType == CONNECTION_DIRECT) {
			G15 *g15 = ((HidDevice*)devices[index])->g15;
			if (DeviceTypes[g15->deviceTypeIndex].featureReportBytes == size) {
				HANDLE hFile = g15->hFile;
				if (!hFile) {
					hFile = GetG15Handle(g15->name.stringVal);
				}
				if (hFile) {
					count = HidD_SetFeature(hFile, bytes, size);
					if (!g15->hFile) {
						CloseHandle(hFile);
					}
				}
			}
		}
	}

	return count;
}


G19State *GetG19State(Device *dev) {
	if (dev->hidType != HID_LCD_G19) return 0;
	G15 * g15 = dev->g15;
	G19State *out = 0;
	static G19State state;
	unsigned char res[2][258];
	res[0][0] = 5;
	res[1][0] = 7;
	HANDLE hFile = g15->hFile;
	if (!hFile) hFile = GetG15Handle(g15->name.stringVal);
	if (hFile) {
		if (HidD_GetFeature(hFile, res[0], 258) && HidD_GetFeature(hFile, res[1], 258)) {
			state.MLights = (res[0][1]>>7) + ((res[0][1]>>5)&2) + ((res[0][1]>>3)&4) + ((res[0][1]>>1)&8);
			state.color.a = 0xFF;
			state.color.r = res[1][1];
			state.color.g = res[1][2];
			state.color.b = res[1][3];
			out = &state;
		}
		/*
		res[1] = 0x20;
		if (HidD_SetFeature(hFile, (unsigned char*)&res, 258)) {
			out = 0;
		}//*/
		if (!g15->hFile) {
			CloseHandle(hFile);
		}
	}
	return out;
}


G15State *GetG15State(Device *dev) {
	if (dev->hidType >= HID_LCD_G19) return 0;
	G15 * g15 = dev->g15;
	G15State *out = 0;
	static G15State state;
	state.junk = 2;
	HANDLE hFile = g15->hFile;
	if (!hFile) hFile = GetG15Handle(g15->name.stringVal);
	if (hFile) {
		if (HidD_GetFeature(hFile, (unsigned char*)&state, 4)) {
			out = &state;
		}
		if (!g15->hFile) {
			CloseHandle(hFile);
		}
	}
	return out;
}

G15State *GetG15State(ScriptValue &kb, ScriptValue &name) {
	//unsigned char t[4] = {2, 32, 129, 16};
	/*while (1) {
		G15sSetFeature(t, 4, 0);
		t[3] = 22;
	}//*/
	int index = GetG15Index(&kb);
	if (index < 0) return 0;
	if (index) index--;
	else {
		for (int i=0; i<numDevices; i++) {
			if (devices[i]->cType == CONNECTION_DIRECT) {
				index = i;
				break;
			}
		}
	}
	if (devices[index]->cType != CONNECTION_DIRECT) return 0;
	G15 *g15 = devices[index]->g15;
	if (DeviceTypes[g15->deviceTypeIndex].featureReportBytes != 4) return 0;
	name = g15->name;
	return GetG15State(devices[index]);
}

void GetG15s(ScriptValue &s, ScriptValue *args) {
	int type = args[0].i32;
	if (CreateListValue(s, numDevices+2)) {
		for (int i=0; i<numDevices; i++) {
			int add = 0;
			if (type == 0 || (devices[i]->cType == CONNECTION_DIRECT && type-1 == ((HidDevice*)devices[i])->g15->deviceTypeIndex) ||
				(type == SDK_160_43_1 && devices[i]->cType != CONNECTION_DIRECT && devices[i]->height == 43) ||
				(type == SDK_320_240_32 && devices[i]->cType != CONNECTION_DIRECT && devices[i]->height == 240)) {
					devices[i]->id->AddRef();
					ScriptValue sv;
					CreateStringValue(sv, devices[i]->id);
					s.listVal->PushBack(sv);
			}
		}
	}
}

void G15EnableHID(ScriptValue &s, ScriptValue *args) {
	if (!(g15enabledCount++)) {
		//if (!hEvent) hEvent = CreateEvent(0,0,0,0);
		hIOPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, hIOPort, 0, 1);
		if (!hIOPort) {
			g15enabledCount--;
			return;
		}

		hThread = CreateThread(0,0x1000,ThreadProc,0,0,0);
		for (int i=0; i<numDevices; i++) {
			if (devices[i]->cType != CONNECTION_DIRECT) continue;
			G15* g15 = ((HidDevice*)devices[i])->g15;
			g15->hFile = GetG15Handle(g15->name.stringVal, g15);
		}
	}
	CreateIntValue(s, g15enabledCount);
}

#ifdef GOAT
void G15SendImage() {
	if (g15enabledCount && numG15s) {
		int start = 0;
		while (G15s[start]->writing || !G15s[start]->hFile) {
			start++;
			if (start == numG15s) return;
		}
		Buffer *buffer = &G15s[start]->buffer;
		if (buffer) {
			unsigned char *junk = buffer->junk;
			memset(junk+1, 0, 31);
			memset(junk + (160/8)*6+32, 0, sizeof(Buffer)-((160/8)*6+32));
			junk[0] = 3;
			//memcpy(junk+32, screen.bmp.data, (43*160)/8);
			//junk[32] = 0xFF;
			//junk[35] = 0xFF;
			/*
			for (int y = 0; y<40; y+=8) {
				int offset = 32 + y*(160/8);
				for (int x=0; x<160;x++) {
					int xSource = x/32+y*(160/32);
					int shift = x%32;
					junk[offset+x] = ((screen.bmp.data[xSource]>>shift)&1) +
						(((screen.bmp.data[xSource+  160/32]>>shift)&1)<<1) +
						(((screen.bmp.data[xSource+2*160/32]>>shift)&1)<<2) +
						(((screen.bmp.data[xSource+3*160/32]>>shift)&1)<<3) +
						(((screen.bmp.data[xSource+4*160/32]>>shift)&1)<<4) +
						(((screen.bmp.data[xSource+5*160/32]>>shift)&1)<<5) +
						(((screen.bmp.data[xSource+6*160/32]>>shift)&1)<<6) +
						(((screen.bmp.data[xSource+7*160/32]>>shift)&1)<<7);
				}
			}//*/
#ifdef GOAT
			for (int y = 0; y<40; y+=8) {
				int offset = 32 + y*(160/8);
				for (int x=0; x<160/8;x++) {
					int xSource = x+offset-32;
					unsigned int pairs1 = ((((unsigned char*)screen.bmp.data)[xSource+7*160/8]<<16) + (((unsigned char*)screen.bmp.data)[xSource+7*160/8]<<7) & ((0xAA<<16) + (0x55<<7))) +
						((((unsigned char*)screen.bmp.data)[xSource+6*160/8]<<15) + (((unsigned char*)screen.bmp.data)[xSource+6*160/8]<<6) & ((0xAA<<15) + (0x55<<6)));
					unsigned int pairs2 = ((((unsigned char*)screen.bmp.data)[xSource+5*160/8]<<14) + (((unsigned char*)screen.bmp.data)[xSource+5*160/8]<<5) & ((0xAA<<14) + (0x55<<5))) +
						((((unsigned char*)screen.bmp.data)[xSource+4*160/8]<<13) + (((unsigned char*)screen.bmp.data)[xSource+4*160/8]<<4) & ((0xAA<<13) + (0x55<<4)));
					unsigned int pairs3 = ((((unsigned char*)screen.bmp.data)[xSource+3*160/8]<<12) + (((unsigned char*)screen.bmp.data)[xSource+3*160/8]<<3) & ((0xAA<<12) + (0x55<<3))) +
						((((unsigned char*)screen.bmp.data)[xSource+2*160/8]<<11) + (((unsigned char*)screen.bmp.data)[xSource+2*160/8]<<2) & ((0xAA<<11) + (0x55<<2)));
					unsigned int pairs4 = ((((unsigned char*)screen.bmp.data)[xSource+1*160/8]<<10) + (((unsigned char*)screen.bmp.data)[xSource+1*160/8]<<1) & ((0xAA<<10) + (0x55<<1))) +
						((((unsigned char*)screen.bmp.data)[xSource+0*160/8]<< 9) + (((unsigned char*)screen.bmp.data)[xSource+0*160/8]<<0) & ((0xAA<< 9) + (0x55<<0)));
					unsigned int quad1 = (pairs1&((0xCC<<16)+(0xCC<<6))) + (pairs2&((0xCC<<14)+(0xCC<<4)));
					unsigned int quad2 = (pairs3&((0xCC<<12)+(0xCC<<2))) + (pairs4&((0xCC<<10)+(0xCC<<0)));
					unsigned int oct = (quad1 & ((0xF0<<16) + (0xF0<<6))) + (quad2 & ((0xF0<<12) + (0xF0<<2)));
					unsigned int oct2 = (quad1 & ((0xF0<<12) + (0xF0<<2))) + (quad2 & ((0x0F<<12) + (0x0F<<2)));
					quad1 = (pairs1&((0xCC<<14)+(0xCC<<4))) + (pairs2&((0xCC<<12)+(0xCC<<2)));
					quad2 = (pairs3&((0xCC<<10)+(0xCC<<0))) + (pairs4&((0xCC<< 8)+(0xCC>>2)));
					unsigned int oct3 = (quad1 & ((0xF0<<14) + (0xF0<<4))) + (quad2 & ((0xF0<<10) + (0xF0<<0)));
					unsigned int oct4 = (quad1 & ((0xF0<<10) + (0xF0<<0))) + (quad2 & ((0x0F<<10) + (0x0F<<0)));
					/*if (junk[offset+x*8+7] != (unsigned char) (oct>>16)) {
						offset=offset;
					}
					if (junk[offset+x*8+6] != (unsigned char) (oct>>6)) {
						offset=offset;
					}
					if (junk[offset+x*8+3] != (unsigned char) (oct2>>12)) {
						offset=offset;
					}
					if (junk[offset+x*8+2] != (unsigned char) (oct2>>2)) {
						offset=offset;
					}
					if (junk[offset+x*8+5] != (unsigned char) (oct3>>14)) {
						offset=offset;
					}
					if (junk[offset+x*8+4] != (unsigned char) (oct3>>4)) {
						offset=offset;
					}
					if (junk[offset+x*8+1] != (unsigned char) (oct4>>10)) {
						offset=offset;
					}
					if (junk[offset+x*8+0] != (unsigned char) (oct4>>0)) {
						offset=offset;
					}
					//*/
					*(unsigned int*)&junk[offset+8*x] =
						((oct4&0xFF) + ((oct4>>2)&0xFF00)) + (((oct2<<14)&0xFF0000) + ((oct2<<12)&0xFF000000));
					*(unsigned int*)&junk[offset+8*x+4] =
						(((oct3>>4)&0xFF) + ((oct3>>6)&0xFF00)) + (((oct<<10)&0xFF0000) + ((oct<<8)&0xFF000000));
					/*
					junk[offset+x*8+7] = (unsigned char) (oct>>16);
					junk[offset+x*8+6] = (unsigned char) (oct>>6);
					junk[offset+x*8+3] = (unsigned char) (oct2>>12);
					junk[offset+x*8+2] = (unsigned char) (oct2>>2);
					//junk[offset+x*8+3] = (unsigned char) (oct>>6);
					//*/
				}
			}
			/*
			for (int y = 0; y<40; y+=8) {
				int offset = 32 + y*(160/8);
				for (int x=0; x<160;x++) {
					int xSource = x/32+y*(160/32);
					int shift = x%32;
					junk[offset+x] = ((screen.bmp.data[xSource]>>shift)&1) +
						(((screen.bmp.data[xSource+  160/32]>>shift)&1)<<1) +
						(((screen.bmp.data[xSource+2*160/32]>>shift)&1)<<2) +
						(((screen.bmp.data[xSource+3*160/32]>>shift)&1)<<3) +
						(((screen.bmp.data[xSource+4*160/32]>>shift)&1)<<4) +
						(((screen.bmp.data[xSource+5*160/32]>>shift)&1)<<5) +
						(((screen.bmp.data[xSource+6*160/32]>>shift)&1)<<6) +
						(((screen.bmp.data[xSource+7*160/32]>>shift)&1)<<7);
				}
			}//*/
			int offset = 32 + 40*(160/8);
			for (int x=0; x<160;x+=4) {
				int xSource = x/32+40*(160/32);
				int shift = x%32;
				*(unsigned int*)&junk[offset+x] =
					(((screen.bmp.data[xSource]>>shift)&1) +
					 (((screen.bmp.data[xSource+  160/32]>>shift)&1)<<1) +
					 (((screen.bmp.data[xSource+2*160/32]>>shift)&1)<<2)) +
					((((screen.bmp.data[xSource]>>(shift+1))&1) +
					  (((screen.bmp.data[xSource+  160/32]>>(shift+1))&1)<<1) +
					  (((screen.bmp.data[xSource+2*160/32]>>(shift+1))&1)<<2))<<8)+
					((((screen.bmp.data[xSource]>>(shift+2))&1) +
					  (((screen.bmp.data[xSource+  160/32]>>(shift+2))&1)<<1) +
					  (((screen.bmp.data[xSource+2*160/32]>>(shift+2))&1)<<2))<<16)+
					((((screen.bmp.data[xSource]>>(shift+3))&1) +
					  (((screen.bmp.data[xSource+  160/32]>>(shift+3))&1)<<1) +
					  (((screen.bmp.data[xSource+2*160/32]>>(shift+3))&1)<<2))<<24);
			}
#endif

			for (int i=start; i<numG15s;i++) {
				if (G15s[i]->hFile && !(G15s[i]->writing)) {
					if (i!=start) memcpy(&G15s[i]->buffer, &G15s[start]->buffer, sizeof(Buffer));
					G15s[i]->writing = 1;
					memset(&G15s[i]->overlappedWrite, 0, sizeof(OVERLAPPED));
					if (!WriteFile(G15s[i]->hFile, G15s[i]->buffer.junk, sizeof(Buffer), 0, &G15s[i]->overlappedWrite) && GetLastError() != ERROR_IO_PENDING) {
						G15s[i]->writing = 0;
					}
				}
				/*
				else {
					if (i==numG15s-1) free(buffer);
				}
				/*
				if (i < numG15s-1) {
				}
				int q;
				if (G15s[i]->hFile) {
					q=WriteFile(G15s[i]->hFile, junk, sizeof(junk), 0, &G15s[i]->overlappedWrite);
					int w = GetLastError();
					w=w;
				}
				/*if (G15s[i]->hFile && G15s[i]->inUse < 2 && (q=WriteFileEx(G15s[i]->hFile, junk, sizeof(junk), &G15s[i]->overlappedWrite, G15WriteCompletion))) {
					int w = GetLastError();
					if (!w)
						G15s[i]->inUse++;
				}//*/
			}
		}
	}
}
#endif
