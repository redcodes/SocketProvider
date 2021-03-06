#pragma once
#include <WS2spi.h>
#pragma comment(lib,"Ws2_32.lib")
#include "ProviderService/CIPC.h"

#ifdef _WIN64
// {5A3808E9-44FE-4A49-A750-C2D9D694AA43}
static const GUID gProviderGuid =
{ 0x5a3808e9, 0x44fe, 0x4a49, { 0xa7, 0x50, 0xc2, 0xd9, 0xd6, 0x94, 0xaa, 0x43 } };
#else
// {5A3808E9-44FE-4A49-A750-C2D9D694AA43}
static const GUID gProviderGuid =
{ 0x5a3808e9, 0x44fe, 0x4a49, { 0xa7, 0x50, 0xc2, 0xd9, 0xd6, 0x94, 0xaa, 0x43 } };

#endif

class CSocketProvider
{
public:
	CSocketProvider();
	virtual ~CSocketProvider();

	int Initialize(
		WORD                wVersion,
		LPWSPDATA           lpWSPData,
		LPWSAPROTOCOL_INFOW lpProtocolInfo,
		WSPUPCALLTABLE      UpCallTable,
		LPWSPPROC_TABLE     lpProcTable);

	void UnInitialize();

private:
	static int WSPAPI WSPConnect(
		SOCKET                s,
		const struct sockaddr FAR* name,
		int                   namelen,
		LPWSABUF              lpCallerData,
		LPWSABUF              lpCalleeData,
		LPQOS                 lpSQOS,
		LPQOS                 lpGQOS,
		LPINT                 lpErrno
	);

	static int WSPAPI WSPSendTo(
		_In_ SOCKET s,
		_In_reads_(dwBufferCount) LPWSABUF lpBuffers,
		_In_ DWORD dwBufferCount,
		_Out_opt_ LPDWORD lpNumberOfBytesSent,
		_In_ DWORD dwFlags,
		_In_reads_bytes_opt_(iTolen) const struct sockaddr FAR* lpTo,
		_In_ int iTolen,
		_Inout_opt_ LPWSAOVERLAPPED lpOverlapped,
		_In_opt_ LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
		_In_opt_ LPWSATHREADID lpThreadId,
		_Out_ LPINT lpErrno
	);
};

