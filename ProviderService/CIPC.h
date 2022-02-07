#pragma once
#include <windows.h>

typedef int (*IPCCALLBACK)(LPCSTR request,DWORD requestSize,LPSTR response, DWORD& responseSize,void* param);
class CIPC
{
public:
	virtual ~CIPC() = 0 {};

	virtual int Bind(LPCTSTR name,int backlog, IPCCALLBACK callback,void* param) = 0;

	virtual int Connect(LPCTSTR name, DWORD timeout = INFINITE) = 0;
	virtual int Close() = 0;

	virtual int Write(void* buf,DWORD bufSize) = 0;
	virtual int Read(void* buf, DWORD bufSize, DWORD& readed) = 0;

	static CIPC* GetInstance();
};

