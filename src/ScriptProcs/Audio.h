#include "../ScriptValue.h"

void InitAudio();
void UninitAudio();
void GetNumMixers(ScriptValue &s, ScriptValue *args);
void GetAudioValue(ScriptValue &s, ScriptValue *args);
void SetAudioValue(ScriptValue &s, ScriptValue *args);

void GetAudioType(ScriptValue &s, ScriptValue *args);
void GetAudioString(ScriptValue &s, ScriptValue *args);

void GetVistaMasterVolume(ScriptValue &s, ScriptValue *args);
void SetupVistaVolume();
void CleanupVistaVolume();

