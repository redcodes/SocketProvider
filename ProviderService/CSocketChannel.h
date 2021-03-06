#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#include <winsock2.h>
#pragma comment(lib, "Ws2_32.lib")
#include<iostream>

class InetSocketAddress
{
public:
	InetSocketAddress(LPCSTR address,USHORT port);
	InetSocketAddress(ULONG ip, USHORT port);
	virtual ~InetSocketAddress();

	InetSocketAddress& operator=(const InetSocketAddress& address);

	ULONG GetAddress()const;
	USHORT GetPort()const;

	std::string ToString();
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

	int SendTo(const InetSocketAddress& address, LPCSTR buf, int bufSize);
	int RecvFrom(InetSocketAddress& address, LPSTR buf, int bufSize);

	int Close();	
	SOCKET Socket();
protected:
	CSocketChannel(int af, int type, int protocol);
	CSocketChannel(SOCKET hSocket);
	virtual ~CSocketChannel();
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
	InetSocketAddress GetLocalAddress()const;

};

class CClientSocketChannel : public CSocketChannel
{
public:
	CClientSocketChannel(int af, int type, int protocol);
	virtual ~CClientSocketChannel();

	int Connect(const InetSocketAddress& local);
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

class UDPServerSocket : public CSocketChannel
{
public:
	UDPServerSocket();
	virtual ~UDPServerSocket();

	int Bind(const InetSocketAddress& local, int backlog);
};



