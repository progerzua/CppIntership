// Additional C++ task for Kodisoft Intership
// That i made very fast(in 36 hours)
// So you could find some mistakes here.

// Can you find 4 jokes here?

// Joke #1
// Ну и разврат! - произнес дождевой червь, увидев тарелку спагетти.

#include "stdafx.h"
#include "ProcessManager.h"
#include "Logger.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <sstream>

using namespace std;

void callbackStart(){
	cout << "Started" << endl;
}

void callbackCrash(){
	cout << "Crash" << endl; 
}

void callbackStopped(){
	cout << "ManuallyStopped" << endl;
}


int _tmain(int argc, _TCHAR* argv[])
{
	Logger logger(L"log.log");
	
	wstring commandLine;

	cout << "Write path to file(C:\\Windows\\notepad.exe for example) : " << endl ;
	wcin >> commandLine;
	ProcessManager pm((commandLine.c_str()), &logger);

	pm.setOnProcStart(bind(callbackStart));
	pm.setOnProcCrash(bind(callbackCrash));
	pm.setOnProcManuallyStopped(bind(callbackStopped));

	if (pm.getCommandLine() != NULL)
		wcout << "Path was succesfully read "<< pm.getCommandLine() << endl;


	cout <<"PID: "<< pm.getPID() << endl;

	cout  << endl << "Commands: "<< endl << "  -start" << endl << "  -stop" << endl << "  -restart" << endl << "  -exit" << endl;
	wstring command;

	do
	{
		wcin >> command;
		if (command == L"stop") pm.stop();
		if (command == L"start") pm.start();
		if (command == L"restart") pm.restart();
		if (command == L"exit") break;

	} while (true);

	


	return 0;
}

