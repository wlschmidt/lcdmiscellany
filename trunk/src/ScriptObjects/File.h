#ifndef DIRECTORY_H
#define DIRECTORY_H
#include "../ScriptValue.h"

//void ResolvePath(ScriptValue &s, ScriptValue *args);
/*void ListFiles(ScriptValue &s, ScriptValue *args);
void FileSize(ScriptValue &s, ScriptValue *args);
void FileName(ScriptValue &s, ScriptValue *args);
void FilePath(ScriptValue &s, ScriptValue *args);
void FileIsDirectory(ScriptValue &s, ScriptValue *args);
//void FileFullPath(ScriptValue &s, ScriptValue *args);
//*/
extern int FileReaderType;
extern int FileWriterType;

void OpenFileReader(ScriptValue &s, ScriptValue *args);
void FileReaderRead(ScriptValue &s, ScriptValue *args);
void CleanupFileReader(ScriptValue &s, ScriptValue *args);

void OpenFileWriter(ScriptValue &s, ScriptValue *args);
void FileWriterWrite(ScriptValue &s, ScriptValue *args);
void CleanupFileWriter(ScriptValue &s, ScriptValue *args);


#endif
