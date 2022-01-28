#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#include <winsock2.h>
#pragma comment(lib, "Ws2_32.lib")


class InetSocketAddress
{
public:
	InetSocketAddress(LPCSTR address,USHORT port);
	virtual ~InetSocketAddress();

	ULONG GetAddress()const;
	USHORT GetPort()const;
private:
	ULONG m_address;
	USHORT m_port;
};

class CSocketChannel
{
public:
	static CSocketChannel* Create(int af, int type, int protocol);
	static CSocketChannel* Create(SOCKET socket);
	static void Release(CSocketChannel* pChannel);
	int Read(LPSTR buf,int bufSize);
	int Write(LPCSTR buf,int bufSize);
	int Close();
protected:
	CSocketChannel(int af, int type, int protocol);
	CSocketChannel(SOCKET hSocket);
	virtual ~CSocketChannel();
	SOCKET Socket();
private:
	SOCKET m_hSocket;
};

class CServerSocketChannel : public CSocketChannel
{
public:
	CServerSocketChannel(int af, int type, int protocol);
	virtual ~CServerSocketChannel();

	int Bind(const InetSocketAddress& local, int backlog);
	CSocketChannel* Accept();
};

class ServerSocket
{
public:
	ServerSocket();
	virtual ~ServerSocket();

	int Bind(const InetSocketAddress& local, int backlog);
	CSocketChannel* Accept();
private:
	CServerSocketChannel* m_pListenChannel;
};



