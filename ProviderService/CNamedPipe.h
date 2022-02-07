#pragma once
#include "CIPC.h"
#include <list>

#define BUFSIZE 4096
typedef struct
{
	OVERLAPPED oOverlap;
	HANDLE hPipe;
	char chRequest[BUFSIZE];
	DWORD cbRead;
	char chReply[BUFSIZE];
	DWORD cbToWrite;
	DWORD dwState;
	BOOL  fPendingIO;
} PIPEINST, * LPPIPEINST;

class CNamedPipe : public CIPC
{
public:
	CNamedPipe();
	virtual ~CNamedPipe();

	virtual int Bind(LPCTSTR name, int backlog, IPCCALLBACK callback, void* param) override;
	virtual int Connect(LPCTSTR name, DWORD timeout = INFINITE) override;
	virtual int Close() override;
	virtual int Write(void* buf, DWORD bufSize) override;
	virtual int Read(void* buf, DWORD bufSize, DWORD& readed) override;
private:
	int Listen(PIPEINST* pipe,LPCTSTR name,HANDLE hEvent);
	int RunLoop(DWORD dwTimeout = INFINITE);
	void Clear();
	void DisconnectAndReconnect(PIPEINST* pipe);
	BOOL ConnectToNewClient(HANDLE hPipe, LPOVERLAPPED lpo);
private:
//	std::list<PIPEINST*> m_pipeLists;
	PIPEINST* m_pipes;
	int m_pipeCount;
	HANDLE* m_hEvents;

	IPCCALLBACK m_callback;
	void* m_callbackParam;
};

