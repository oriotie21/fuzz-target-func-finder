#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <Windows.h>
#include "debugger.h"
#include <signal.h>

Debugger* debugger;

void Usage(wchar_t* arg0) {
	printf("Usage : %S ", arg0);
	printf("[-f <target filepath>] ");
	printf("<program path> [<args>]\n");

	exit(0);
}
wchar_t** pArgs; //target executable is included here
wchar_t* targetFilename = NULL;
//wchar_t* targetExecutableName = NULL;
void parseArg(int argc, wchar_t** argv) {
	int arg = 1;

	for (; arg < argc && (argv[arg][0] == '-' || argv[arg][0] == '/'); arg++)
	{
		wchar_t* argn = argv[arg] + 1;

		wchar_t* argp = argn;

		while (*argp != 0) //moves to end of option name
			argp++;
		argp++; //pass null byte
		while (*argp == ' ' || *argp == '\t') //moves to start of value
			argp++;
		switch (argn[0]) {
		case 'f'://error!!!
		case 'F':

			targetFilename = (wchar_t*) malloc(sizeof(wchar_t) * (wcslen(argp) + 1));
			wcscpy(targetFilename, argp);
			arg += 1;
			break;

		default:
			printf("%s Bad argument: %s\n", argv[0], argv[arg]);
			Usage(argv[0]);
			break;
		}
	}
	if (!argv[argc - 1] || argc < 2) //target file missing or not enough argument
	{
		Usage(argv[0]);

	}
	pArgs = (wchar_t**) malloc(sizeof(char*) * (argc - arg));
	int _arg = arg;
	for (; arg < argc; arg++)
	{
		//wchar_t* argn = argv[arg] + 1;
		wchar_t* argn = argv[arg];
	//	wchar_t* argp = pArgs[arg - _arg];
		int argi = arg - _arg;
		pArgs[argi] = (wchar_t*)malloc(sizeof(wchar_t) * (wcslen(argn) + 5));
		wcscpy(pArgs[argi], L"\"");
		wcscat(pArgs[argi], argn);
		wcscat(pArgs[argi], L"\" ");
		/*
		wchar_t* argp = argn;

		while (*argp != 0) //moves to end of option name
			argp++;
		argp++; //pass null byte
		while (*argp == ' ' || *argp == '\t') //moves to start of value
			argp++;
		switch (argn[0]) {
		
		}
		*/
	}

	
	



}
void onExit(int code) {
	debugger->stop(); //fixit
	exit(0);
}
int wmain(int argc, wchar_t** argv) {

	signal(SIGABRT, onExit);
	signal(SIGINT, onExit);
	signal(SIGTERM, onExit);

	parseArg(argc, argv);

	debugger = new Debugger();

	//debugger->setBreakPoint(CREATEFILE_W);
	//debugger->setBreakPoint(CLOSEHANDLE);

	debugger->setup(pArgs, targetFilename);
	debugger->run();

	free(targetFilename);
	
}
