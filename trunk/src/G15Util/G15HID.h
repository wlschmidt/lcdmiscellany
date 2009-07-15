#ifndef G15HID_H
#define G15HID_H

#include "../sdk/v3.01/lglcd.h"
#include "../ScriptValue.h"
#include "../Device.h"

int G15sSetFeature(unsigned char* bytes, int size, ScriptValue &kb);

void InitHID();
void UninitHID();
void UpdateHID();

int G15EnableHID();
int G15DisableHID();

inline int G15sSetLight(int value, ScriptValue &kb) {
	if (value < 0) value = 0;
	static unsigned char bytes[4] = {0x02, 0x01, 0x00, 0x00};
	if (value >= 2) bytes[2] = 2;
	else bytes[2] = (unsigned char) value;
	return G15sSetFeature(bytes, 4, kb);
}

inline int G15sSetLCDLight(int value, ScriptValue &kb) {
	if (value < 0) value = 0;
	static unsigned char bytes[4] = {0x02, 0x02, 0x00, 0x00};
	if (value >= 2) bytes[2] = 32;
	else bytes[2] = (unsigned char) (16*value);
	return G15sSetFeature(bytes, 4, kb);
}

inline int G15sSetMLights(int value, ScriptValue &kb) {
	if (value < 0) value = 0;
	static unsigned char bytes[4] = {0x02, 0x04, 0x00, 0x00};
	bytes[2] = (0x0F & (unsigned char) value)^0x0F;
	unsigned char bytes2[258] = {5,(unsigned char)(((value&1)<<7) + ((value&2)<<5) + ((value&4)<<3) + ((value&8)<<1)),0};
	return G15sSetFeature(bytes, 4, kb) + G15sSetFeature(bytes2, 258, kb);
}

inline int G15sSetBacklightColor(unsigned int value, ScriptValue &kb) {
	unsigned char bytes2[258] = {7,value>>16, value>>8, value,0};
	return G15sSetFeature(bytes2, 258, kb);
}

inline int G15sSetContrast(int value, ScriptValue &kb) {
	if (value < 0) value = 0;
	static unsigned char bytes[4] = {0x02, 0x20, 0x81, 0x00};
	if (value > 26) value = 26;
	bytes[3] = (unsigned char) value;
	return G15sSetFeature(bytes, 4, kb);
}


struct G15State {
	unsigned char junk;
	unsigned char light;
	unsigned char LCDLight;
	unsigned char MLights;
};

// returns static memory, so need to copy if keep around.
G15State *GetG15State(ScriptValue &kb, ScriptValue &sv);
// returns static memory, so need to copy if keep around.
G15State *GetG15State(Device *dev);

struct G19State {
	unsigned char MLights;
	Color4 color;
};
// returns static memory, so need to copy if keep around.
G19State *GetG19State(Device *dev);

void GetG15s(ScriptValue &s, ScriptValue &junk);
void GetG15ButtonState(ScriptValue &out, ScriptValue &arg);
extern int g15enabledCount;

void G15SendImage();

#endif
