// ProviderService.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "CSocketChannel.h"
#include "ThreadImpl.h"
#include "../CSocketRedirectRules.h"
#include "../Log.h"
#include "CIPC.h"

LPCSTR hello = "hello,world";
CSocketRedirectRules redirectRuleList;

const TCHAR* PIPENAME = _T("\\\\.\\pipe\\namedpipeServer");

bool Init()
{
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(sockVersion, &wsaData) != 0)
	{
		return false;
	}

	return true;
}

bool UnInit()
{
	int ret = WSACleanup();
	if (ret != 0) {
		return false;
	}

	return true;
}

void TestChannel()
{
	InetSocketAddress address("127.0.0.1", 8888);
	ServerSocket serverSocket;
	serverSocket.Bind(address, 50);

	CSocketChannel* pClientChannel = serverSocket.Accept();
	pClientChannel->Write(hello, strlen(hello));

	char recvBuf[20] = { 0 };
	pClientChannel->Read(recvBuf, sizeof(recvBuf));
	printf("%s", recvBuf);

	pClientChannel->Close();
	CSocketChannel::Release(pClientChannel);
}

void ThreadProc(void* param)
{
	CSocketChannel* pLocalChannel = (CSocketChannel*)param;
	CClientSocketChannel* pRemoteChannel = NULL;

	do
	{
		// 接收真实的应用地址
		byte appAddress[6] = { 0 };
		pLocalChannel->Read((LPSTR)appAddress, sizeof(appAddress));

		ULONG remoteIP = 0;
		USHORT remotePort = 0;

		memcpy(&remoteIP, appAddress, sizeof(ULONG));
		memcpy(&remotePort, appAddress + sizeof(ULONG), sizeof(USHORT));

		// 找到代理规则，则开始收发数据
		ULONG localProxyIP = 0;
		USHORT localProxyPort = 0;
		if (0 != redirectRuleList.FindRules(remoteIP, remotePort, localProxyIP, localProxyPort))
			break;

		// 连接真实应用服务器	
		InetSocketAddress remoteAddress(remoteIP, remotePort);
		pRemoteChannel = new CClientSocketChannel(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (0 != pRemoteChannel->Connect(remoteAddress))
		{
			LOG_INFO(_T("Connect Remote Host Failed"));
			break;
		}


		while (TRUE)
		{
			byte recvBuf[4096] = { 0 };
			int nReadLength = pLocalChannel->Read((LPSTR)recvBuf, sizeof(recvBuf));
			if (nReadLength <= 0)
			{
				LOG_INFO(_T("read failed"));
				break;
			}

			LOG_INFOA("request = %s", recvBuf);

			int nWriteLength = pRemoteChannel->Write((LPSTR)recvBuf, nReadLength);
			if (nWriteLength <= 0)
			{
				LOG_INFO(_T("write failed"));
				break;
			}
		}

	} while (FALSE);

	if (NULL != pRemoteChannel)
	{
		pRemoteChannel->Close();
		CSocketChannel::Release(pRemoteChannel);
	}

	if (NULL != pLocalChannel)
	{
		pLocalChannel->Close();
		CSocketChannel::Release(pLocalChannel);
	}
}

void CmdThreadProc(void* param)
{
	while (TRUE)
	{
		CIPC* pIPC = CIPC::GetInstance();
		pIPC->Bind(PIPENAME, 50);

		const int BUFSIZE = 10;
		BYTE buf[BUFSIZE] = { 0 };
		DWORD readed = 0;
		pIPC->Read(buf, BUFSIZE, readed);

		ULONG srcIP, destIP = 0;
		USHORT srcPort, destPort = 0;
		DWORD appPID = 0;
		int isRedirect = 0;

		memcpy(&srcIP, buf, sizeof(srcIP));
		memcpy(&srcPort, buf + sizeof(srcIP), sizeof(srcPort));
		memcpy(&appPID, buf + sizeof(srcIP) + sizeof(srcPort), sizeof(appPID));
		if (appPID == GetCurrentProcessId())	// 拦截进程是本地数据代理进程，则不重定向。
			isRedirect = 0;
		else
			isRedirect = 1;

		int finded = redirectRuleList.FindRules(srcIP, srcPort, destIP, destPort);
		if (isRedirect && 0 == finded && destIP != 0 && destPort != 0)
		{
			ZeroMemory(buf, sizeof(buf));
			memcpy(buf, &destIP, sizeof(destIP));
			memcpy(buf + sizeof(destIP), &destPort, sizeof(destPort));	
			pIPC->Write(buf, 6);
		}
		else
		{
			ZeroMemory(buf, sizeof(buf));
			pIPC->Write(buf, 6);
		}

		pIPC->Close();
		delete pIPC;
		pIPC = NULL;
	}
}


void TestPipeServer()
{
	CIPC* pIPC = CIPC::GetInstance();
	pIPC->Bind(PIPENAME, 50);

	char* buf = new char[1024];
	memset(buf, 0, 1024);
	DWORD dwReaded = 0;
	pIPC->Read(buf, 1024, dwReaded);
	LOG_INFOA("buf = %s", buf);

	LPCSTR bufOut = "Hello,client";
	memset(buf, 0, 1024);
	memcpy(buf, bufOut, strlen(bufOut));
	pIPC->Write((void*)buf, strlen(buf));

	delete[] buf;
	buf = NULL;

	pIPC->Close();
	delete pIPC;
	pIPC = NULL;
}

int main()
{
	Init();

	// 启动本地监听服务
	InetSocketAddress address("127.0.0.1", 8888);
	ServerSocket serverSocket;
	serverSocket.Bind(address, 50);

	IThread* pCmdThread = new CThreadImpl(CmdThreadProc, NULL);
	pCmdThread->Start();

	// 初始化代理规则
	redirectRuleList.AddRules("120.53.233.203", 1234, "127.0.0.1", 8888);

	while (TRUE)
	{
		CSocketChannel* pClientChannel = serverSocket.Accept();
		IThread* pThread = new CThreadImpl(ThreadProc, pClientChannel);
		pThread->Start();
	}


	UnInit();
}