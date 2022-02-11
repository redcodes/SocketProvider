#include "pch.h"
#include "CSocketProvider.h"
#include <Ws2tcpip.h>
#include "ProviderService/CIPC.h"
#include "winsock.h"
#include "ProviderService/Log.h"

const TCHAR* IPCNAME = _T("\\\\.\\pipe\\namedpipeServer");
LPWSAPROTOCOL_INFOW GetProvider(LPINT lpnTotalProtocols)
{
	DWORD dwSize = 0;
	int nError;
	LPWSAPROTOCOL_INFOW pProtoInfo = NULL;

	if (::WSCEnumProtocols(NULL, pProtoInfo, &dwSize, &nError) == SOCKET_ERROR)
	{
		if (nError != WSAENOBUFS)
			return NULL;
	}

	pProtoInfo = (LPWSAPROTOCOL_INFOW)::GlobalAlloc(GPTR, dwSize);
	*lpnTotalProtocols = ::WSCEnumProtocols(NULL, pProtoInfo, &dwSize, &nError);

	return pProtoInfo;
}


void FreeProvider(LPWSAPROTOCOL_INFO pProtoInfo)
{
	::GlobalFree(pProtoInfo);
}

DWORD GetCatalogEntryId(LPWSAPROTOCOL_INFO lpProtocolInfo)
{
	DWORD id = 0;

	// 如果是分层协议或者基础协议，则直接返回协议的目录项
	if (lpProtocolInfo->ProtocolChain.ChainLen <= 1)
	{
		id = lpProtocolInfo->dwCatalogEntryId;
	}
	else // 如果是协议链，则返回协议链最下层的基础协议的ID。注意：协议链中索引从0开始为分层协议，最后一层是基础协议。
	{
		int index = lpProtocolInfo->ProtocolChain.ChainLen - 1;
		id = lpProtocolInfo->ProtocolChain.ChainEntries[index];
	}

	return id;
}

int FindRules(ULONG srcIP, USHORT srcPort, ULONG& destIP, USHORT& destPort)
{
	CIPC* pIPC = CIPC::GetInstance();
	int result = 0;
	do
	{
		if ((result = pIPC->Connect(IPCNAME)) != 0)
		{
			LOG_INFO(_T("pipe connect failed = %d"), result);
			break;
		}


		const int BUFSIZE = 10;
		DWORD pid = GetCurrentProcessId();
		BYTE buf[BUFSIZE] = { 0 };
		memcpy(buf, &srcIP, sizeof(srcIP));
		memcpy(buf + sizeof(srcIP), &srcPort, sizeof(srcPort));
		memcpy(buf + sizeof(srcIP) + sizeof(srcPort),&pid,sizeof(pid));
		pIPC->Write(buf, BUFSIZE);

		ZeroMemory(buf, sizeof(buf));
		DWORD readed = 0;
		pIPC->Read(buf, 6, readed);
		memcpy(&destIP, buf, sizeof(destIP));
		memcpy(&destPort, buf + sizeof(destIP), sizeof(destPort));

		result = 0;

	} while (FALSE);

	pIPC->Close();
	delete pIPC;
	pIPC = NULL;

	return result;
}

static WSPPROC_TABLE m_nextWSProcTable;

CSocketProvider::CSocketProvider()
{
	ZeroMemory(&m_nextWSProcTable, sizeof(m_nextWSProcTable));
}

CSocketProvider::~CSocketProvider()
{
}

int WSPAPI
CSocketProvider::WSPConnect(
	SOCKET                s,
	const struct sockaddr FAR* name,
	int                   namelen,
	LPWSABUF              lpCallerData,
	LPWSABUF              lpCalleeData,
	LPQOS                 lpSQOS,
	LPQOS                 lpGQOS,
	LPINT                 lpErrno
)
{
	LOG_INFO(_T("WSPConnect"));

	BOOL bRedirect = FALSE;
	ULONG srcIP = ((sockaddr_in*)name)->sin_addr.S_un.S_addr;
	USHORT srcPort = ((sockaddr_in*)name)->sin_port;
	ULONG destIP = 0;
	USHORT destPort = 0;

	int type = 0;
	int length = sizeof(int);

	getsockopt(s, SOL_SOCKET, SO_TYPE, (char*)&type, &length);
	LOG_INFO(_T("Socket Type Is %d"),type);
	if (type == SOCK_STREAM)
		bRedirect = TRUE;
	else if (type == SOCK_DGRAM)
		bRedirect = FALSE;

	if (bRedirect && 0 == FindRules(srcIP, srcPort, destIP, destPort) && (destIP != 0 && destPort != 0))
	{
		SOCKADDR_IN transferSrv;
		transferSrv.sin_family = AF_INET;
		transferSrv.sin_addr.s_addr = destIP;
		transferSrv.sin_port = destPort;
		int result = m_nextWSProcTable.lpWSPConnect(s, (SOCKADDR*)&transferSrv, sizeof(SOCKADDR), lpCallerData, lpCalleeData, lpSQOS, lpGQOS, lpErrno);

		if (SOCKET_ERROR == result)
		{
			LOG_INFO(_T("LastError = %u\n"), *lpErrno);
			if (*lpErrno == WSAEWOULDBLOCK)
			{
				fd_set Write, Err;
				TIMEVAL Timeout;

				FD_ZERO(&Write);
				FD_ZERO(&Err);
				FD_SET(s, &Write);
				FD_SET(s, &Err);

				Timeout.tv_sec = 60;
				Timeout.tv_usec = 0;

				result = m_nextWSProcTable.lpWSPSelect(0,			//ignored
					NULL,		//read
					&Write,	//Write Check
					&Err,		//Error Check
					&Timeout, lpErrno);
			}
		}

		if (SOCKET_ERROR != result)
		{
			// 连接成功之后发送真实应用地址
			char appAddress[6] = { 0 };
			memcpy(appAddress, &srcIP, sizeof(srcIP));
			memcpy(appAddress + sizeof(srcIP), &srcPort, sizeof(srcPort));
			::send(s, appAddress, sizeof(appAddress), 0);
		}

		return result;
	}

	return m_nextWSProcTable.lpWSPConnect(s, name, namelen, lpCallerData, lpCalleeData, lpSQOS, lpGQOS, lpErrno);
}

