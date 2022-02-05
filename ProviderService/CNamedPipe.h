#pragma once
#include "CIPC.h"

class CNamedPipe : public CIPC
{
public:
	CNamedPipe();
	virtual ~CNamedPipe();

	virtual int Bind(LPCTSTR name, int backlog) override;
	virtual int Connect(LPCTSTR name, DWORD timeout = INFINITE) override;
	virtual int Close() override;
	virtual int Write(void* buf, DWORD bufSize) override;
	virtual int Read(void* buf, DWORD bufSize, DWORD& readed) override;
private:
	HANDLE m_hPipe;
	OVERLAPPED m_ov;
	HANDLE m_hEvent;
};

