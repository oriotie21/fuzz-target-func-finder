#include "instruction.h"
#include "dlog.h"


InstStack::reverse_iterator getPair(InstStack* stack, ADDR arg) {
	//	instStack->instStack start move

	InstStack::reverse_iterator rit;
	VERB("target handle is 0x%p", arg);
	for (rit = stack->rbegin(); rit != stack->rend(); ++rit) {
		if ( (*rit)->handle == arg) {
			VERB("pair found!");
			return rit;
		}

	}
	WARN("pair not found");
	return rit; //if not found, rit is stack.rend
	

}

FuncPair getPairBelowLCA(CallStack* rOpenCall, CallStack* rCloseCall) {
	//CallStack* rOpenCall = new CallStack();
	//CallStack* rCloseCall = new CallStack();
	FuncPair pair;
	/*
	while (!openCall->empty()) {
		rOpenCall->push(openCall->top()); 
		openCall->pop(); //pop only delete element{doesn't return top value :(}
	}
	while (!closeCall->empty()) {
		rCloseCall->push(closeCall->top());
		closeCall->pop();
	}
	*/
	//init
	pair.openFunc = 0;
	pair.closeFunc = 0;
	pair.lcaFunc = 0;

	while (rOpenCall->top()->childEbp
		&& rCloseCall->top()->childEbp
		&& !strcmp(rOpenCall->top()->funcName, rCloseCall->top()->funcName)) {
		VERB("%x == %x", rOpenCall->top()->childEbp, rCloseCall->top()->childEbp);
		pair.lcaFunc = rOpenCall->top();
		rOpenCall->pop();
		rCloseCall->pop();

		if (rOpenCall->empty() || rCloseCall->empty()) //prevent crash, LCA not exists
			break;
		
	}
	
	if (!rOpenCall->empty() && !rCloseCall->empty()) {
		VERB("LCA Found!");
		VERB("openFunc.top : 0x%p", rOpenCall->top());
		VERB("closeFunc.top : 0x%p", rCloseCall->top());
		pair.openFunc = rOpenCall->top();
		pair.closeFunc = rCloseCall->top();
		
	}
	else {
		WARN("LCA not found!");
		
	}
	return pair;
}