#ifndef X64
#ifdef _DEBUG
//#include <vld.h>
#endif
#endif

//#include <crtdbg.h>

#include <INITGUID.H>

#include "global.h"
#include "Device.h"
#include "http/httpGetPage.h"
#include "stringUtil.h"
#include "ScriptObjects/ScriptObjects.h"
#include "ScriptObjects/MediaFile.h"
#include "ScriptObjects/Socket.h"
#include "ScriptProcs/ScriptMisc.h"
#include "G15Util/G15HID.h"
#include "util.h"
#include "ScriptObjects/SysTrayIcon.h"
#include "plugin/plugins.h"

// Unique identifier for use with COPY_DATA.
// Does not currently matter.
#define EXEC_MESSAGE ((__int64)0x6543263461789675)

HWND nextClipboardHwnd = 0;

#include "Timer.h"

#include "Screen.h"
#include "Config.h"
#include "PerfMon.h"

#include "Network.h"
#include "ShortCut.h"
#include "vm.h"
#include "ScriptProcs/networking/ScriptIP.h"
#include "ScriptProcs/Event/Event.h"

#include "ScriptProcs/Event/Keyboard.h"
#include "ScriptProcs/Audio.h"

PerfMon perfMon;

void QueueRedraw() {
	PostMessage(ghWnd, WM_APP, 2, 2);
}

void RedrawNow(StringValue *dev) {
	((Device*)dummyScreen)->needRedraw = 1;
	for (int i=0; i<numDevices; i++) {
		if (dev && scriptstricmp(dev, devices[i]->id)) continue;
		if (devices[i]->cType == CONNECTION_SYSTRAY_ICON) continue;
		devices[i]->needRedraw = 1;
	}
	if (activeScreen == dummyScreen)
		SendMessage(ghWnd, WM_APP, 2, 1);
}

void Redraw(StringValue *dev) {
	int changed = 0;
	for (int i=0; i<numDevices; i++) {
		if (dev && scriptstricmp(dev, devices[i]->id)) continue;
		if (devices[i]->cType == CONNECTION_SYSTRAY_ICON) continue;
		changed |= !devices[i]->needRedraw;
		devices[i]->needRedraw = 1;
	}
	if (changed) {
		QueueRedraw();
	}
}

HWND nextClipboardListener = 0;

char *gClassName = APP_NAME;

DWORD ddeID = 0;

HWND ghWnd = 0;
int quitting = 0;


void Quit() {
	quitting = 1;
	PostMessage(ghWnd, WM_CLOSE, 0, 0);
}

int lastTimes[4] = {0,0,0,0};

bool gOnTray = 0;
bool noLCD = 0;

/*int volChangeProc;
int ipChangeProc;
int counterUpdateProc;
int foregroundWindowChangeProc;
int screenSaverProc;
*/

static int LoadTrayIcon(HWND hWnd) {

	NOTIFYICONDATA nData;
	memset(&nData, 0, sizeof(NOTIFYICONDATA));
	
	nData.cbSize = sizeof(NOTIFYICONDATA);
	nData.hWnd = hWnd;
	nData.uID = IDI_FROG;
	nData.hIcon = LoadIcon(ghInst, MAKEINTRESOURCE(IDI_FROG));
	nData.uCallbackMessage = WMA_ICON_MESSAGE;
	wcscpy(nData.szTip, L"LCD Miscellany");
	nData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;

	if(Shell_NotifyIcon(NIM_ADD, &nData) == 0) {
		return 0;
	}
	gOnTray = 1;

	return 1;
}

static void RemoveTrayIcon() {
	if (gOnTray) {
		gOnTray = 0;
		
		NOTIFYICONDATA nData;
		
		nData.cbSize = sizeof(nData);
		nData.hWnd = ghWnd;
		nData.uID = IDI_FROG;
		
		Shell_NotifyIcon(NIM_DELETE, &nData);
	}
}

char buttonTimerActive = 0;


