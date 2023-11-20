#define _CRT_SECURE_NO_WARNINGS
#define PIPE_MAX_BUF 12400
#include <stdio.h>
#include <Windows.h>
#include "debugger.h"
#include "dlog.h"
#include <tchar.h>

int VERBOSE = 0;
int WARN = 0;

typedef const char* LABEL;
/*
windbg command or message
*/


LABEL BREAK_SYMBOL = "Breakpoint";
LABEL BREAK_EXCEPTION_SYMBOL = "Break instruction exception";
LABEL TERMINATE_SYMBOL = "ntdll!NtTerminateProcess";
LABEL MSG_BP_INIT = "bp";
LABEL CMD_BP_CREATE = "bp KERNELBASE!CreateFileW";
LABEL CMD_BP_CLOSE = "bp KERNELBASE!CloseHandle";
LABEL CMD_CONTINUE = "g";
LABEL CMD_QUIT = "q";
LABEL CMD_STEPOVER = "p";
LABEL CMD_STEPOUT = "gu";
LABEL CMD_PRINT_CALLSTACK = "k";
LABEL CMD_KILL_DBG = "taskkill /F /IM cdb.exe";


LABEL CMD_SHOW_REGISTER = "r";
LABEL REG_OUTPUT_VALID = "iopl";
LABEL MSG_END_OF_CALLSTACK32 = "00000000";
LABEL MSG_END_OF_CALLSTACK64 = "00000000`00000000";


#ifdef _WIN64
LABEL CMD_PRINT_STR = ".printf \"%%mu\", %p";
wchar_t CMD_LAUNCH_DBG[] = L"\"C:\\Program Files (x86)\\Windows Kits\\10\\Debuggers\\x64\\cdb.exe\" -hd ";
LABEL CMD_PRINT_ARG = ".printf \"%%p\", %s";
#elif _WIN32
LABEL CMD_PRINT_STR = ".printf \"%%mu\", %x";
wchar_t CMD_LAUNCH_DBG[] = L"\"C:\\Program Files (x86)\\Windows Kits\\10\\Debuggers\\x86\\cdb.exe\" -hd ";
LABEL CMD_PRINT_ARG = ".printf \"%%x\", poi(esp+%d)";
#endif

DWORD totalRead = 0;


