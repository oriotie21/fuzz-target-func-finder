#pragma once
#ifndef INST_H
#define INST_H
#include <stack>
#include <list>

#ifdef _WIN64
typedef unsigned __int64 ADDR;

#elif _WIN32
typedef unsigned int ADDR;

#endif

using namespace std;






struct CallFrame {
	ADDR childEbp;
	ADDR returnAddr; //not used now
	char* funcName;
};
typedef stack<CallFrame*> CallStack;

struct Instruction {
	int handle;
	wchar_t* fileName;
	CallStack* callStack;

};
typedef std::list<Instruction*> InstStack;//스택이지만, 중간 원소를 뺄수있음, 후입선출 우선

struct FuncPair {
	CallFrame* lcaFunc;
	CallFrame* openFunc;
	CallFrame* closeFunc;
};


InstStack::reverse_iterator getPair(InstStack*, ADDR);
FuncPair getPairBelowLCA(CallStack*, CallStack*);





#endif