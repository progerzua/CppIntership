
// Joke #3
// ≈сли вы бисексуал, то шансы хорошо провести вечер увеличиваютс€ вдвое.

#pragma once

#include <fstream>
#include <string>
#include <mutex>

using namespace std;

class Logger
{
	mutex mut;
	wofstream out;
	wstring fileName;


public:

	Logger(wstring fileName);
	void log(wstring message);
	~Logger();
};