/*
struct Chip8 {
	unsigned char memory[0x1000];
	unsigned char regs[16];
	unsigned short cp;
	unsigned short I;
	unsigned short codeLen;
	unsigned short stack[32];
	unsigned int lastTime;
	char stackPtr;
	unsigned char delayTimer;
	unsigned char soundTimer;
	int Tick();
	// bytes
	unsigned char width;
	// pixels
	unsigned char height;
	unsigned char screen[16*64];
	unsigned char overflow[16];
	unsigned short keys;
};

Chip8 *LoadChip8(unsigned char *f) {
	FILE *in = fopen((char*)f, "rb");
	if (!in) return 0;
	Chip8 *out = (Chip8*) calloc(1,sizeof(Chip8));
	if (!out) {
		fclose(in);
		return 0;
	}
	out->codeLen = (unsigned short)fread(out->memory+0x200, 1, sizeof(out->memory)-0x200, in);
	fclose(in);
	if (!out->codeLen) {
		free(out);
		return 0;
	}
	out->stackPtr = -1;
	out->cp = 0x200;
	out->lastTime = timeGetTime();
	static unsigned char font[] = {
		0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
		0x20, 0x60, 0x20, 0x20, 0x70, // 1
		0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
		0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
		0x90, 0x90, 0xF0, 0x10, 0x10, // 4
		0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
		0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
		0xF0, 0x10, 0x20, 0x40, 0x40, // 7
		0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
		0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
		0xF0, 0x90, 0xF0, 0x90, 0x90, // A
		0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
		0xF0, 0x80, 0x80, 0x80, 0xF0, // C
		0xE0, 0x90, 0x90, 0x90, 0xE0, // D
		0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
		0xF0, 0x80, 0xF0, 0x80, 0x80, // F
	};
    static unsigned char sfont[] = {
		 0x3C, 0x7E, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0x7E, 0x3C, // 0
		 0x18, 0x38, 0x58, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, // 1
		 0x3E, 0x7F, 0xC3, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xFF, 0xFF, // 2
		 0x3C, 0x7E, 0xC3, 0x03, 0x0E, 0x0E, 0x03, 0xC3, 0x7E, 0x3C, // 3
		 0x06, 0x0E, 0x1E, 0x36, 0x66, 0xC6, 0xFF, 0xFF, 0x06, 0x06, // 4
		 0xFF, 0xFF, 0xC0, 0xC0, 0xFC, 0xFE, 0x03, 0xC3, 0x7E, 0x3C, // 5
		 0x3E, 0x7C, 0xC0, 0xC0, 0xFC, 0xFE, 0xC3, 0xC3, 0x7E, 0x3C, // 6
		 0xFF, 0xFF, 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x60, 0x60, // 7
		 0x3C, 0x7E, 0xC3, 0xC3, 0x7E, 0x7E, 0xC3, 0xC3, 0x7E, 0x3C, // 8
		 0x3C, 0x7E, 0xC3, 0xC3, 0x7F, 0x3F, 0x03, 0x03, 0x3E, 0x7C, // 9
	};
	memcpy(out->memory, font, sizeof(font));
	memcpy(out->memory+sizeof(font), sfont, sizeof(sfont));
	out->width = 8;
	out->height = 32;
	return out;
}


int Chip8::Tick() {
	int error = 0;
	if (soundTimer | delayTimer) {
		unsigned int t = timeGetTime();
		if (t-lastTime >= 1000/60) {
			if (soundTimer) soundTimer --;
			if (delayTimer) delayTimer --;
			lastTime = t;
		}
	}
	if (cp >= 0x999) cp = 0;
	keys = 0;
	for (int i=0; i<16; i++) {
		static char map[16] = {'X', '1', '2', '3', 'Q', 'W', 'E', 'A', 'S', 'D', 'Z', 'C', '4', 'R', 'F', 'V'};
		keys += ((1&(GetKeyState(map[i])>>15))<<i);
		if (keys) {
			keys = keys;
		}
	}
	switch (memory[cp]&0xF0) {
		case 0x00:
			if (memory[cp]) {
				error = 4;
				break;
			}
			if (memory[cp+1] == 0xE0) {
				// clear.
				memset(screen, 0, width*height);
				error = -1;
			}
			else if (memory[cp+1] == 0xEE) {
				// ret
				if (stackPtr >= 0) {
					cp = stack[stackPtr--];
				}
				else {
					error = 3;
				}
			}
			else if ((memory[cp+1]&0xF0) == 0xC0) {
				// Scroll down.
				int dy = memory[cp+1]&0xF;
				memmove(screen+dy*width, screen, (height-dy)*width);
				memset(screen, 0, dy*width);
				error = -1;
			}
			else if (memory[cp+1] == 0xFB) {
				// Scroll right 4 pixels.
				int xw = width/4-1;
				for (int y=0; y<height; y++) {
					unsigned int *row = (unsigned int*) (screen+y*width);
					row[xw] <<= 4;
					for (int x = xw-1; x>= 0; x--) {
						row[x+1] += (row[x]>>28);
						row[x]<<=4;
					}
				}
			}
			else if (memory[cp+1] == 0xFC) {
				// Scroll left 4 pixels.
				int xw = width/4-1;
				for (int y=0; y<height; y++) {
					unsigned int *row = (unsigned int*) (screen+y*width);
					row[0] >>= 4;
					for (int x = 1; x <= xw; x++) {
						row[x-1] += (row[x]<<28);
						row[x]>>=4;
					}
				}
			}
			else if (memory[cp+1] == 0xFE) {
				// Small screen.  Doesn't clear.
				width = 64/8;
				height = 32;
			}
			else if (memory[cp+1] == 0xFF) {
				// Large screen.  Doesn't clear.
				width = 128/8;
				height = 64;
			}
			else {
				// ???
				error = 2;
			}
			break;
		case 0x10:
			// jmp
			cp = ((memory[cp]&0xF)<<8) + memory[cp+1]-2;
			break;
		case 0xB0:
			// jmp + v0
			cp = regs[0] + ((memory[cp]&0xF)<<8) + memory[cp+1]-2;
			break;
		case 0x20:
			// call addr.
			if (stackPtr < 31) {
				stack[++stackPtr] = cp;
				cp = ((memory[cp]&0xF)<<8) + memory[cp+1]-2;
			}
			else {
				error = 1;
			}

			break;
		case 0x30:
			// Skip if Vx == byte.
			if (regs[memory[cp]&0x0F] == memory[cp+1]) cp+=2;
			break;
		case 0x40:
			// Skip if Vx != byte.
			if (regs[memory[cp]&0x0F] != memory[cp+1]) cp+=2;
			break;
		case 0x50:
			// Skip if Vx == Vy.
			if (regs[memory[cp]&0x0F] == regs[memory[cp+1]>>4]) cp+=2;
			break;
		case 0x90:
			// Skip if Vx != Vy.
			if (regs[memory[cp]&0x0F] != regs[memory[cp+1]>>4]) cp+=2;
			break;
		case 0x60:
			// LD Vx, byte.
			regs[memory[cp]&0x0F] = memory[cp+1];
			break;
		case 0x70:
			// Add Vx, byte.  no carry
			regs[memory[cp]&0x0F] += memory[cp+1];
			break;
		case 0x80:
			// 2-register Math
			{
				unsigned char r1 = memory[cp] & 0xF;
				unsigned char r2 = memory[cp+1]>>4;
				unsigned short carry;
				switch (memory[cp+1] & 0xF) {
					case 0x0:
						regs[r1] = regs[r2];
						break;
					case 0x1:
						regs[r1] |= regs[r2];
						break;
					case 0x2:
						regs[r1] &= regs[r2];
						break;
					case 0x3:
						regs[r1] ^= regs[r2];
						break;
					case 0x4:
						carry = regs[r1] + (unsigned short) regs[r2];
						// Not sure if order is right.
						regs[0xF] = (unsigned char)(carry>>8);
						regs[r1] = (unsigned char) carry;
						break;
					case 0x5:
						carry = regs[r1] - regs[r2];
						regs[0xF] = (unsigned char)!(carry>>8);
						regs[r1] = (unsigned char) carry;
						break;
					case 0x6:
						carry = regs[r1];
						regs[0xF] = 1&(unsigned char) carry;
						regs[r1] = ((unsigned char) carry)>>1;
						break;
					case 0x7:
						carry = regs[r2] - regs[r1];
						regs[0xF] = (unsigned char)!(carry>>8);
						regs[r1] = (unsigned char) carry;
						break;
					case 0xE:
						carry = regs[r1];
						regs[0xF] = (unsigned char)(carry>>7);
						regs[r1] = (unsigned char) (carry<<1);
						break;
					default:
						error = -3;
						break;
				}
			}
			break;
		case 0xA0:
			// LD I, imm24.
			I = ((memory[cp]&0x0F)<<8) + memory[cp+1];
			break;
		case 0xC0:
			// random & kk.
			regs[memory[cp]&0x0F] = memory[cp+1] & rand();
			break;
		case 0xD0:
			{
				// Draw pixels
				unsigned int h = memory[cp+1]&0x0F;
				unsigned int x = regs[memory[cp]&0x0F];
				unsigned int y = regs[memory[cp+1]>>4];
				if (x >= (unsigned char)(width*8)) {
					y += x/(width*8);
					x %= (width*8);
				}
				unsigned int xp = x/8;
				unsigned int shift = x%8; 
				regs[0xF] = 0;
				if (h) {
					for (unsigned int p = 0; p<h; p++) {
						if (y >= height) break;
						unsigned char byte = memory[(p+I)&0xFFF];
						unsigned int flipped =
							(((byte&0x01)<<7) +
							 ((byte&0x02)<<5) +
							 ((byte&0x04)<<3) +
							 ((byte&0x08)<<1) +
							 ((byte&0x10)>>1) +
							 ((byte&0x20)>>3) +
							 ((byte&0x40)>>5) +
							 ((byte&0x80)>>7)) << shift;
						unsigned int *pt = (unsigned int*)&screen[y*width+xp];
						*pt ^= flipped;
						regs[0xF] |= (((*pt)&flipped) != flipped);
						y++;
					}
				}
				else {
					for (unsigned int p = 0; p<32; p+=2) {
						if (y >= height) break;
						unsigned char byte = memory[(p+I)&0xFFF];
						unsigned char byte2 = memory[(p+I+1)&0xFFF];
						unsigned int flipped =
							((((byte2&0x01)<<7) +
							  ((byte2&0x02)<<5) +
							  ((byte2&0x04)<<3) +
							  ((byte2&0x08)<<1) +
							  ((byte2&0x10)>>1) +
							  ((byte2&0x20)>>3) +
							  ((byte2&0x40)>>5) +
							  ((byte2&0x80)>>7))*256 +
							 (((byte&0x01)<<7) +
							  ((byte&0x02)<<5) +
							  ((byte&0x04)<<3) +
							  ((byte&0x08)<<1) +
							  ((byte&0x10)>>1) +
							  ((byte&0x20)>>3) +
							  ((byte&0x40)>>5) +
							  ((byte&0x80)>>7))) << shift;
						unsigned int *pt = (unsigned int*)&screen[y*width+xp];
						*pt ^= flipped;
						regs[0xF] |= (((*pt)&flipped) != flipped);
						y++;
					}
				}
				error = -1;
			}
			break;
		case 0xF0:
			{
				unsigned char v = memory[cp]&0x0F;
				switch (memory[cp+1]) {
					case 0x07:
						regs[v] = delayTimer;
						break;
					case 0x0A:
						{
							// wait for key - do nothing unless key is down.
							int i;
							for (i=0; i<16; i++) {
								if (1&(keys>>i)) break;
							}
							if (i == 16) return -4;
							regs[v] = i;
							break;
						}
					case 0x15:
						// LD DT, Vx
						delayTimer = regs[v];
						break;
					case 0x18:
						// LD ST, Vx
						soundTimer = regs[v];
						break;
					case 0x1E:
						I = ((I+regs[v])&0xFFF);
						break;
					case 0x29:
						// LD [I], Font(Vx)
						I = 5*regs[v];
						break;
					case 0x30:
						// LD [I], sfont(Vx)
						I = 5*16+10*regs[v];
						break;
					case 0x33:
						// LD [I], BCD(Vx)
						memory[I] = regs[v]/100;
						memory[(I+1)&0xFFF] = (regs[v]/10)%10;
						memory[(I+2)&0xFFF] = regs[v]%10;
						break;
					case 0x55:
						// LD [I], V0-x
						for (int w=0; w<=v; w++) {
							memory[(I+w)&0xFFF] = regs[w];
						}
						break;
					case 0x65:
						// LD V0-x, [I]
						for (int w=0; w<=v; w++) {
							regs[w] = memory[(I+w)&0xFFF];
						}
						break;
					default:
						error = 7;
						break;
				}
			}
			break;
		case 0xE0:
			{
				unsigned char r1 = memory[cp]&0xF;
				unsigned short i = (keys>>regs[r1])&1;
				if (memory[cp+1] == 0x9E) {
					if (i) {
						cp+=2;
					}
				}
				else if (memory[cp+1] == 0xA1) {
					if (!i) {
						cp+=2;
					}
				}
				else {
					error = 5;
				}
			}
			break;
		default:
			error = 6;
			break;
	}
	cp += 2;
	return error;
}
//*/

