
// Joke #4
// Боря, всё детство провёл с отцом на рыбалке, поэтому он так и не научился разговаривать.


#include "ProcessManager.h"

#include <tchar.h>
#include <thread>
#include <sstream>
#include <string>
#include <winternl.h>
#include <cwchar>

using namespace std;

ProcessManager::ProcessManager(const wchar_t* commandLine,  Logger* logger){
	
	//Initialization
	this->logger = logger;
	this->commandLine = _tcsdup(commandLine);
	processStatus = ProcessStatus::STOPPED;
	lastAction = LastAction::STOP;
	previousActionFinished = true;
	start();
}

ProcessStatus ProcessManager::getStatus() const{
	lock_guard<mutex> lock(processStatusMutex);
	return processStatus;
}
HANDLE ProcessManager::getHandle() const{
	lock_guard<mutex> lock(processInformationMutex);
	return pHandle;
}
DWORD ProcessManager::getPID() const{
	lock_guard<mutex> lock(processInformationMutex);
	return pid;
}


ProcessManager::LastAction ProcessManager::getLastAction(){
	lock_guard<mutex> lock(lastActionMutex);
	return lastAction;
}
void ProcessManager::setLastAction(LastAction lastAction){
	lock_guard<mutex> lock(lastActionMutex);
	this->lastAction = lastAction;
}
void ProcessManager::updateStatus(ProcessStatus processStatus){
	lock_guard<mutex> lock(processStatusMutex);
	this->processStatus = processStatus;
}
TCHAR* ProcessManager::getCommandLine() const {
	lock_guard<mutex> lock(processInformationMutex);
	return commandLine;
}

// Partly taken from MSDN
bool ProcessManager::runProc(){	

	// Run a new process

	processInformationMutex.lock(); // Thread safety and bla-bla-bla
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));	

	if (!CreateProcess(NULL,   // No module name (use command line)
						commandLine,        // Command line
						NULL,           // Process handle not inheritable
						NULL,           // Thread handle not inheritable
						FALSE,          // Set handle inheritance to FALSE
						//CREATE_NEW_CONSOLE,//
						DETACHED_PROCESS,             
						NULL,           // Use parent's environment block
						NULL,           // Use parent's starting directory 
						&si,            // Pointer to STARTUPINFO structure
						&pi)           // Pointer to PROCESS_INFORMATION structure
						){	
		pid = 0;
		pHandle = tHandle = NULL;
		processInformationMutex.unlock();

		wstringstream message;
		message << "Process creation failed. Error = " << GetLastError();
		logMessage(message.str().c_str());	

		return false; // Failed to start process
	}


	pid = pi.dwProcessId;
	pHandle = pi.hProcess;
	tHandle = pi.hThread;
	processInformationMutex.unlock();
	
	logMessage(L"Process created", true);

	lock_guard<mutex> callbackLock(startCallbackMutex);
	if (onProcStart)thread(onProcStart).detach();

	return true;
}

// Stop already running process
bool ProcessManager::stopProc(){

	processInformationMutex.lock();
	TerminateProcess(pHandle, 1);
	DWORD exitCode;

	if (GetExitCodeProcess(pHandle, &exitCode)){
		processInformationMutex.unlock();
		if (exitCode != 259){
			logMessage(L"Process manually stopped", true);

			if(pHandle!=NULL)CloseHandle(pHandle); // Closing handles
			if(tHandle!=NULL)CloseHandle(tHandle);

			lock_guard<mutex> callbackLock(manualStopCallbackMutex);
			if (onProcManuallyStopped)thread(onProcManuallyStopped).detach();

			return true; // Process was successfully stopped
		}
		else{
			logMessage(L"Process manual shutdown failed", true);
		}		
	}
	else
		processInformationMutex.unlock();		
		
	
	return false; // Process was not stopped or we can't get exit code
}

bool ProcessManager::restartProc(){	
	if (stopProc()){
		return runProc();
	}	
	return false; // Process was not stopped =(
}

bool ProcessManager::start() {	

	unique_lock <mutex> uniqueOperationLock(operationMutex);
	while (!previousActionFinished) actionFinishedCondition.wait(uniqueOperationLock);
	previousActionFinished = false;

	// Start a new process only if current is stopped
	if (getLastAction() ==  LastAction::STOP){
		setLastAction(LastAction::START);
		if (runProc()){
			updateStatus(ProcessStatus::IS_WORKING);
			if (waiter.joinable()) waiter.join();			
			waiter = thread(&ProcessManager::waitForProcess, this);  // Wait for process closing in another thread
			
			previousActionFinished = true;
			actionFinishedCondition.notify_one(); // Next action can start working
			
			return true; // Process was successfully started
		}		
	}
	
	previousActionFinished = true;
	actionFinishedCondition.notify_one();
	return false; //Process was already running or can't start a new process
}

