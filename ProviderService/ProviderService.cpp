// ProviderService.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "CSocketChannel.h"
#include "ThreadImpl.h"
#include "../CSocketRedirectRules.h"
#include "../Log.h"

LPCSTR hello = "hello,world";
CSocketRedirectRules redirectRuleList;

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
		pLocalChannel->Read((LPSTR)appAddress,sizeof(appAddress));

		ULONG remoteIP = 0;
		USHORT remotePort = 0;

		memcpy(&remoteIP, appAddress, sizeof(ULONG));
		memcpy(&remotePort, appAddress + sizeof(ULONG), sizeof(USHORT));

		// 找到代理规则，则开始收发数据
		ULONG localProxyIP = 0;
		USHORT localProxyPort = 0;
		if (0 != redirectRuleList.FindRules(remoteIP, remotePort,localProxyIP,localProxyPort))
			break;

		// 连接真实应用服务器	
		InetSocketAddress remoteAddress(remoteIP, remotePort);
		pRemoteChannel = new CClientSocketChannel(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (0 != pRemoteChannel->Connect(remoteAddress))
			break;

		while (TRUE)
		{
			byte recvBuf[4096] = { 0 };
			int nReadLength = pLocalChannel->Read((LPSTR)recvBuf, sizeof(recvBuf));
			if (nReadLength <= 0)
			{
				LOG_INFO(_T("read failed"));
				break;
			}

			int nWriteLength = pRemoteChannel->Write((LPSTR)recvBuf, nReadLength);
			if (nWriteLength <= 0)
			{
				LOG_INFO(_T("write failed"));
				break;
			}
		}

	} while (FALSE);

	pRemoteChannel->Close();
	CSocketChannel::Release(pRemoteChannel);

	pLocalChannel->Close();
	CSocketChannel::Release(pLocalChannel);
}

int main()
{
	Init();

	// 启动本地监听服务
	InetSocketAddress address("127.0.0.1", 8888);
	ServerSocket serverSocket;
	serverSocket.Bind(address, 50);

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