static int FlushImage(int drawn) {
	static unsigned __int64 lastDrawn = 0;
	static unsigned __int64 lastFlushed = 0;
	static unsigned __int64 lastDeviceEnum = 0;
	if (noLCD) return 0;
	int errors = 0;
	for (int i = 0; i<numDevices; i ++) {
		if (devices[i]->sendImage) {
			if (devices[i]->UpdateImage() < 0) {
				errors ++;
				DeleteDevice(i);
				i--;
			}
		}
	}

	unsigned __int64 t = GetTickCountNoOverflow();
	// Reenumerate if errors or if not connected to the 3.01 SDK or it's been a while.
	// 3.01 SDK tells me when to enumerate.
	if (errors || (t - lastDeviceEnum >= 5000 && (sdkType != CONNECTION_SDK301 || t - lastDeviceEnum >= 20000))) {
		lastDeviceEnum = t;
		UpdateSDKDevices();
	}
	if (drawn) {
		if (t - lastFlushed <= 70) {
			SetTimer(ghWnd, 3, 80, 0);
		}
		lastDrawn = t;
	}
	else {
		if (t - lastDrawn >= 70) {
			KillTimer(ghWnd, 3);
		}
	}
	lastFlushed = t;
	return 1;
}


static void Draw() {
	// Shouldn't happen, but just in case...
	if (activeScreen != dummyScreen) return;
	static unsigned  __int64 lastDraw = 0;
	unsigned __int64 tick = time64i();
	int redrawn = 0;

	int SDKConnections = 0;
	int needDummy = 1;
	for (int i=0; i<numDevices; i++) {
		SDKConnections += (devices[i]->cType == CONNECTION_SDK103 || devices[i]->cType == CONNECTION_SDK301);
	}

	for (int i=0; i<numDevices; i++) {
		if (devices[i]->cType != CONNECTION_SYSTRAY_ICON) {
			needDummy = 0;
		}
		if (devices[i]->cType == CONNECTION_DIRECT && SDKConnections) {
			devices[i]->sendImage = 0;
			continue;
		}
		if (devices[i]->needRedraw) {
			devices[i]->sendImage = 1;
			devices[i]->needRedraw = 0;
			ScriptValue sv;
			ScriptValue sv2;
			activeScreen = devices[i];
			if (CreateListValue(sv2, 4)) {
				sv2.listVal->PushBack(devices[i]->width);
				sv2.listVal->PushBack(devices[i]->height);
				sv2.listVal->PushBack(devices[i]->bpp);
				sv2.listVal->PushBack(devices[i]->cType);
			}
			if (devices[i]->cType == CONNECTION_SYSTRAY_ICON) {
				ScriptValue sv3;
				if (CreateListValue(sv, 4)) {
					SysTrayIconDevice *dev = (SysTrayIconDevice*) devices[i];

					CreateStringValue(sv3, "DrawIcon");
					sv.listVal->PushBack(sv3);

					CreateNullValue(sv3);
					sv.listVal->PushBack(sv3);

					dev->objectValue.AddRef();
					sv.listVal->PushBack(dev->objectValue);

					sv.listVal->PushBack(sv2);

					if (dev->obj) dev->obj->AddRef();
					RunFunction(dev->funcId, sv, CAN_DRAW, dev->obj);
					dev->UpdateImage();
				}
				else {
					sv2.Release();
				}
			}
			else {
				devices[i]->id->AddRef();
				CreateStringValue(sv, devices[i]->id);
				TriggerEvent(drawEvent, CAN_DRAW, &sv, &sv2);
				lastDraw = tick;
				redrawn = 1;
			}
			activeScreen = dummyScreen;
		}
	}
	if (needDummy) {
		// Prevents recursive calls.
		if (((Device*)dummyScreen)->needRedraw) {
			// Prevents recursive calls with initial check, just in case.
			dummyScreen = 0;
			ScriptValue sv;
			CreateNullValue(sv);
			TriggerEvent(drawEvent, CAN_DRAW, &sv);
			lastDraw = tick;
			dummyScreen = activeScreen;
			redrawn = 1;
		}
	}

	if (redrawn || (unsigned __int64)(tick - lastDraw) >= 5) {
		if (!FlushImage(1)) {
			lastDraw = tick - 10;
		}
		else
			lastDraw = tick;
	}
	/*
	screen.BeginDraw();
	screen.Clear();
	screen.EndDraw();
	Chip8 *chip8 = LoadChip8((unsigned char*)"chip8\\schip_roms\\JOUST23");
	//*/
	//while (1) {
		//needRedraw = 1;
		//if (res == -1) Sleep(50);
		//if (needRedraw && screen.BeginDraw()) {
			/*
			res = chip8->Tick();
			screen.Clear();
			for (int y=0; y<chip8->height; y++) {
				unsigned char *pos = chip8->screen+y*chip8->width;
				for (int x=0; x<chip8->width; x++) {
					for (int px = 0, mask = 0x01; mask; mask<<=1,px++) {
						if (pos[x] & mask) {
							screen.SetPixel(x*8+px, y);
						}
					}
				}
			}
			//*/
		//	needRedraw = 0;
		//	TriggerEvent(drawEvent, CAN_DRAW);
			/*activeApp->Draw();
			visibleToolbar->Draw();
			if (overlay) overlay->Draw();
			//*/
		/*	#ifdef _DEBUG
				//screen.DisplayTextRight(159, 32, "Debug");
			#endif
			screen.EndDraw();
			redrawn = 1;
		}
		//*/
	/*
		if (redrawn || (unsigned __int64)(tick - lastDraw) >= 5) {
			if (!FlushImage(1)) {
				lastDraw = tick - 10;
			}
			else
				lastDraw = tick;
		}//*/
	//}
}


