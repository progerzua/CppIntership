/*
ProcessManager
It can perform basic operations with processes: start, stop, resume and restart processes
*/

// Joke #2
// ћедленный компьютер Ч это когда ты знаешь имена всех разработчиков Adobe Photoshop.

#pragma once

#include <Windows.h>
#include <thread>
#include <mutex>
#include <functional>
#include <string>
#include <condition_variable>
#include "Logger.h"

using namespace std;


enum ProcessStatus{
	IS_WORKING, RESTARTING, STOPPED
};


class ProcessManager
{
	enum LastAction{
		START, STOP, RESTART
	};

	// Mutex for thread safety
	mutable mutex processStatusMutex, 
		lastActionMutex, 
		startCallbackMutex, 
		crashCallbackMutex, 
		manualStopCallbackMutex, 
		processInformationMutex,
		operationMutex;


	condition_variable actionFinishedCondition; // To prevent error, when another action not finished
	bool previousActionFinished;
	LastAction lastAction;  // To restart process if it was closed by itself
	LastAction getLastAction();
	void setLastAction(LastAction lastAction);
	ProcessStatus processStatus;

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	wchar_t *commandLine;
	DWORD pid;
	HANDLE pHandle; // Process Handle
	HANDLE tHandle; // Thread Handle
	Logger* logger;
	
	thread waiter; // To wait for process closing in another thread
	function<void()> onProcStart;
	function<void()> onProcCrash;
	function<void()> onProcManuallyStopped;

	bool runProc();
	bool stopProc();
	bool restartProc();
	void waitForProcess();

	void updateStatus(ProcessStatus processStatus);
	void logMessage(const wchar_t* message, bool showPID = false);


public:

	ProcessManager(const wchar_t* commandLine, Logger* logger = NULL);
	bool restart();
	bool stop();
	bool start();
	ProcessStatus getStatus() const;
	HANDLE getHandle() const;
	DWORD getPID() const;
	wchar_t* getCommandLine() const;

	void setOnProcStart(function<void()> onProcStart);
	void setOnProcCrash(function<void()> onProcCrash);
	void setOnProcManuallyStopped(function<void()> onProcManuallyStopped);

	~ProcessManager();
};
