#ifndef SCRIPTDRAWING_H
#define SCRIPTDRAWING_H
#include "..\\ScriptValue.h"
void DisplayTextRight(ScriptValue &s, ScriptValue *args);
void DisplayTextCentered(ScriptValue &s, ScriptValue *args);
void DrawClippedText(ScriptValue &s, ScriptValue *args) ;

void ClearScreen(ScriptValue &s, ScriptValue *args);

void ColorRect(ScriptValue &s, ScriptValue *args);

void ClearRect(ScriptValue &s, ScriptValue *args);
void InvertRect(ScriptValue &s, ScriptValue *args);
void DrawRect(ScriptValue &s, ScriptValue *args);
void XorRect(ScriptValue &s, ScriptValue *args);

void DrawPixel(ScriptValue &s, ScriptValue *args);
void InvertPixel(ScriptValue &s, ScriptValue *args);
void ClearPixel(ScriptValue &s, ScriptValue *args);

void ColorLine(ScriptValue &s, ScriptValue *args);
void DrawLine(ScriptValue &s, ScriptValue *args);
void InvertLine(ScriptValue &s, ScriptValue *args);
void ClearLine(ScriptValue &s, ScriptValue *args);

void DisplayText(ScriptValue &s, ScriptValue *args);
void NeedRedraw(ScriptValue &s, ScriptValue *args);

void TextSize(ScriptValue &s, ScriptValue *args);
void FormatText(ScriptValue &s, ScriptValue *args);

void SetBgColor(ScriptValue &s, ScriptValue *args);
void SetDrawColor(ScriptValue &s, ScriptValue *args);
void MyRGB(ScriptValue &s, ScriptValue *args);
void MyRGBA(ScriptValue &s, ScriptValue *args);

void GetHDC(ScriptValue &s, ScriptValue *args);
#endif
