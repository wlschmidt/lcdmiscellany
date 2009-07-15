#include "CommandPromptProcess.h"
#include "../global.h"
#include <shellapi.h>
#include "../util.h"
#include "../stringUtil.h"
#include "../unicode.h"

unsigned int CommandPromptProcessType;

struct ConnectedProcess {
	union {
		struct {
			HANDLE error[2], in[2], out[2];
		};
		HANDLE handles[6];
	};
};

void CoommandPromptProcess(ScriptValue &s, ScriptValue* args) {
if (0)
{
  char buf[1024];           //i/o buffer

  STARTUPINFOA si;
  SECURITY_ATTRIBUTES sa;
  PROCESS_INFORMATION pi;
  HANDLE newstdin,newstdout,read_stdout,write_stdin;  //pipe handles

  sa.lpSecurityDescriptor = NULL;
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.bInheritHandle = true;         //allow inheritable handles

  if (!CreatePipe(&newstdin,&write_stdin,&sa,0))   //create stdin pipe
  {
    return;
  }
  if (!CreatePipe(&read_stdout,&newstdout,&sa,0))  //create stdout pipe
  {
    CloseHandle(newstdin);
    CloseHandle(write_stdin);
    return;
  }

  GetStartupInfoA(&si);      //set startupinfo for the spawned process
  /*
  The dwFlags member tells CreateProcess how to make the process.
  STARTF_USESTDHANDLES validates the hStd* members. STARTF_USESHOWWINDOW
  validates the wShowWindow member.
  */
  si.dwFlags = STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
  si.wShowWindow = SW_HIDE;
  si.hStdOutput = newstdout;
  si.hStdError = newstdout;     //set the new handles for the child process
  si.hStdInput = newstdin;
  char app_spawn[] = "C:\\windows\\system32\\ping.exe"; //sample, modify for your
                                                     //system

  //spawn the child process
  if (!CreateProcessA(app_spawn,NULL,NULL,NULL,TRUE,CREATE_NEW_CONSOLE,
                     NULL,NULL,&si,&pi))
  {
    CloseHandle(newstdin);
    CloseHandle(newstdout);
    CloseHandle(read_stdout);
    CloseHandle(write_stdin);
    return;
  }

  unsigned long exit=0;  //process exit code
  unsigned long bread;   //bytes read
  unsigned long avail;   //bytes available

  for(;;)      //main program loop
  {
    PeekNamedPipe(read_stdout,buf,1023,&bread,&avail,NULL);
    //check to see if there is any data to read from stdout
    if (bread != 0)
    {
      if (avail > 1023)
      {
        while (bread >= 1023)
        {
          ReadFile(read_stdout,buf,1023,&bread,NULL);  //read the stdout pipe
          printf("%s",buf);
        }
      }
      else {
        ReadFile(read_stdout,buf,1023,&bread,NULL);
        printf("%s",buf);
      }
    }
    GetExitCodeProcess(pi.hProcess,&exit);      //while the process is running
    if (exit != STILL_ACTIVE)
      break;
	/*
    if (kbhit())      //check for user input.
    {
      bzero(buf);
      *buf = (char)getche();
      //printf("%c",*buf);
      WriteFile(write_stdin,buf,1,&bread,NULL); //send it to stdin
      if (*buf == '\r') {
        *buf = '\n';
        printf("%c",*buf);
        WriteFile(write_stdin,buf,1,&bread,NULL); //send an extra newline char,
                                                  //if necessary
      }
    }
	//*/
  }
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  CloseHandle(newstdin);            //clean stuff up
  CloseHandle(newstdout);
  CloseHandle(read_stdout);
  CloseHandle(write_stdin);
}

	wchar_t *path = ResolvePath(args[0].stringVal->value);
	wchar_t *dir = 0;
	if (path && (!args[2].stringVal->len || (dir = ResolvePath(args[2].stringVal->value)))) {
		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		memset(&si, 0, sizeof(si));
		si.cb = sizeof(si);
		wchar_t *params = UTF8toUTF16Alloc(args[1].stringVal->value);
		ConnectedProcess *cp = 0;
		cp = (ConnectedProcess*)calloc(1,sizeof(ConnectedProcess));
		if (cp) {
			SECURITY_ATTRIBUTES saAttr; 
			saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
			saAttr.bInheritHandle = TRUE;
			saAttr.lpSecurityDescriptor = NULL; 
			static __int64 namedPipes = 0;
			char pipeNames[3][100];
			sprintf(pipeNames[0], "\\\\.\\pipe\\LCD Miscellany error pipe %I64i", namedPipes);
			sprintf(pipeNames[1], "\\\\.\\pipe\\LCD Miscellany out pipe %I64i", namedPipes);
			sprintf(pipeNames[2], "\\\\.\\pipe\\LCD Miscellany in pipe %I64i", namedPipes);
			namedPipes++;
			//*
			cp->error[0] = CreateNamedPipeA(pipeNames[0], PIPE_ACCESS_INBOUND, PIPE_TYPE_BYTE | PIPE_WAIT, 1, 1024, 1024, 5000, 0);
			cp->error[1] = CreateFileA(pipeNames[0], GENERIC_WRITE, 0, &saAttr, OPEN_EXISTING, 0, 0);
			cp->out[0] = CreateNamedPipeA(pipeNames[1], PIPE_ACCESS_INBOUND, PIPE_TYPE_BYTE | PIPE_WAIT, 1, 1024, 1024, 5000, 0);
			cp->out[1] = CreateFileA(pipeNames[1], GENERIC_WRITE, 0, &saAttr, OPEN_EXISTING, 0, 0);
			cp->in[0] = CreateNamedPipeA(pipeNames[2], PIPE_ACCESS_OUTBOUND, PIPE_TYPE_BYTE | PIPE_WAIT, 1, 1024, 1024, 5000, 0);
			cp->in[1] = CreateFileA(pipeNames[2], GENERIC_READ, 0, &saAttr, OPEN_EXISTING, 0, 0);
			//*/
			//CreatePipe(&cp->error[0], &cp->error[1], &saAttr, 0);
			//CreatePipe(&cp->out[0], &cp->out[1], &saAttr, 0);
			//CreatePipe(&cp->in[1], &cp->in[0], &saAttr, 0);
			int happy = 1;
			for (int i=0; i<3; i++) {
				if (cp->handles[2*i] == INVALID_HANDLE_VALUE ||
					cp->handles[2*i+1] == INVALID_HANDLE_VALUE) {
					happy = 0;
				}
			}
			if (happy) {
				si.hStdError = cp->out[1];
				si.hStdInput = cp->in[1];
				si.hStdOutput = cp->out[1];
				si.dwFlags = STARTF_USESTDHANDLES;
				if (!CreateProcessW(L"c:\\Windows\\System32\\PING.EXE", 0, 0, 0, 1, CREATE_NEW_CONSOLE, 0, dir, &si, &pi)/* && CreateListValue(s, 4)//*/) {
					happy = 0;
				}
				else {
					CreateIntValue(s, pi.dwProcessId);
					if (si.dwFlags) {
						char temp[1000];
						{
							DWORD bread, avail;
							//PeekNamedPipe(read_stdout,temp,1000,&bread,&avail,NULL);
							PeekNamedPipe(cp->out[0],temp,1000,&bread,&avail,NULL);
							PeekNamedPipe(cp->in[0],temp,1000,&bread,&avail,NULL);
							bread = bread;
						}
						DWORD read = 0;
						ReadFile(cp->out[0], temp, 1, &read, 0);
						ReadFile(cp->error[0], temp, 1, &read, 0);
						ReadFile(cp->in[0], temp, 1000, &read, 0);
						read = read;
					}
				}
			}
			if (!happy) {
				if (cp) {
					for (int i=0; i<6; i++) {
						if (cp->handles[i] != INVALID_HANDLE_VALUE) {
							CloseHandle(cp->handles[i]);
						}
					}
					free(cp);
				}
			}
		}
		free(params);
	}
	free(path);
	free(dir);
}
