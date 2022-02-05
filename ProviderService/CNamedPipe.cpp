#include "CNamedPipe.h"
#include <assert.h>
const int BUFSIZE = 4096;

CNamedPipe::CNamedPipe()
{
	m_hPipe = INVALID_HANDLE_VALUE;
	memset(&m_ov, 0, sizeof(OVERLAPPED));
	m_hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

CNamedPipe::~CNamedPipe()
{
	Close();
}

int CNamedPipe::Bind(LPCTSTR name, int backlog)
{
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = FALSE;      //non inheritable handle, same as default
	sa.lpSecurityDescriptor = 0;    //default security descriptor

	m_hPipe = CreateNamedPipe(
		name, // pipe name
		PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,       // read/write access
		PIPE_TYPE_BYTE |          // byte type pipe
		PIPE_READMODE_BYTE |      // byte-read mode
		PIPE_WAIT,                // blocking mode
		PIPE_UNLIMITED_INSTANCES, // max. instances
		BUFSIZE,                  // output buffer size
		BUFSIZE,                  // input buffer size
		3000,                     // client time-out
		NULL);

	if (INVALID_HANDLE_VALUE == m_hPipe)
		return GetLastError();

	memset(&m_ov, 0, sizeof(OVERLAPPED));
	m_ov.hEvent = m_hEvent;

	bool connected = false;
	if (!ConnectNamedPipe(m_hPipe, &m_ov))
	{
		switch (GetLastError()) {
		case ERROR_IO_PENDING:
			connected = false;
			break;
		case ERROR_PIPE_CONNECTED:
			connected = true;
			break;
		default:
			CloseHandle(m_hPipe);
			m_hPipe = INVALID_HANDLE_VALUE;
			return GetLastError();
		}
	}
	else
	{
		// ∑¢…˙¡À¥ÌŒÛ
		SetEvent(m_hEvent);
		return GetLastError();
	}

	WaitForSingleObject(m_hEvent, INFINITE);

	if (!connected)
	{
		DWORD dwTrans = 0;
		GetOverlappedResult(m_hPipe, &m_ov, &dwTrans, FALSE);
	}
	return 0;
}

int CNamedPipe::Connect(LPCTSTR name, DWORD timeout /*= INFINITE*/)
{
	assert(m_hPipe == INVALID_HANDLE_VALUE);

	m_hPipe = CreateFile(
		name,			// pipe name 
		GENERIC_READ |  // read and write access 
		GENERIC_WRITE,
		0,              // no sharing 
		NULL,           // default security attributes
		OPEN_EXISTING,  // opens existing pipe 
		0,              // default attributes 
		NULL);          // no template file 

	if (INVALID_HANDLE_VALUE != m_hPipe)
		return 0;

	return GetLastError();
}

int CNamedPipe::Close()
{
	if (NULL != m_hEvent)
	{
		SetEvent(m_hEvent);
		WaitForSingleObject(m_hEvent, INFINITE);
		CloseHandle(m_hEvent);
		m_hEvent = NULL;
	}

	if (INVALID_HANDLE_VALUE != m_hPipe)
	{
		CloseHandle(m_hPipe);
		m_hPipe = INVALID_HANDLE_VALUE;
	}

	return 0;
}

int CNamedPipe::Write(void* buf, DWORD bufSize)
{
	DWORD dwWrited = 0;
	BOOL bOk = WriteFile(m_hPipe, buf, bufSize, &dwWrited, NULL);

	if (bOk && dwWrited == bufSize)
		return 0;

	return GetLastError();
}

int CNamedPipe::Read(void* buf, DWORD bufSize, DWORD& readed)
{
	BOOL bOk = ReadFile(m_hPipe, buf, bufSize, &readed, NULL);

	if (bOk)
		return 0;

	return GetLastError();
}
