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

InetSocketAddress::InetSocketAddress(ULONG ip, USHORT port)
{
	m_address = ip;
	m_port = port;
}

InetSocketAddress::~InetSocketAddress()
{

}

InetSocketAddress& InetSocketAddress::operator=(const InetSocketAddress& address)
{
	if (&address == this)
		return *this;

	this->m_address = address.GetAddress();
	this->m_port = address.GetPort();

	return *this;
}

ULONG InetSocketAddress::GetAddress() const
{
	return m_address;
}

USHORT InetSocketAddress::GetPort() const
{
	return m_port;
}

std::string InetSocketAddress::ToString()
{
	in_addr in;
	memcpy(&in, &m_address, 4);
	char* ip = inet_ntoa(in);

	USHORT port = ntohs(m_port);

	char address[20] = { 0 };
	sprintf_s(address, "%s:%d",ip,port);

	return address;
}

CServerSocketChannel::CServerSocketChannel(int af, int type, int protocol)
	:CSocketChannel(af, type, protocol)
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

CSocketChannel* CSocketChannel::Create(int af, int type, int protocol)
{
	CSocketChannel* pChannel = new CSocketChannel(af, type, protocol);
	return pChannel;
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
	size_t    nleft;
	size_t   nwritten;

	nleft = bufSize;
	while (nleft > 0)
	{
		if ((nwritten = ::send(m_hSocket, buf, nleft, 0)) < 0)
		{
			if (nleft == SOCKET_ERROR)
				return -1;    /* error, return -1 */
			else
				break;        /* error, return amount written so far */
		}
		else if (nwritten == 0)
		{
			break;
		}
		nleft -= nwritten;
		buf += nwritten;
	}

	return(bufSize - nleft);


	//	return ::send(m_hSocket, buf, bufSize, 0);
}

int CSocketChannel::SendTo(const InetSocketAddress& address, LPCSTR buf, int bufSize)
{
	sockaddr_in remote = { 0 };
	remote.sin_family = AF_INET;
	remote.sin_addr.S_un.S_addr = address.GetAddress();
	remote.sin_port = address.GetPort();

	int nSize = sizeof(remote);

	return ::sendto(m_hSocket, buf, bufSize, 0, (sockaddr*)&remote, nSize);
}

int CSocketChannel::RecvFrom(InetSocketAddress& address, LPSTR buf, int bufSize)
{
	sockaddr_in remote;
	int nSize = sizeof(remote);

	int result = ::recvfrom(m_hSocket, buf, bufSize, 0, (sockaddr*)&remote, &nSize);

	ULONG fromIP = ((sockaddr_in*)&remote)->sin_addr.S_un.S_addr;
	USHORT fromPort = ((sockaddr_in*)&remote)->sin_port;

	InetSocketAddress from(fromIP, fromPort);
	address = from;

	return result;
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

CClientSocketChannel::CClientSocketChannel(int af, int type, int protocol) : CSocketChannel(af, type, protocol)
{

}

CClientSocketChannel::~CClientSocketChannel()
{

}

int CClientSocketChannel::Connect(const InetSocketAddress& local)
{
	sockaddr_in server_addr = { 0 };
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = local.GetAddress();
	server_addr.sin_port = local.GetPort();

	if (SOCKET_ERROR == ::connect(Socket(), (struct sockaddr*)&server_addr, sizeof(server_addr)))
		return WSAGetLastError();

	return 0;
}

UDPServerSocket::UDPServerSocket() : CSocketChannel(AF_INET, SOCK_DGRAM, 0)
{

}

UDPServerSocket::~UDPServerSocket()
{

}

int UDPServerSocket::Bind(const InetSocketAddress& local, int backlog)
{
	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = local.GetPort();
	sin.sin_addr.S_un.S_addr = local.GetAddress();

	if ((::bind(Socket(), (LPSOCKADDR)&sin, sizeof(sin)) == SOCKET_ERROR))
	{
		return WSAGetLastError();
	}

	return 0;
}