int WSPAPI CSocketProvider::WSPSendTo(
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
)
{
	LOG_INFOA(__FUNCDNAME__);
	ULONG srcIP = ((sockaddr_in*)lpTo)->sin_addr.S_un.S_addr;
	USHORT srcPort = ((sockaddr_in*)lpTo)->sin_port;

	ULONG destIP = 0;
	USHORT destPort = 0;

	if (0 == FindRules(srcIP, srcPort, destIP, destPort) && (destIP != 0 && destPort != 0))
	{
		SOCKADDR_IN transferSrv;
		transferSrv.sin_family = AF_INET;
		transferSrv.sin_addr.s_addr = destIP;
		transferSrv.sin_port = destPort;
		int result = m_nextWSProcTable.lpWSPSendTo(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, (SOCKADDR*)&transferSrv, iTolen, lpOverlapped, lpCompletionRoutine, lpThreadId, lpErrno);
		if (SOCKET_ERROR == result)
		{
			LOG_INFO(_T("WSPSendTo LastError = %u\n"), *lpErrno);
		}

		return result;
	}

	return m_nextWSProcTable.lpWSPSendTo(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, lpTo, iTolen, lpOverlapped, lpCompletionRoutine, lpThreadId, lpErrno);
}

int CSocketProvider::Initialize(
	WORD                wVersion,
	LPWSPDATA           lpWSPData,
	LPWSAPROTOCOL_INFOW lpProtocolInfo,
	WSPUPCALLTABLE      UpCallTable,
	LPWSPPROC_TABLE     lpProcTable)
{
	LOG_INFO(_T("Initialize"));

	int result = 0;

	int nTotalProtos = 0;
	LPWSAPROTOCOL_INFO pProtoInfos = NULL;

	do
	{
		// 遍历注册表中所有的LSP
		pProtoInfos = GetProvider(&nTotalProtos);

		// 获取下层协议的目录项
		DWORD dwCatalogEntryID = GetCatalogEntryId(lpProtocolInfo);

		// 获取下层协议信息
		WSAPROTOCOL_INFO nextProcotolInfo = { 0 };
		for (int i = 0; i < nTotalProtos; i++)
		{
			if (dwCatalogEntryID == pProtoInfos[i].dwCatalogEntryId)
			{
				memcpy(&nextProcotolInfo, &pProtoInfos[i], sizeof(WSAPROTOCOL_INFO));
				break;
			}
		}

		// 获取下层LSP的DLL路径
		TCHAR szBaseProviderDll[MAX_PATH] = { 0 };
		int nLen = MAX_PATH;
		int nErrcode = 0;
		if (SOCKET_ERROR == ::WSCGetProviderPath(&nextProcotolInfo.ProviderId, szBaseProviderDll, &nLen, &nErrcode))
		{
			result = WSAEPROVIDERFAILEDINIT;
			break;
		}

		if (!::ExpandEnvironmentStrings(szBaseProviderDll, szBaseProviderDll, MAX_PATH))
		{
			result = WSAEPROVIDERFAILEDINIT;
			break;
		}

		// 加载下层LSP
		HMODULE hModule = ::LoadLibrary(szBaseProviderDll);
		if (hModule == NULL)
		{
			result = WSAEPROVIDERFAILEDINIT;
			break;
		}

		LPWSPSTARTUP pfnWSPStartup = NULL;

		pfnWSPStartup = (LPWSPSTARTUP)::GetProcAddress(hModule, "WSPStartup");
		if (pfnWSPStartup == NULL)
		{
			result = WSAEPROVIDERFAILEDINIT;
			break;
		}

		// 调用其WSPStartup函数。注意！！！：如果下层协议是基础协议，则要传递基础协议信息。
		LPWSAPROTOCOL_INFOW pPassInfo = lpProtocolInfo;
		if (nextProcotolInfo.ProtocolChain.ChainLen == BASE_PROTOCOL)
		{
			pPassInfo = &nextProcotolInfo;
		}

		int nRet = pfnWSPStartup(wVersion, lpWSPData, pPassInfo, UpCallTable, lpProcTable);
		if (nRet != ERROR_SUCCESS)
		{
			break;
		}

		// 保存下层调用表
		m_nextWSProcTable = *lpProcTable;

		lpProcTable->lpWSPConnect = &CSocketProvider::WSPConnect;
		lpProcTable->lpWSPSendTo = &CSocketProvider::WSPSendTo;

	} while (FALSE);

	if (NULL != pProtoInfos)
	{
		FreeProvider(pProtoInfos);
		pProtoInfos = NULL;
	}

	return 0;
}

void CSocketProvider::UnInitialize()
{
}
