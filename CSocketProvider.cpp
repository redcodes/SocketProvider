#include "pch.h"
#include "CSocketProvider.h"
#include <Ws2tcpip.h>
#include "ProviderService/CIPC.h"
#include "winsock.h"

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

	// ����Ƿֲ�Э����߻���Э�飬��ֱ�ӷ���Э���Ŀ¼��
	if (lpProtocolInfo->ProtocolChain.ChainLen <= 1)
	{
		id = lpProtocolInfo->dwCatalogEntryId;
	}
	else // �����Э�������򷵻�Э�������²�Ļ���Э���ID��ע�⣺Э������������0��ʼΪ�ֲ�Э�飬���һ���ǻ���Э�顣
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
			break;

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

	ULONG srcIP = ((sockaddr_in*)name)->sin_addr.S_un.S_addr;
	USHORT srcPort = ((sockaddr_in*)name)->sin_port;
	ULONG destIP = 0;
	USHORT destPort = 0;

	if (0 == FindRules(srcIP, srcPort, destIP, destPort) && (destIP != 0 && destPort != 0))
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
			// ���ӳɹ�֮������ʵӦ�õ�ַ
			char appAddress[6] = { 0 };
			memcpy(appAddress, &srcIP, sizeof(srcIP));
			memcpy(appAddress + sizeof(srcIP), &srcPort, sizeof(srcPort));
			::send(s, appAddress, sizeof(appAddress), 0);
		}

		return result;
	}

	return m_nextWSProcTable.lpWSPConnect(s, name, namelen, lpCallerData, lpCalleeData, lpSQOS, lpGQOS, lpErrno);
}

int CSocketProvider::Initialize(
	WORD                wVersion,
	LPWSPDATA           lpWSPData,
	LPWSAPROTOCOL_INFOW lpProtocolInfo,
	WSPUPCALLTABLE      UpCallTable,
	LPWSPPROC_TABLE     lpProcTable)
{
	OutputDebugString(_T("Initialize"));

	int result = 0;

	int nTotalProtos = 0;
	LPWSAPROTOCOL_INFO pProtoInfos = NULL;

	do
	{
		// ����ע��������е�LSP
		pProtoInfos = GetProvider(&nTotalProtos);

		// ��ȡ�²�Э���Ŀ¼��
		DWORD dwCatalogEntryID = GetCatalogEntryId(lpProtocolInfo);

		// ��ȡ�²�Э����Ϣ
		WSAPROTOCOL_INFO nextProcotolInfo = { 0 };
		for (int i = 0; i < nTotalProtos; i++)
		{
			if (dwCatalogEntryID == pProtoInfos[i].dwCatalogEntryId)
			{
				memcpy(&nextProcotolInfo, &pProtoInfos[i], sizeof(WSAPROTOCOL_INFO));
				break;
			}
		}

		// ��ȡ�²�LSP��DLL·��
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

		// �����²�LSP
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

		// ������WSPStartup������ע�⣡����������²�Э���ǻ���Э�飬��Ҫ���ݻ���Э����Ϣ��
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

		// �����²���ñ�
		m_nextWSProcTable = *lpProcTable;

		lpProcTable->lpWSPConnect = &CSocketProvider::WSPConnect;

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