wchar_t* targetFileName = NULL;
STARTUPINFO si = { 0, };
PROCESS_INFORMATION pi = { 0, };

	int Debugger::setup(wchar_t** eArgs, wchar_t* fName) //create pipe and process
	{
		targetFileName = fName;
		
		// Create security attributes to create pipe.
		SECURITY_ATTRIBUTES saAttr = { sizeof(SECURITY_ATTRIBUTES) };
		saAttr.bInheritHandle = TRUE; // Set the bInheritHandle flag so pipe handles are inherited by child process. Required.
		saAttr.lpSecurityDescriptor = NULL;
		if (!CreatePipe(&readPipe, &writePipe, &saAttr, 1)) { //actually has no buffer
			ERROR("Create Pipe Failed!\n");
			return 0;
		}

		si.cb = sizeof(STARTUPINFO);
		si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES; // STARTF_USESTDHANDLES is Required.
		si.hStdOutput = writePipe; // Requires STARTF_USESTDHANDLES in dwFlags.
		si.hStdInput = readPipe;
		si.hStdError = writePipe; // Requires STARTF_USESTDHANDLES in dwFlags.
		// si.hStdInput remains null.

		//si.wShowWindow = SW_HIDE; // Prevents cmd window from flashing. Requires STARTF_USESHOWWINDOW in dwFlags.
		//wchar_t cmd[100] = { 0, };
		
		//wchar_t* tmpCmd = (wchar_t*)calloc(wcslen(CMD_LAUNCH_DBG) + wcslen(eName) + 10, sizeof(wchar_t));
		//wcscpy_s(tmpCmd, 70, CMD_LAUNCH_DBG);

		
		int exeCmdLength = 0;
		int exeArgCount = _msize(eArgs) / sizeof(wchar_t*);
		for (int i = 0; i < exeArgCount; i++) {
			exeCmdLength += _msize(eArgs[i]) / sizeof(wchar_t);
		}
		exeCmdLength += 1;

		wchar_t* exeCmd = (wchar_t*)calloc(exeCmdLength, sizeof(wchar_t*));
		//bring each arg and strcat
		for (int i = 0; i < exeArgCount ; i++) {
			wcscat(exeCmd, eArgs[i]);
		}

		wchar_t* cmd = (wchar_t*) calloc( wcslen(CMD_LAUNCH_DBG) + exeCmdLength + 10, sizeof(wchar_t) );

		wcscpy(cmd, CMD_LAUNCH_DBG);
		wcscat(cmd, exeCmd);
		
		INFO("Initiating command : %ws\n", cmd);
		
		

		if (!CreateProcessW(
			NULL,
			(TCHAR*) cmd,
			NULL,
			NULL,
			TRUE,               // TRUE=handles are inherited. Required.
			CREATE_NEW_CONSOLE, // creation flags
			NULL,               // use parent's environment
			NULL,               // use parent's current directory
			&si,                // __in, STARTUPINFO pointer
			&pi)
			) {
			ERROR("Debugger Launch Failed!\n");
			return 0;
		}
		
		
		WriteLine(CMD_BP_CREATE, 1);
		INFO("windbg initiated");
		//waitForResponse((char*)MSG_BP_INIT); //input prompt not displays
		WriteLine(CMD_BP_CLOSE, 1);
		//waitForResponse((char*) MSG_BP_INIT);
		INFO("BP set");

	}
	
	char* Debugger::waitForResponse(char* target, int getResponse) {
		char* received;
		VERB("waiting for %s", target);
		//wait for string
		for (;;) {
		
			received = ReadLine();
			if (strstr(received, target)) //if 'breakpoint n defined' displays -> pass
				break;
			free(received);
		}
		VERB("target received");
		if (getResponse)
			return received;
		else
			free(received);
		return NULL;
		//FlushBuffer();
	}
	void Debugger::waitForInput() {
		char* response = NULL;
		do {
		if(response)
			free(response);
		response = waitForResponse((char*)">", 1);
		} while (response && !(response[1] == ':' && response[5] == '>')); //prompt : "n:nnn>"
		if(response)
			free(response);

	}
	int Debugger::run() { //loop, when break -> onBreak, exit -> onExit()
		//waitForResponse((char*) MSG_BP_INIT);
		WriteLine(CMD_CONTINUE, 1);
		INFO("running program");
		for (;;) {
			char* buf = ReadLine();
			if (strlen(buf) != 0) {

				//example message : Breakpoint N hit
				if (!strncmp(buf, BREAK_SYMBOL, strlen(BREAK_SYMBOL))) { //paused by brakepoint
					VERB("Breakpoint Hit");
					strtok(buf, " "); //cut of 'Breakpoint'
					int funcIndex = strtol(strtok(NULL, " "), NULL, 16); //get breakpoint index(=function index)
					fetchRegister();
					ADDR breakAddr = reg[6]; //eip or rip
					onBreak(funcIndex);
					WriteLine(CMD_CONTINUE, 1);

				}
				else if (strstr(buf, BREAK_EXCEPTION_SYMBOL) >(char*) 0) {//break instruction exception
					VERB("Breakpoint instruction exception rasied, ignoring...");
					WriteLine(CMD_CONTINUE, 1);
				}
				else if (!strncmp(buf, TERMINATE_SYMBOL, strlen(TERMINATE_SYMBOL))) { //process termintaed
					INFO("Process Terminated");
					break;
				}
			}
			free(buf);
		}

		return 0;
	}
	int Debugger::stop() {
		WriteLine(CMD_QUIT, 1);
		TerminateProcess(pi.hProcess, 0);
		return -1;
	}
	int Debugger::onBreak(int funcIndex) {
		if (funcIndex == funcList::FCreateFile) {
			VERB("File Opened");

			Instruction* i = new Instruction();
			ADDR arg = getArg(1);
			char* argstr = getStr(arg); //createfilew's argument
			i->fileName = (wchar_t*)calloc(strlen(argstr) + 1, sizeof(wchar_t));
			mbstowcs(i->fileName, argstr, strlen(argstr));
			free(argstr);

			WriteLine(CMD_STEPOUT, 1);
			fetchRegister();
			i->handle = getReturn();
			i->callStack = getCallStack();
			
			instStack.push_back(i);
		}
		else if (funcIndex == funcList::FCloseHandle) {
			VERB("File Closed");

			if (instStack.empty()) {
				ERROR("Instruction stack is empty. Something went wrong!");
				return -1;
			}
			//pop pair
			InstStack::reverse_iterator pairIndex = getPair(&instStack, getArg(1));
			
			if (pairIndex == instStack.rend()) {
				WARN("skipping this function ...");
				return -1;
			}

			
			Instruction* openInst = *pairIndex;
			
			CallStack* openCall = openInst->callStack;
			CallStack* closeCall = getCallStack();
			FuncPair pair = getPairBelowLCA(openCall, closeCall); 
			if (pair.openFunc != NULL && pair.closeFunc != NULL) {
				do {
					if (targetFileName) {
						if (wcscmp(targetFileName, openInst->fileName))
							break;
					}else
						INFO("filename : %ws", openInst->fileName);
					INFO("Target function is...");
					if (pair.lcaFunc) //if parent exists
						INFO("From function %s ...", pair.lcaFunc->funcName);
					else
						WARN("No common parent found");
					INFO("open : %s", pair.openFunc->funcName);
					INFO("close : %s", pair.closeFunc->funcName);
				} while (0);
			}
			/*if (pair.openFunc != NULL && pair.closeFunc != NULL) {
				if (!targetFileName) {
					INFO("Target function is...");
					INFO("filename : %ws", openInst->fileName);
					if (pair.lcaFunc) //if parent exists
						INFO("From function %s ...", pair.lcaFunc->funcName);
					else
						WARN("No common parent found");
					INFO("open : %s", pair.openFunc->funcName);
					INFO("close : %s", pair.closeFunc->funcName);
				}
				else {
					if (!wcscmp(targetFileName, openInst->fileName)) {
						INFO("Target function is...");
						if (pair.lcaFunc) //if parent exists
							INFO("From function %s ...", pair.lcaFunc->funcName);
						else
							WARN("No common parent found");
						INFO("open : %s", pair.openFunc->funcName);
						INFO("close : %s", pair.closeFunc->funcName);
					}
				}
			}*/
			//erase
			CleanCallstack(openCall);
			CleanCallstack(closeCall);
			instStack.erase(std::next(pairIndex).base());
			
		} //리턴받을때? writefile에 p한번 보내고 fetch register
		
		return 0;
	}
	
	void Debugger::fetchRegister() {



		WriteLine(CMD_SHOW_REGISTER, 1);
		char buf[PIPE_MAX_BUF + 3] = { 0, };
		
		//parse from eax to edi
		char* line;
		do{ //read until correct output reaches
			
			
			line = ReadLine();
			strcat(buf, line);
			free(line);

		}while (!strstr(buf, REG_OUTPUT_VALID));

		

		char* start = buf;
			for (int i = 0; i < 9; i++) { //reg info is devided by space or enter
			#ifdef _WIN64
			reg[i] = _strtoui64(start + 4, NULL, 16); 
			#elif _WIN32
			reg[i] = strtol(start + 4, NULL, 16);
			#endif


			char* next1 = strchr(start, ' ');
			char* next2 = strchr(start, '\n');
			if (next1 && next1 < next2)
				start = next1 + 1;
			else
				start = next2 + 1;
		}
		
		//parse from ip to bp
		VERB("Register parsed (ex. eax=%p eip=%p)", reg[0], reg[6]);
	}
	CallStack* Debugger::getCallStack() {
		WriteLine(CMD_PRINT_CALLSTACK, 1);
		CallStack* callStack = new CallStack();
		BOOL enteredLastLine = false; //" 00000000" printed
		BOOL EOLReached = false; // end of last line
		//콜스택이 여러줄 뜨는거 수정해야됨.
		char buf[PIPE_MAX_BUF + 3] = { 0, };
		int inputLength = 0;
		while (1) {
			
			//입력 한번에 받기
			char* tmpbuf = ReadLine(); 
			int tmpBufLen = strlen(tmpbuf);
			strcat(buf, tmpbuf);
			inputLength += tmpBufLen;

			if (!enteredLastLine) {
			#ifdef _WIN64
				enteredLastLine = (BOOL)strstr(tmpbuf, MSG_END_OF_CALLSTACK64);
			#elif _WIN32
				enteredLastLine = (BOOL)strstr(tmpbuf, MSG_END_OF_CALLSTACK32);
			#endif
			}
			EOLReached = tmpbuf[tmpBufLen - 1] == 10;


			if (enteredLastLine && EOLReached) { //RetAddr == 0x0 && endofline
				free(tmpbuf);
				break;
			}
			free(tmpbuf);
		}
		char* curBufPtr = buf;
		char* curBufEnd = buf + strlen(buf) - 1;
			//parse by line
			while(curBufPtr < curBufEnd){
				
			//implement strtok('\n')
			char* curLineEnd = strchr(curBufPtr, '\n');
			if (curLineEnd == 0) {
				curLineEnd = strchr(curBufPtr, '\0');
			}
			*curLineEnd = '\0';
			 
				if ( '0' <= curBufPtr[0] && curBufPtr[0] <= '9') { //first letter of 'ChildEBP', "WARNING" + exception for pipe read error
					 //hex input 'C' as number... so we have to filter it by letter 'C'

					CallFrame* callFrame = new CallFrame();
					
					ADDR retAddr, childEbp;
					#ifdef _WIN64
					ADDR upperDword, lowerDword;
					upperDword = _strtoui64(strtok(curBufPtr, "`"), NULL, 16);
					lowerDword = _strtoui64(strtok(NULL, " "), NULL, 16);
					childEbp = (upperDword << 32) + lowerDword;

					upperDword = _strtoui64(strtok(NULL, "`"), NULL, 16);
					lowerDword = _strtoui64(strtok(NULL, " "), NULL, 16);
					retAddr = (upperDword << 32) + lowerDword;
					#elif _WIN32
					childEbp = strtol(strtok(curBufPtr, " "), NULL, 16);
					retAddr = strtol(strtok(NULL, " "), NULL, 16);
			

					#endif
					callFrame->childEbp = childEbp;
					callFrame->returnAddr = retAddr;

					//parse function name
					char* fName = strtok(NULL, "     ");
					if (fName) {
						char* funcNameBuf = (char*)malloc(sizeof(char) * (strlen(fName) + 1));
						strcpy(funcNameBuf, fName);
						callFrame->funcName = funcNameBuf;
					}
					else {
						callFrame->funcName = 0;
					}

					//VERB("[Callstack] ebp : %p, ret : %p, funcName : %s", callFrame->childEbp, callFrame->returnAddr, callFrame->funcName);
					callStack->push(callFrame);
					if (callFrame->returnAddr == 0) //end of callstack
					{
						break;
					}
				}
			
			curBufPtr = curLineEnd + 1;
			}
		
		return callStack;
	}
	void Debugger::CleanCallstack(CallStack* stack) {

		while (!stack->empty()) {
			CallFrame* frame = stack->top();
			if (frame->funcName) {
				free(frame->funcName); //clear function name address
				frame->funcName = (char*) 0;
			}
			stack->pop();
		}
	}
	char* Debugger::ReadLine() {
		DWORD dwRead;
		char* buf = (char*) calloc(PIPE_MAX_BUF + 3, sizeof(char));
		
		DELAY();
		ReadFile(readPipe, buf, PIPE_MAX_BUF, &dwRead, NULL);
		
		
		buf[dwRead] = '\0';
		VERB("Reading from debugger : %s", buf);
		//VERB("read complete");
		//CloseHandle(readPipe);
		return buf;
	}
	void Debugger::WriteLine(char* cmd, int wait) {
		WriteLine((const char*)cmd, wait);
	}
	
	void Debugger::WriteLine(const char* cmd, int wait) {
		
		if(wait)
			waitForInput();
		DWORD dwWrite;
		DWORD dwTmpRead;
		DELAY();
		char* buf = (char*)calloc(PIPE_MAX_BUF + 3, sizeof(char));
		strncpy(buf, cmd, PIPE_MAX_BUF);
		int inputLen = strlen(cmd);
		buf[inputLen] = '\n';
		buf[inputLen + 1] = '\0';
		VERB("Sending to debugger : %s", buf);
		
		
		
		WriteFile(writePipe, buf, strlen(buf), &dwWrite, NULL);
		free(buf);
		//VERB("send complete");
		//CloseHandle(writePipe);
	}
	int Debugger::getReturn() {
		return reg[0];//return EAX or RAX
	}
	ADDR Debugger::getArg(int index) {


		fetchRegister(); //faster method than .printf
		char cmd[PIPE_MAX_BUF + 3] = { 0, };
		ADDR r;
	#ifdef _WIN64 //64bit argument processing
		int argnum = 0;
	switch (index) {
	case 1:
		argnum = 2;
		break;
	case  2:
		argnum = 3;
		break;
	default:
		//will be implemented if needed(r8, r9 ...)
		break;
	}
		//sprintf(cmd, CMD_PRINT_ARG, reg);
	r = reg[argnum];
	#elif _WIN32  //32bit argument processing
		sprintf(cmd, CMD_PRINT_ARG, index*4); //copy print_arg command to cmd buffer
		WriteLine(cmd, 1);
		char* arg = ReadLine(); //get result
		r = strtol(arg, NULL, 16);
		free(arg);
	#endif
		
		return r;

		//inplement processing 64bit argument processing

	}

	char* Debugger::getStr(ADDR addr) {
		char cmd[PIPE_MAX_BUF + 3] = { 0, };
		sprintf(cmd, CMD_PRINT_STR, addr);
		WriteLine(cmd, 1);
		char* value = ReadLine(); //file name included
		return value;
	}
