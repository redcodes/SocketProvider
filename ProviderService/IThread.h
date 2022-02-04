#pragma once
#include <windows.h>

class IRunnable
{
public:
	virtual ~IRunnable() = 0 {}
	virtual void Start() = 0;
	virtual void Run() = 0;
	virtual void Stop() = 0;
};
class IThread
{
public:
	virtual ~IThread() = 0 {}
	virtual int Start() = 0;
	virtual void Stop() = 0;
	virtual int Join(int nTimeout = INFINITE) = 0;
};