const unsigned int WM_TASKBARCREATED = RegisterWindowMessageA("TaskbarCreated");

HWND lastForegroundWindow = 0;

int screenSaverOn = 0;
int monitorOff = 0;

static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	// If is faster than a case statement, and this is by far the most common message.
	if (uMsg == WM_TIMER || uMsg == WM_APP) {
		wchar_t test[100];
		wsprintf(test, L"%X\t%i\t%i\r\n", uMsg, wParam, lParam);
		//if (!quitting) {
			if (wParam == 0) {
				//*
				if (EventExists(foregroundWindowChangeEvent)) {
					HWND foregroundWindow = GetForegroundWindow();
					if (foregroundWindow != lastForegroundWindow) {
						HWND temp = lastForegroundWindow;
						lastForegroundWindow = foregroundWindow;
						TriggerEvent(foregroundWindowChangeEvent, (__int64)foregroundWindow, (__int64)temp);
					}
				}
				int running = 0;
				if (SystemParametersInfo(SPI_GETSCREENSAVERRUNNING, 0, &running, 0) && running != screenSaverOn) {
					screenSaverOn = running;
					TriggerEvent(screenSaverEvent, running);
				}
				int w = SystemParametersInfo(SPI_GETPOWEROFFACTIVE, 0, &running, 0);

				perfMon.Update();
				if (EventExists(counterUpdateEvent)) {
					TriggerEvent(counterUpdateEvent);
				}
				Timer::Update();
				// Makes sure we flush every 5 seconds or so, just in case.
				Draw();
			}
			else if (wParam == 1) {
				int needTimer = 0;
				for (int w=0; w<numDevices; w++) {
					int v = devices[w]->gKeyState;
					ScriptValue sv[2];
					CreateStringValue(sv[1], devices[w]->id);
					// Just in case device dies for some reason while triggering event.
					// Don't think it can happy, but just in case...
					sv[1].AddRef();
					int i = 1;
					while (v) {
						if (v&i) {
							v -= i;
							sv[1].AddRef();
							CreateIntValue(sv[0], i);
							// Doesn't currently matter.
							TriggerEvent(g15ButtonDownEvent, &sv[0], &sv[1]);
							needTimer = 1;
						}
						i <<= 1;
					}
					sv[1].Release();
				}
				if (needTimer) {
					SetTimer(hWnd, 1, 240, 0);
				}
				else {
					KillTimer(hWnd, 1);
				}
			}
			else if (wParam == 2) {
				Draw();
			}
			else if (wParam == 3) {
				FlushImage(0);
			}
		return 0;
	}
	lParam = lParam;
	switch (uMsg) {
		case WM_USER:
			{
				TriggerEvent(userEvent, wParam, lParam);
			}
			break;
		case WMU_EATEN_KEY:
			{
				ScriptValue modifier;
				ScriptValue vk;
				ScriptValue string;
				CreateIntValue(modifier, (wParam>>16)&15);
				CreateIntValue(vk, (short)wParam);
				if (!lParam) CreateNullValue(string);
				else {
					int len = 1 + ((lParam&0xFFFF0000)!=0);
					CreateStringValue(string, (wchar_t*)&lParam, len);
				}
				int id = keyUpEvent;
				if (wParam>>24) {
					id = keyDownEvent;
				}
				TriggerEvent(id, &modifier, &vk, &string);
			}
			return 0;
		case WMA_LCD_DEVICE_CHANGE:
			RefreshPluginDevices((int)wParam);
			return 0;
		case WMA_TRIGGER_EVEN_BY_NAME:
			{
				unsigned char *e = (unsigned char*) wParam;
				ScriptValue sv;
				CreateStringValue(sv, (StringValue*)lParam);
				int id = GetEventId(e, 1);
				free(e);
				if (id >= 0) {
					TriggerEvent(id, &sv);
				}
				else {
					sv.Release();
				}
			}
			return 0;
		case WMU_TRIGGER_EVENT:
			{
				// Used from different threads.  Can only add refs in main thread.
				EventParams *p = (EventParams *)lParam;
				p->args[0].AddRef();
				p->args[1].AddRef();
				TriggerEvent(p->id, &p->args[0], &p->args[1]);
				free(p);
			}
			return 0;
		case WMU_TRIGGER_EVENT_FULL:
			{
				FullEventParams *p = (FullEventParams *)lParam;
				TriggerEventFull(p->id, p->args);
				free(p);
			}
			return 0;
		case WM_COPYDATA:
			{
				int w = 8;
				COPYDATASTRUCT *cd = (COPYDATASTRUCT*) lParam;
				unsigned char* params;
				if (cd && (params = (unsigned char*)cd->lpData) && cd->cbData > 8 && (*(__int64*)params == EXEC_MESSAGE) && params[cd->cbData-1] == 0) {
					ScriptValue sv;
					if (CreateListValue(sv, 10)) {
						ScriptValue sv2;

						do {
							int start = w;
							while (params[w] && params[w]!='\n') {
								w++;
							}
							if (CreateStringValue(sv2, params+start, w-start))
								sv.listVal->PushBack(sv2);
						}
						while (params[w++]);
						TriggerEvent(execEvent, &sv);
					}
				}
			}
		//case WMU_EXEC:
			/*
			if (wParam == 0x97345234) {
				ATOM a = (ATOM) lParam;
				wchar_t temp[260];
				int t = GlobalGetAtomNameW(a, temp, 260);
				if (0 < t && t < 260) {
					unsigned char *s = UTF16toUTF8Alloc(temp);
					if (s) {
						mediaPlayer.PlayFile(s, 0);
						return 0x97345234;
					}
				}
			}//*/
			return 0;
		/*case WMU_EXEC:
			{
				return WM_DDE_ACK;
				wParam = wParam;
			}//*/
		case WM_DEVICECHANGE:
			if (wParam == 0x0007) {
				UpdateHID();
				UpdateSDKDevices();
			}
			//wParam = wParam;
			break;
		case WMU_BUTTONS_HID:
			{
				int i;
				for (i = 0; i<numDevices; i++) {
					if (devices[i]->cType == CONNECTION_DIRECT && devices[i]->g15 == (G15*)lParam) {
						lParam = i;
						break;
					}
				}
				if (i == numDevices) return 0;
			}
		case WMU_BUTTONS:
			if (lParam < numDevices) {
				int down = 0;
				if (devices[lParam]->cType == CONNECTION_DIRECT && devices[0]->cType != CONNECTION_DIRECT) {
					wParam &= ~0x3F;
				}
				int v = devices[lParam]->gKeyState ^ (int)wParam;
				int i = 1;
				devices[lParam]->gKeyState = (unsigned int)wParam;
				ScriptValue sv[2];
				CreateStringValue(sv[1], devices[lParam]->id);
				// Just in case device dies for some reason while triggering event.
				// Don't think it can happy, but just in case...
				sv[1].AddRef();
				while (v) {
					if (v&i) {
						v -= i;
						CreateIntValue(sv[0], i);
						sv[1].AddRef();
						if (wParam & i) {
							down = 1;
							TriggerEvent(g15ButtonDownEvent, &sv[0], &sv[1]);
						}
						else
							TriggerEvent(g15ButtonUpEvent, &sv[0], &sv[1]);
					}
					i <<= 1;
				}
				sv[1].Release();
				if (down)
					SetTimer(hWnd, 1, 240, 0);
			}
				/*
			if (!numG15s || !g15enabledCount) {
				int v = gButtons & ~(int)wParam;
				int i = 1;
				while (v) {
					if (v&i) {
						v -= i;
						ScriptValue sv[2];
						CreateIntValue(sv[0], i);
						CreateNullValue(sv[1]);
						TriggerEvent(g15ButtonUpEvent, &sv[0], &sv[1]);
					}
					i <<= 1;
				}
			}
			gButtons = (int) wParam;
			//*/
			return 0;//WndProc(hWnd, WM_TIMER, 1, 0);

		case WMU_VISTA_AUDIO_DEV_CHANGE:
			SetupVistaVolume();
		case WMU_VISTA_VOLUME_CHANGE:
			TriggerEvent(audioChangeEvent);
			return 0;

		case MM_MIXM_LINE_CHANGE:
			InitAudio();
		case MM_MIXM_CONTROL_CHANGE:
			TriggerEvent(audioChangeEvent);
			break;

		case WM_POWERBROADCAST:
			if (wParam == PBT_POWERSETTINGCHANGE) {
				POWERBROADCAST_SETTING *s = (POWERBROADCAST_SETTING*)lParam;
				if (!memcmp(&s->PowerSetting, &GUID_MONITOR_POWER_ON, sizeof(GUID_MONITOR_POWER_ON)) && s->DataLength > 0) {
					int newValue = (0 == s->Data[0]);
					if (monitorOff != newValue) {
						monitorOff = newValue;
						TriggerEvent(monitorPowerEvent, newValue);
					}
				}
			}
			break;
		case WM_HOTKEY:
			shortcutManager.HotkeyPressed(wParam);
			return 0;
		case WMU_DNS_DONE:
			{
				DNSData* data = (DNSData*) wParam;
				data->netProc(data->firstAddr, data->id);
				data->firstAddr = 0;
				CleanupDNS(data);
			}
			return 0;
/*		case WMU_PING_SOCKET:
			HandlePingSocketMessage(wParam, lParam);
			return 0;
			//*/
		case WMU_SCRIPTED_SOCKET:
			HandleScriptedSocketMessage(wParam, lParam);
			return 0;
		case WMU_HTTP_SOCKET:
			HandleHttpSocketMessage(wParam, lParam);
			return 0;
		/*case WMU_START_CONNECTING:
			StartHttpConnection((int)wParam);
			return 0;
			//*/
		case WM_CLOSE:
			DestroyWindow(hWnd);
			break;
		//case WMU_FOUND_PAGE:
		//	systemStats.FindIP(GetBufferedHttpSocket((int)wParam));
		//	return 0;
		case WMU_CALL_PROC:
			// Used to execute code in main thread.  Primarily used
			// for other mini-threads that need to run a script, or continue running
			// a script, after they've done their job.
			((MiscProc*) lParam) ((void*)wParam);
			return 0;
		//case WMU_LOCAL_NAMES:
		//	systemStats.SendNetworkNames((NetworkNames **)wParam);
		//	return 0;
		case WMU_DSHOW_EVENT:
			HandleDshowEvent(lParam, wParam);
			//mediaPlayer.Event();
			return 0;
		case WMU_TIMER_PROC_DONE:
			((Timer*) wParam)->SaveTime();
			// Keep thread handles around as simple way to check if threads are running.
			// Have to clean them up properly, however.
			if (lParam) {
				HANDLE *thread = (HANDLE*) lParam;
				CloseHandle(*thread);
				*thread = 0;
			}
			return 0;
		case WMU_KEYBOARD_MESSAGE:
			/*if (activeApp) {
				activeApp->HandleKBMessageSync((int)wParam, (int)lParam);
				if (needRedraw) Draw();
			}//*/
			return 0;
		case WMA_ICON_MESSAGE:
			if (wParam == IDI_FROG) {
				POINT mouse;
				HMENU hMenu, hMenu2;
				switch(lParam) {
				case WM_RBUTTONUP:
					if (GetCursorPos(&mouse) == 0)
						break;
					if (hMenu = LoadMenu(ghInst, (LPCTSTR) IDR_TRAY_MENU)) {
						if (hMenu2 = GetSubMenu(hMenu, 0)) {
							SetForegroundWindow(hWnd);
							TrackPopupMenu(hMenu2, 0, mouse.x, mouse.y, 0, hWnd, 0);
							PostMessage(hWnd, WM_NULL, 0, 0);
						}

						DestroyMenu(hMenu);
					}
					return 0;
				//case WM_LBUTTONDBLCLK:
				default:
					break;
				}
			}
			else {
				for (int i=0; i<numDevices; i++) {
					if (devices[i]->cType == CONNECTION_SYSTRAY_ICON) {
						SysTrayIconDevice *dev = (SysTrayIconDevice *)devices[i];
						if (dev->iid == wParam) {
							HandleIconMessage(dev, lParam);
							break;
						}
					}
				}
			}
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case ID_TRAY_EXIT:
					DestroyWindow(hWnd);
					return 0;
				default:
					break;
			}
			break;
		case WM_CREATE:
			ghWnd = hWnd;
 			if (config.GetConfigInt((unsigned char*) "General", (unsigned char*) "Tray Icon", 0) == 1)
				LoadTrayIcon(hWnd);
			// Initializes notifications and GUID.
			break;
		case WM_DESTROY:
			if (nextClipboardListener) {
				ChangeClipboardChain(hWnd, nextClipboardListener);
				nextClipboardListener  = 0;
			}
			TriggerEvent(quitEvent, 0);
			ChangeClipboardChain(hWnd, nextClipboardHwnd);
			nextClipboardHwnd = 0;
			quitting = 1;
			if (devices && devices[0]->cType == CONNECTION_DIRECT) {
				for (int i=0; i<numDevices; i++) {
					if (devices[i]->cType == CONNECTION_DIRECT) {
						devices[i]->Clear();
						devices[i]->sendImage = 1;
					}
					else devices[i]->sendImage = 0;
				}
				Sleep(100);
				FlushImage(1);
			}
			// Inform other threads/whatever we're quitting, and set hWnd to null
			// to prevent sending messages anywhere.
			RemoveTrayIcon();
			PostQuitMessage(0);
			break;
		case WM_CHANGECBCHAIN:
			if ((HWND)wParam == nextClipboardListener) {
				nextClipboardListener = (HWND)lParam;
			}
			else {
				SendMessage(nextClipboardListener, WM_CHANGECBCHAIN, wParam, lParam);
			}
			return 0;
		case WM_DRAWCLIPBOARD:
			if (nextClipboardListener) SendMessage(nextClipboardListener, WM_DRAWCLIPBOARD, wParam, lParam);
			ReplyMessage(0);
			TriggerEvent(clipboardEvent);
			break;
		default:
			if (uMsg == WM_TASKBARCREATED) {
				if (gOnTray) {
					RemoveTrayIcon();
					LoadTrayIcon(hWnd);
				}
				UpdateIcons();
			}
			break;
	}
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}

