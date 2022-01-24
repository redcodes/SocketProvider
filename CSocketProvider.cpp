#include "pch.h"
#include "CSocketProvider.h"
#include <Ws2tcpip.h>

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

WSPPROC_TABLE CSocketProvider::m_nextWSProcTable = { 0 };
CSocketProvider::CSocketProvider()
{
	ZeroMemory(&m_nextWSProcTable, sizeof(m_nextWSProcTable));
}

CSocketProvider::~CSocketProvider()
{
}

CSocketProvider& CSocketProvider::GetInstance()
{
	static CSocketProvider socketProvider;
	return socketProvider;
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

	SOCKADDR_IN transferSrv;
	//	inet_pton(AF_INET, "192.168.220.132", &transferSrv.sin_addr.s_addr);

	transferSrv.sin_family = AF_INET;
	transferSrv.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	transferSrv.sin_port = htons(8888);
	int result = m_nextWSProcTable.lpWSPConnect(s, (SOCKADDR*)&transferSrv, sizeof(SOCKADDR), lpCallerData, lpCalleeData, lpSQOS, lpGQOS, lpErrno);

	if (SOCKET_ERROR == result)
	{
		DWORD error = *lpErrno;
		TCHAR sLog[MAX_PATH] = { 0 };
		_stprintf_s(sLog, _T("LastError = %u\n"), error);
		OutputDebugString(sLog);
	}

	return result;
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