bool ProcessManager::stop(){
	unique_lock <mutex> uniqueOperationLock(operationMutex);
	while (!previousActionFinished) actionFinishedCondition.wait(uniqueOperationLock);// Wait until another action(start, restart) finish
	previousActionFinished = false;
	
	if (getStatus() == ProcessStatus::IS_WORKING){
		// Stop process only if it's running
		setLastAction(LastAction::STOP);
		if (stopProc()){
			updateStatus(ProcessStatus::STOPPED); // Update status
			previousActionFinished = true;
			actionFinishedCondition.notify_one(); // Next action can start working

			return true;
		}
	}

	previousActionFinished = true;
	actionFinishedCondition.notify_one(); 
	return false;
}
bool ProcessManager::restart(){

	unique_lock <mutex> uniqueOperationLock(operationMutex);
	while (!previousActionFinished) actionFinishedCondition.wait(uniqueOperationLock); // Wait until another action finish
	previousActionFinished = false;

	// Restart process only if it's running
	if (getStatus() == ProcessStatus::IS_WORKING){
		setLastAction(LastAction::RESTART);
		updateStatus(ProcessStatus::RESTARTING); 

		if (restartProc()){
			updateStatus(ProcessStatus::IS_WORKING); 
			if(waiter.joinable()) waiter.join(); // Wait for previous call of waitFunction to finish
			waiter = thread(&ProcessManager::waitForProcess, this); // Wait for process closing in another thread			
				
			previousActionFinished = true;
			actionFinishedCondition.notify_one();
			
			return true;
		}
		else {
			//if something gone wrong
			DWORD exitCode;
			if (GetExitCodeProcess(pi.hProcess, &exitCode)){
				//checking exit code
				if (exitCode == 259) updateStatus(ProcessStatus::IS_WORKING); //259==RUNNING
				else updateStatus(ProcessStatus::STOPPED); //any another exit code indicate crash or closing
			}
			else{
				//can't get exit code
				updateStatus(ProcessStatus::STOPPED);
			}
		}				
	}

	previousActionFinished = true;
	actionFinishedCondition.notify_one();
	return false; //Process wasn't restarted or process wasn't restarted correct
}

// This function waits for process closing and restart process if needed
void ProcessManager::waitForProcess(){

	do {
		WaitForSingleObject(pHandle, INFINITE); //wait for process closing

		if (getLastAction() == LastAction::START || (getLastAction() == LastAction::RESTART && getStatus() != ProcessStatus::RESTARTING)){
			
			// must restart process only if it was closed by itself
			logMessage(L"Process closed by itself", true);
			
			updateStatus(ProcessStatus::STOPPED); // update status

			crashCallbackMutex.lock();
			if(onProcCrash)thread(onProcCrash).detach(); // execute callback
			crashCallbackMutex.unlock();
			
			if (!runProc()){
				break;
			}
			else {
				updateStatus(ProcessStatus::IS_WORKING); //if we can, update status
			}
		}
		else{
			break; //we closed that process 
		}
	} while (true);
}


void ProcessManager::setOnProcStart(function<void()> onProcStart){
	lock_guard<mutex> lock(startCallbackMutex);
	this->onProcStart = onProcStart;
}
void ProcessManager::setOnProcCrash(function<void()> onProcCrash){
	lock_guard<mutex> lock(crashCallbackMutex);
	this->onProcCrash = onProcCrash;
}
void ProcessManager::setOnProcManuallyStopped(function<void()> onProcManuallyStopped){
	lock_guard<mutex> lock(manualStopCallbackMutex);
	this->onProcManuallyStopped = onProcManuallyStopped;
}


void ProcessManager::logMessage(const wchar_t* message, bool showPID){
	if (logger){
		wstringstream messageStream;
		messageStream << L"ProcessManager: ";
		if(showPID)messageStream << "[" << getPID() << "] ";
		messageStream << message;
		logger->log(messageStream.str());
	}
}


//destructor
ProcessManager::~ProcessManager(){
	stop(); 
	if(waiter.joinable())waiter.join();
	delete[] commandLine;
}