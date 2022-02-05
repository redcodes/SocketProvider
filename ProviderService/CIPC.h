#pragma once
#include <windows.h>

class CIPC
{
public:
	virtual ~CIPC() = 0 {};

	virtual int Bind(LPCTSTR name,int backlog) = 0;

	virtual int Connect(LPCTSTR name, DWORD timeout = INFINITE) = 0;
	virtual int Close() = 0;

	virtual int Write(void* buf,DWORD bufSize) = 0;
	virtual int Read(void* buf, DWORD bufSize, DWORD& readed) = 0;

	static CIPC* GetInstance();
};

