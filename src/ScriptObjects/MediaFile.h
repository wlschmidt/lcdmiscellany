#ifndef MEDIAFILE_H
#define MEDIAFILE_H
#include "..\\ScriptValue.h"

void MediaFile(ScriptValue &s, ScriptValue *args);
void MediaFilePlay(ScriptValue &s, ScriptValue *args);
void MediaFilePause(ScriptValue &s, ScriptValue *args);
void MediaFileStop(ScriptValue &s, ScriptValue *args);
void MediaFileSet(ScriptValue &s, ScriptValue *args);
void DestroyMediaFile(ScriptValue &s, ScriptValue *args);

void MediaSetVolume(ScriptValue &s, ScriptValue *args);

void MediaFileAuthor(ScriptValue &s, ScriptValue *args);
void MediaFileTitle(ScriptValue &s, ScriptValue *args);
void MediaFileDescription(ScriptValue &s, ScriptValue *args);
void MediaFileCopyright(ScriptValue &s, ScriptValue *args);

void MediaFileSetVolume(ScriptValue &s, ScriptValue *args);
void MediaFileSetBalance(ScriptValue &s, ScriptValue *args);

void MediaFileSetPosition(ScriptValue &s, ScriptValue *args);
void MediaFileGetPosition(ScriptValue &s, ScriptValue *args);
void MediaFileGetDuration(ScriptValue &s, ScriptValue *args);

void MediaFilePause(ScriptValue &s, ScriptValue *args);
void MediaFileGetState(ScriptValue &s, ScriptValue *args);

void HandleDshowEvent(size_t wParam, size_t lParam);

void MediaFileGetFFT(ScriptValue &s, ScriptValue *args);

extern unsigned int MediaFileType;

#endif
