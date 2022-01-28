#include "CSocketChannel.h"

ServerSocket::ServerSocket()
{
	m_pListenChannel = new CServerSocketChannel(AF_INET, SOCK_STREAM, IPPROTO_TCP);
}

ServerSocket::~ServerSocket()
{
	if (NULL != m_pListenChannel)
	{
		m_pListenChannel->Close();
		delete m_pListenChannel;
		m_pListenChannel = NULL;
	}
}

int ServerSocket::Bind(const InetSocketAddress& local, int backlog)
{
	return m_pListenChannel->Bind(local, backlog);
}

CSocketChannel* ServerSocket::Accept()
{
	return m_pListenChannel->Accept();
}

InetSocketAddress::InetSocketAddress(LPCSTR address, USHORT port)
{
	m_address = inet_addr(address);
	m_port = htons(port);
}

InetSocketAddress::~InetSocketAddress()
{

}

ULONG InetSocketAddress::GetAddress() const
{
	return m_address;
}

USHORT InetSocketAddress::GetPort() const
{
	return m_port;
}

CServerSocketChannel::CServerSocketChannel(int af, int type, int protocol)
	:CSocketChannel(af,type,protocol)
{

}

CServerSocketChannel::~CServerSocketChannel()
{

}

int CServerSocketChannel::Bind(const InetSocketAddress& local, int backlog)
{
	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = local.GetPort();
	sin.sin_addr.S_un.S_addr = local.GetAddress();

	if ((::bind(Socket(), (LPSOCKADDR)&sin, sizeof(sin)) == SOCKET_ERROR))
	{
		return WSAGetLastError();
	}

	if (::listen(Socket(), backlog) == SOCKET_ERROR)
	{
		Close();
		return WSAGetLastError();
	}

	return 0;
}

CSocketChannel* CServerSocketChannel::Accept()
{
	struct sockaddr_in remoteAddr;
	int nAddrlen = sizeof(remoteAddr);

	SOCKET hClient = ::accept(Socket(), (SOCKADDR*)&remoteAddr, &nAddrlen);
	CSocketChannel* pClientChannel = CSocketChannel::Create(hClient);

	return pClientChannel;
}

CSocketChannel* CSocketChannel::Create(SOCKET socket)
{
	return new CSocketChannel(socket);
}

void CSocketChannel::Release(CSocketChannel* pChannel)
{
	if (NULL != pChannel)
	{
		delete pChannel;
		pChannel = NULL;
	}
}

int CSocketChannel::Read(LPSTR buf, int bufSize)
{
	return ::recv(m_hSocket, buf, bufSize, 0);
}

int CSocketChannel::Write(LPCSTR buf, int bufSize)
{
	return ::send(m_hSocket, buf, bufSize, 0);
}

int CSocketChannel::Close()
{
	if (INVALID_SOCKET != m_hSocket)
	{
		::closesocket(m_hSocket);
		m_hSocket = INVALID_SOCKET;
	}

	return 0;
}

CSocketChannel::CSocketChannel(int af, int type, int protocol)
{
	m_hSocket = ::socket(af, type, protocol);
}

CSocketChannel::CSocketChannel(SOCKET hSocket)
{
	m_hSocket = hSocket;
}

CSocketChannel::~CSocketChannel()
{
	Close();
}

SOCKET CSocketChannel::Socket()
{
	return m_hSocket;
}