#include "Script.h"

typedef HANDLE HPOWERNOTIFY;
typedef HPOWERNOTIFY WINAPI RegisterPowerSettingNotificationType (
  __in  HANDLE hRecipient,
  __in  LPCGUID PowerSettingGuid,
  __in  DWORD Flags
);

int errors = 0;
int warnings = 0;

HINSTANCE ghInst;
#include <crtdbg.h>
int WINAPI WinMain(	HINSTANCE hInst,
					HINSTANCE hPrevInstance,
					LPSTR lpCmdLine,
					int nCmdShow) {
#ifdef _DEBUG
	//int tmp = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	//_CrtSetDbgFlag (tmp | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_CRT_DF | _CRTDBG_DELAY_FREE_MEM_DF);
	_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG|_CRTDBG_ALLOC_MEM_DF|_CRTDBG_CHECK_ALWAYS_DF|_CRTDBG_CHECK_CRT_DF|_CRTDBG_DELAY_FREE_MEM_DF);
#endif

	UtilInit();

	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
	{
		wchar_t temp[MAX_PATH*2+20];
		if (GetConfFile(temp, L".log")) {
			DeleteFileW(temp);
		}
	}

	srand(time_t(0));
	noLCD = (0!=config.GetConfigInt((unsigned char*)"General", (unsigned char*) "NoLCD", 0));

	HANDLE hMutex = CreateMutexW(0, 0, L"LCD Misc Start Mutex");
	if (!hMutex) return 0;
	DWORD r = WaitForSingleObject(hMutex, 0);
	unsigned char *args = 0;
	int argsLen = 0;
	{
		wchar_t *command = GetCommandLineW();
		if (command && (args = (unsigned char*) malloc(sizeof(unsigned char) * (8+1+wcslen(command))))) {
			*(__int64*)args = EXEC_MESSAGE;
			UTF16toUTF8(args+8, command);
			unsigned char *cmd = args+8;
			int i = 0, j = 0, inquotes = 0;
			int backslash = 0;
			while (cmd[i]) {
				if (cmd[i] == '"') {
					j -= backslash/2;
					if (!(backslash&1)) {
						inquotes = !inquotes;
					}
					else {
						cmd[j++] = '"';
					}
					i++;
					backslash = 0;
					continue;
				}
				else if (cmd[i] == '\\') {
					backslash++;
					cmd[j++] = cmd[i++];
					continue;
				}
				else if ((cmd[i] == ' ' || cmd[i] == '\t') && !inquotes) {
					if (!j) {
						i++;
						continue;
					}
					if (!inquotes) {
						i++;
						if (cmd[j-1] != '\n') {
							cmd[j++] = '\n';
						}
						continue;
					}
				}
				cmd[j++] = cmd[i++];
				backslash = 0;
			}
			while (j && cmd[j-1] == '\n') j--;
			cmd[j] = 0;
			unsigned char *t = strchr(cmd, '\n');
			if (!t) {
				free(args);
				args = 0;
			}
			else {
				strcpy(cmd, t+1);
				argsLen = 9 + strlen(cmd);
				srealloc(args, sizeof(unsigned char)*argsLen);
			}
		}
	}
	if (r != WAIT_OBJECT_0) {
		CloseHandle(hMutex);
		return 0;
	}

	{
		int t = 0;
		while (1) {
			HWND hWndOld = FindWindowW(L"LCD Miscellany",  L"Text no one should see.");
			if (!hWndOld) break;
			if (args) {
				COPYDATASTRUCT cd;
				cd.dwData = 0;
				cd.cbData = argsLen;
				cd.lpData = args;
				SendMessage(hWndOld, WM_COPYDATA, 0, (LPARAM)&cd);
				free(args);
				exit(0);
			}
			else {
				t++;
				if (t >= 32) {
					exit(0);
				}
				else if (t >= 16) {
					DWORD test;
					if (GetWindowThreadProcessId(hWndOld, &test)) {
						KillProcess(test);
					}
				}
				else {
					// No clue why this combination is needed, but it is
					// to avoid showing the window and make sure quit is called.
					PostMessage(hWndOld, WM_CLOSE, 0,0);
					DestroyWindow(hWndOld);
					Sleep(250);
				}
			}
		}
	}

	{
		const WNDCLASSW wc = {
			0,
			(WNDPROC) WndProc,
			0,
			0,
			hInst,
			LoadIcon(hInst, MAKEINTRESOURCE(IDI_FROG)),
			0,
			0,
			0,
			L"LCD Miscellany"
		};

		ghInst = hInst;
		if (!RegisterClassW(&wc)) {
		    //CoUninitialize();
			CloseHandle(hMutex);
			return 1;
		}
		// gHwnd initialized in WM_CREATE:
		HWND hWnd = CreateWindow(L"LCD Miscellany", L"Text no one should see.",
								  0, //WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
								  0,0,//CW_USEDEFAULT, CW_USEDEFAULT,
								  0,0,//CW_USEDEFAULT, CW_USEDEFAULT,
								  0,0, hInst, 0);
		HMODULE user32 = GetModuleHandleA("User32.dll");
		if (user32) {
			RegisterPowerSettingNotificationType* RegisterPowerSettingNotification =
				(RegisterPowerSettingNotificationType*) GetProcAddress(user32, "RegisterPowerSettingNotification");
			if (RegisterPowerSettingNotification) {
				RegisterPowerSettingNotification(hWnd, &GUID_MONITOR_POWER_ON, DEVICE_NOTIFY_WINDOW_HANDLE);
			}
		}
	}
	if (!InitSockets()) {
		errorPrintf(2, "Sockets Error: Couldn't initialize sockets");
		CloseHandle(hMutex);
		return 0;
	}

	if (!ghWnd) {
		CloseHandle(hMutex);
		WSACleanup();
		return 1;
	}
	SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);

	if (!RegisterProcs() || !RegisterObjectTypes()) {
		errorPrintf(2, "Error: Couldn't initialize functions");
		CloseHandle(hMutex);
		WSACleanup();
		return 0;
	}
	activeScreen = dummyScreen = new Device(160, 43, 1, CONNECTION_NULL, SDK_160_43_1, "");
	UpdateSDKDevices();
	InitHID();
	InitPlugins();
	{
		int test = config.GetConfigInt((unsigned char*)"Script", (unsigned char*) "ScriptDebug", 0);
		if (test & 1) {
			wchar_t temp[2*MAX_PATH+20];
			if (GetConfFile(temp, L" ASM.log")) {
				FILE *out = _wfopen(temp, L"wb");
				if (out) fclose(out);
			}
		}
		int i=0;
		while (1) {
			unsigned char *temp = config.GetConfigString((unsigned char*)"Script", (unsigned char*)"LoadFile", (unsigned char*)0, 1, i);
			if (!temp) break;
			LoadScript(temp);
			free(temp);
			i++;
		}
		CleanupCompiler();
		CheckFxns();

		if (!InitEvents() ||
			!(VM.globals = (ScriptValue*)malloc(sizeof(ScriptValue) * VM.Globals.numValues))) {
			errorPrintf(2, "Memory initialization error");
			CloseHandle(hMutex);
			WSACleanup();
			delete [] activeScreen;
			UninitSDK();
			UninitHID();
			return 0;
		}
		for (i=0; i<VM.Globals.numValues; i++) {
			CreateNullValue(VM.globals[i]);
		}
	}
	CloseHandle(hMutex);
	errorPrintf(0, "\n");



	if (SetTimer(ghWnd, 0, 1000, 0)) {
		SetupVistaVolume();
		InitAudio();
		shortcutManager.Load();
		MSG msg;
		perfMon.Update();
		{
			int i = 0;
			while (1) {
				unsigned char *temp = config.GetConfigString((unsigned char*)"Script", (unsigned char*)"InitProc", (unsigned char*)0, 1, i);
				if (!temp) break;
				int w = FindFunction(temp);
				if (w < 0 || !RunFunction(w, CAN_WAIT)) {
					errorPrintf(2, "Initialization Warning: Failed to run function: %s.\r\n", temp);
					warnings++;
				}
				free(temp);
				i++;
			}
		}


		if (args) {
			COPYDATASTRUCT cd;
			cd.dwData = 0;
			cd.cbData = argsLen;
			cd.lpData = args;
			SendMessage(ghWnd, WM_COPYDATA, (WPARAM)ghWnd, (LPARAM)&cd);
			free(args);
		}
		PostMessage(ghWnd, WM_TIMER, 0, 0);
		nextClipboardHwnd = SetClipboardViewer(ghWnd);

		if (errors || warnings) {
			char errorString[1000] = "";
			char *title = "LCD Miscellany Warning";
			int flag = MB_ICONWARNING;
			if (errors) {
				title = "LCD Miscellany Error";
				if (errors > 1) sprintf(errorString, "%i errors\n", errors);
				else strcpy(errorString, "1 error\n");
				flag = MB_ICONEXCLAMATION;
			}
			if (warnings) {
				if (warnings > 1) sprintf(strchr(errorString, '\0'), "%i warnings\n", warnings);
				else strcpy(strchr(errorString, '\0'), "1 warning\n");
			}
			sprintf(strchr(errorString, '\0'), "\nOpen log file?");
			wchar_t file[MAX_PATH*2];
			if (GetConfFile(file, L".log")) {
				if (IDYES == MessageBoxA(0, errorString, title, MB_YESNO | flag)) {
					ShellExecuteW(0, 0, file, 0, 0, SW_SHOW);
				}
			}
		}

		//CreateSysTrayIconDevice("Test");
		while (GetMessage(&msg, 0, 0, 0) == TRUE) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	delete dummyScreen;
	UninitPlugins();
	UninitHID();
	UninitSDK();
	UninitIcons();
	CleanupDdraw();
	config.Save();

	CleanupPings();
	CleanupSockets();

	// need to avoid uninitializing com before I release com objects.  Might move more cleanup here later.
	CleanupAllDNS();
	CleanupHttpSockets();
	CleanKeyEvents();

	CleanupScripting();
	UninitAudio();
	CleanupVistaVolume();
	KillCom();
	return 0;
}

