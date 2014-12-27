#include "Logger.h"
#include <ctime>

using namespace std;

Logger::Logger(wstring fileName)
{
	out = wofstream(fileName, ofstream::out | wofstream::app);
}

void Logger::log(wstring message){

	time_t t = time(NULL);
	char info[128];
	
	if (strftime(info, sizeof(info), "%c", localtime(&t))) {	
		
		mut.lock();
		out << info << ":" << message << endl;
		mut.unlock();
		
	}
	
	else {
	
		mut.lock();
		out << message << endl;
		mut.unlock();
		
	}
}

Logger::~Logger()
{
	out.close();
}
