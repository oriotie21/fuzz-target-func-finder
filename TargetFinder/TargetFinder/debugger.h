#pragma once
#ifndef DEBUGGER_H
#define DEBUGGER_H

#include "instruction.h"
#include <list>



enum funcList {
	FCreateFile,
	FCloseHandle,
};


class Debugger {
public:
	HANDLE readPipe;
	HANDLE writePipe;
	ADDR reg[9]; //ax, bx, cx, dx, si, di;
				   //ip, sp, bp;
	InstStack instStack;

	int onBreak(int);
	void fetchRegister();
	CallStack* getCallStack();
	void CleanCallstack(CallStack*);
	int run();
	int stop();
	int setup(wchar_t** eArgs, wchar_t* fName);
	char* waitForResponse(char* target, int getResponse);
	void waitForInput();
	char* ReadLine();
	void WriteLine(char* cmd, int wait);
	int getReturn();
	ADDR getArg(int index);
	char* getStr(ADDR addr);
	void WriteLine(const char* cmd, int wait);

};


#endif