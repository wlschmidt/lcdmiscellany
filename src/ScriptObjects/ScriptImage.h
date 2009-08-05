#ifndef SCRIPT_IMAGE_H
#define SCRIPT_IMAGE_H

#include "../ScriptValue.h"

void LoadImage32(ScriptValue &s, ScriptValue *args);
void LoadMemoryImage32(ScriptValue &s, ScriptValue* args);
void Image32Size(ScriptValue &s, ScriptValue *args);
void Image32Zoom(ScriptValue &s, ScriptValue *args);
void Image32ToBitImage(ScriptValue &s, ScriptValue *args);
void Image32SaveBMP(ScriptValue &s, ScriptValue *args);

void ColorImageSize(ScriptValue &s, ScriptValue *args);

void MyLoadImage(ScriptValue &s, ScriptValue *args);
void LoadMemoryImage(ScriptValue &s, ScriptValue* args);
void DrawImage(ScriptValue &s, ScriptValue* args);
void ClearImage(ScriptValue &s, ScriptValue* args);
void InvertImage(ScriptValue &s, ScriptValue* args);
void ImageSize(ScriptValue &s, ScriptValue *args);
void IntersectImage(ScriptValue &s, ScriptValue* args);
void DrawTransformedImage(ScriptValue &s, ScriptValue* args);
void DrawRotatedScaledImage(ScriptValue &s, ScriptValue* args);

void ScriptGetClipboardData(ScriptValue &s, ScriptValue *args);
void ScriptSetClipboardData(ScriptValue &s, ScriptValue *args);

extern unsigned int ImageType;
extern unsigned int Image32Type;

#endif
