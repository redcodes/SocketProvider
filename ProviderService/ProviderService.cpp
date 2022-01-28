// ProviderService.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "CSocketChannel.h"

LPCSTR hello = "hello,world";
int main()
{
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(sockVersion, &wsaData) != 0) 
    {
		return false;
	}

    InetSocketAddress address("127.0.0.1", 8888);
    ServerSocket serverSocket;
    serverSocket.Bind(address,50);

    CSocketChannel* pClientChannel = serverSocket.Accept();
    pClientChannel->Write(hello, strlen(hello));

	char recvBuf[20] = { 0 };
	pClientChannel->Read(recvBuf, sizeof(recvBuf));
	printf("%s",recvBuf);

    pClientChannel->Close();
	CSocketChannel::Release(pClientChannel);

	int ret = WSACleanup();
	if (ret != 0) {
		return false;
	}

}