
// Joke #3
// ���� �� ���������, �� ����� ������ �������� ����� ������������� �����.

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

