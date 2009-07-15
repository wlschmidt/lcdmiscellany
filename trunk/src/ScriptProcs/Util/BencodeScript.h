#ifndef BENCODESCRIPT_H
#define BENCODESCRIPT_H
#include "../../ScriptValue.h"

void Bencode(ScriptValue &s, ScriptValue *args);
void BencodeExact(ScriptValue &s, ScriptValue *args);
void Bedecode(ScriptValue &s, ScriptValue *args);

void JSONdecode(ScriptValue &s, ScriptValue *args);

#endif
