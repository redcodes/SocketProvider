// ProviderService.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "CSocketChannel.h"
#include "ThreadImpl.h"
#include "../CSocketRedirectRules.h"
#include "Log.h"
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

		fd_set readfds, writefds, exceptfds;
		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		FD_ZERO(&exceptfds);

		FD_SET(pLocalChannel->Socket(), &readfds);
		FD_SET(pRemoteChannel->Socket(), &readfds);

		FD_SET(pLocalChannel->Socket(), &exceptfds);
		FD_SET(pRemoteChannel->Socket(), &exceptfds);

		int selectResult = 0;
		fd_set readfdsOld, writefdsOld, exceptfdsOld;

		int nReadLength = 0;
		int nWriteLength = 0;

		bool bLoop = true;
		while (bLoop)
		{
			byte recvBuf[4096] = { 0 };

			readfdsOld = readfds;
			writefdsOld = writefds;
			exceptfdsOld = exceptfds;

			selectResult = select(0, &readfdsOld, &writefdsOld, &exceptfdsOld, NULL);
			if (selectResult <= 0)
			{
				LOG_INFO(_T("select error = %d"), WSAGetLastError());
				break;
			}

			for (DWORD i = 0; i < readfds.fd_count; i++)
			{
				// 本地应用数据
				if (FD_ISSET(readfds.fd_array[i], &readfdsOld))
				{
					if (readfds.fd_array[i] == pLocalChannel->Socket())
					{
						nReadLength = pLocalChannel->Read((LPSTR)recvBuf, sizeof(recvBuf));
						if (nReadLength <= 0)
						{
							if (nReadLength == 0)
								LOG_INFO(_T("local channel close"));
							else
								LOG_INFO(_T("read failed"));

							bLoop = false;
							break;
						}

						LOG_INFOA("request = %s", recvBuf);

						nWriteLength = pRemoteChannel->Write((LPSTR)recvBuf, nReadLength);
						if (nWriteLength <= 0)
						{
							LOG_INFO(_T("write failed"));
							bLoop = false;
							break;
						}
					}

					// 远程应用服务器返回数据
					if (readfds.fd_array[i] == pRemoteChannel->Socket())
					{
						nReadLength = pRemoteChannel->Read((LPSTR)recvBuf, sizeof(recvBuf));
						if (nReadLength <= 0)
						{
							if (nReadLength == 0)
								LOG_INFO(_T("remote channel close"));
							else
								LOG_INFO(_T("read failed"));

							bLoop = false;
							break;
						}

						LOG_INFOA("response = %s", recvBuf);

						nWriteLength = pLocalChannel->Write((LPSTR)recvBuf, nReadLength);
						if (nWriteLength <= 0)
						{
							LOG_INFO(_T("write failed"));
							bLoop = false;
							break;
						}
					}
				}

				if (FD_ISSET(exceptfds.fd_array[i], &exceptfdsOld))
				{
					LOG_INFO(_T("select except"));
					break;
				}
			}
			
			if (!bLoop)
				break;
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

int CmdCallback(LPCSTR request, DWORD requestSize, LPSTR response, DWORD& responseSize, void* param)
{
	BYTE* buf = (BYTE*)request;
	BYTE* outbuf = (BYTE*)response;

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
		responseSize = 6;
		ZeroMemory(outbuf, responseSize);
		memcpy(outbuf, &destIP, sizeof(destIP));
		memcpy(outbuf + sizeof(destIP), &destPort, sizeof(destPort));
	}
	else
	{
//		ZeroMemory(outbuf, sizeof(buf));
		responseSize = 0;
	}

	return 0;
}

void CmdThreadProc(void* param)
{
	CIPC* pIPC = CIPC::GetInstance();
	pIPC->Bind(PIPENAME, 50, CmdCallback, NULL);

	return;

	while (TRUE)
	{
		CIPC* pIPC = CIPC::GetInstance();
		pIPC->Bind(PIPENAME, 50,NULL,NULL);

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
	pIPC->Bind(PIPENAME, 50, CmdCallback, NULL);

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

#include "CTaskQueueImpl.h"
class CTaskImpl : public CTask
{
public:
	CTaskImpl(DWORD value)
	{
		m_value = value;
	}

	virtual ~CTaskImpl()
	{

	}

	virtual int Execute()
	{
		LOG_INFO(_T("value = %d"),m_value);

		return 0;
	}

	virtual int Cancel()
	{
		return 0;
	}
private:
	DWORD m_value;
};

static void QueueThreadProc(void* param)
{
	CTaskQueueImpl* taskQueue = (CTaskQueueImpl*)param;

	DWORD i = 0;
	while (taskQueue->IsRunning())
	{
		CTask* pTask = new CTaskImpl(i++);
		taskQueue->AddTask(pTask);

		Sleep(1);
	}

}

#include "Log.h"
void TestTaksQueue()
{
	LOG_INFO(_T("buf = %s"), _T("Hello,World"));
	LOG_INFOA(("buf = %s"), ("Hello,World"));

	CTaskQueueImpl taskQueue;	
	taskQueue.Start();

	IThread* pThread = new CThreadImpl(QueueThreadProc, &taskQueue);
	pThread->Start();

	while (TRUE)
	{
		continue;
	}

	taskQueue.Stop();
	pThread->Stop();

	delete pThread;
	pThread = NULL;

}



int main()
{
	TestTaksQueue();
	return 0;

	Init();

// 	TestPipeServer();
// 	return 0;

	// 启动本地监听服务
	InetSocketAddress address("127.0.0.1", 8888);
	ServerSocket serverSocket;
	serverSocket.Bind(address, 50);

	IThread* pCmdThread = new CThreadImpl(CmdThreadProc, NULL);
	pCmdThread->Start();

	// 初始化代理规则
	redirectRuleList.AddRules("120.53.233.203", 1234, "127.0.0.1", 8888);
	redirectRuleList.AddRules("114.115.205.144", 80, "127.0.0.1", 8888);

	while (TRUE)
	{
		CSocketChannel* pClientChannel = serverSocket.Accept();
		IThread* pThread = new CThreadImpl(ThreadProc, pClientChannel);
		pThread->Start();
	}


	UnInit();
}