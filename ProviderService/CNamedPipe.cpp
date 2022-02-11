#include "CNamedPipe.h"
#include <assert.h>
#include "Log.h"

#define CONNECTING_STATE 0 
#define READING_STATE 1 
#define WRITING_STATE 2 

CNamedPipe::CNamedPipe()
{
	m_pipeCount = 0;
	m_hEvents = NULL;
	m_pipes = NULL;
}

CNamedPipe::~CNamedPipe()
{
	
}

int CNamedPipe::Bind(LPCTSTR name, int backlog, IPCCALLBACK callback, void* param)
{
	m_callback = callback;
	m_callbackParam = param;

	m_pipeCount = backlog;
	m_pipes = new PIPEINST[backlog];
	m_hEvents = new HANDLE[backlog];
	memset(m_pipes, 0, sizeof(PIPEINST) * backlog);
	memset(m_hEvents,0,sizeof(HANDLE) * backlog);

	for (int i = 0;i< backlog;i++)
	{
		m_pipes[i].hPipe = INVALID_HANDLE_VALUE;
		m_hEvents[i] = CreateEvent(NULL, TRUE, TRUE, NULL);
		Listen(&m_pipes[i], name, m_hEvents[i]);
	}

	RunLoop(INFINITE);
	return 0;
}

int CNamedPipe::Connect(LPCTSTR name, DWORD timeout /*= INFINITE*/)
{
	m_pipeCount = 1;
	m_pipes = new PIPEINST[m_pipeCount];
	m_hEvents = new HANDLE[m_pipeCount];
	m_pipes[0].hPipe = INVALID_HANDLE_VALUE;
	m_hEvents[0] = NULL;

	m_pipes[0].hPipe = CreateFile(
		name,			// pipe name 
		GENERIC_READ |  // read and write access 
		GENERIC_WRITE,
		0,              // no sharing 
		NULL,           // default security attributes
		OPEN_EXISTING,  // opens existing pipe 
		0,              // default attributes 
		NULL);          // no template file 

	if (INVALID_HANDLE_VALUE != m_pipes[0].hPipe)
		return 0;

	return GetLastError();
}

int CNamedPipe::Close()
{
	for (int i = 0; i < m_pipeCount; i++)
	{
		HANDLE hEvent = m_hEvents[i];
		if (NULL != hEvent)
		{
			SetEvent(hEvent);
			WaitForSingleObject(hEvent, INFINITE);
			CloseHandle(hEvent);
			m_hEvents[i] = NULL;
		}

		HANDLE hPipe = m_pipes[i].hPipe;
		if (INVALID_HANDLE_VALUE != hPipe)
		{
			CloseHandle(hPipe);
			m_pipes[i].hPipe = INVALID_HANDLE_VALUE;
		}
	}

	Clear();

	return 0;
}

int CNamedPipe::Write(void* buf, DWORD bufSize)
{
	DWORD dwWrited = 0;
	BOOL bOk = WriteFile(m_pipes[0].hPipe, buf, bufSize, &dwWrited, NULL);

	if (bOk && dwWrited == bufSize)
		return 0;

	return GetLastError();
}

int CNamedPipe::Read(void* buf, DWORD bufSize, DWORD& readed)
{
	BOOL bOk = ReadFile(m_pipes[0].hPipe, buf, bufSize, &readed, NULL);

	if (bOk)
		return 0;

	return GetLastError();
}

int CNamedPipe::Listen(PIPEINST* pipe,LPCTSTR name,HANDLE hEvent)
{
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = FALSE;      //non inheritable handle, same as default
	sa.lpSecurityDescriptor = 0;    //default security descriptor

	pipe->hPipe = CreateNamedPipe(
		name, // pipe name
		PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,       // read/write access
		PIPE_TYPE_MESSAGE |          // byte type pipe
		PIPE_READMODE_BYTE |      // byte-read mode
		PIPE_WAIT,                // blocking mode
		PIPE_UNLIMITED_INSTANCES, // max. instances
		BUFSIZE,                  // output buffer size
		BUFSIZE,                  // input buffer size
		3000,                     // client time-out
		NULL);

	if (INVALID_HANDLE_VALUE == pipe->hPipe)
		return GetLastError();

	memset(&pipe->oOverlap, 0, sizeof(OVERLAPPED));
	pipe->oOverlap.hEvent = hEvent;

	if (!ConnectNamedPipe(pipe->hPipe, &pipe->oOverlap))
	{
		switch (GetLastError()) {
		case ERROR_IO_PENDING:
			pipe->fPendingIO = true;
			break;
		case ERROR_PIPE_CONNECTED:
			pipe->fPendingIO = false;
			break;
		default:
			CloseHandle(pipe->hPipe);
			pipe->hPipe = INVALID_HANDLE_VALUE;
			return GetLastError();
		}
	}
	else
	{
		// ·¢ÉúÁË´íÎó
		SetEvent(pipe->oOverlap.hEvent);
		return GetLastError();
	}

	pipe->dwState = pipe->fPendingIO ?
		CONNECTING_STATE : // still connecting 
		READING_STATE;     // ready to read 

	return 0;
}

int CNamedPipe::RunLoop(DWORD dwTimeout /*= INFINITE*/)
{
	DWORD dwWait = 0;
	BOOL fSuccess = FALSE;
	DWORD cbRet = 0;
	DWORD dwErr = 0;

	while (TRUE)
	{
		dwWait = WaitForMultipleObjects(m_pipeCount, m_hEvents, FALSE, dwTimeout);
		int i = dwWait - WAIT_OBJECT_0;
		if (i < 0 || i >(m_pipeCount - 1))
		{
			LOG_INFO(_T("pipe Index out of range"));
			return -1;
		}

		PIPEINST* pPipe = &m_pipes[i];
		if (pPipe->fPendingIO)
		{
			fSuccess = GetOverlappedResult(
				pPipe->hPipe, // handle to pipe 
				&pPipe->oOverlap, // OVERLAPPED structure 
				&cbRet,            // bytes transferred 
				FALSE);            // do not wait 

			switch (pPipe->dwState)
			{
				// Pending connect operation 
			case CONNECTING_STATE:
				if (!fSuccess)
				{
					LOG_INFO(_T("Error %d"), GetLastError());
					return 0;
				}
				pPipe->dwState = READING_STATE;
				break;

				// Pending read operation 
			case READING_STATE:
				if (!fSuccess || cbRet == 0)
				{
					DisconnectAndReconnect(pPipe);
					continue;
				}
				pPipe->cbRead = cbRet;
				pPipe->dwState = WRITING_STATE;
				break;

				// Pending write operation 
			case WRITING_STATE:
				if (!fSuccess || cbRet != pPipe->cbToWrite)
				{
					DisconnectAndReconnect(pPipe);
					continue;
				}
				pPipe->dwState = READING_STATE;
				break;

			default:
			{
				LOG_INFO(_T("Invalid pipe state"));
				return 0;
			}
			}
		}

		switch (pPipe->dwState)
		{
			// READING_STATE: 
			// The pipe instance is connected to the client 
			// and is ready to read a request from the client. 

		case READING_STATE:
			fSuccess = ReadFile(
				pPipe->hPipe,
				pPipe->chRequest,
				BUFSIZE,
				&pPipe->cbRead,
				&pPipe->oOverlap);

			// The read operation completed successfully. 

			if (fSuccess && pPipe->cbRead != 0)
			{
				pPipe->fPendingIO = FALSE;
				pPipe->dwState = WRITING_STATE;
				continue;
			}

			// The read operation is still pending. 

			dwErr = GetLastError();
			if (!fSuccess && (dwErr == ERROR_IO_PENDING))
			{
				pPipe->fPendingIO = TRUE;
				continue;
			}

			// An error occurred; disconnect from the client. 

			DisconnectAndReconnect(pPipe);
			break;

			// WRITING_STATE: 
			// The request was successfully read from the client. 
			// Get the reply data and write it to the client. 

		case WRITING_STATE:

			if (NULL != m_callback)
				m_callback(pPipe->chRequest, pPipe->cbRead, pPipe->chReply, pPipe->cbToWrite,m_callbackParam);		

			fSuccess = WriteFile(
				pPipe->hPipe,
				pPipe->chReply,
				pPipe->cbToWrite,
				&cbRet,
				&pPipe->oOverlap);

			// The write operation completed successfully. 

			if (fSuccess && cbRet == pPipe->cbToWrite)
			{
				pPipe->fPendingIO = FALSE;
				pPipe->dwState = READING_STATE;
				continue;
			}

			// The write operation is still pending. 

			dwErr = GetLastError();
			if (!fSuccess && (dwErr == ERROR_IO_PENDING))
			{
				pPipe->fPendingIO = TRUE;
				continue;
			}

			// An error occurred; disconnect from the client. 

			DisconnectAndReconnect(pPipe);
			break;

		default:
		{
			LOG_INFO(_T("Invalid pipe state"));
			return 0;
		}
		}
	}
}

void CNamedPipe::DisconnectAndReconnect(PIPEINST* pipe)
{
	// Disconnect the pipe instance. 

	if (!DisconnectNamedPipe(pipe->hPipe))
	{
		printf("DisconnectNamedPipe failed with %d.\n", GetLastError());
	}

	// Call a subroutine to connect to the new client. 

	pipe->fPendingIO = ConnectToNewClient(
		pipe->hPipe,
		&pipe->oOverlap);

	pipe->dwState = pipe->fPendingIO ?
		CONNECTING_STATE : // still connecting 
		READING_STATE;     // ready to read 
}

BOOL CNamedPipe::ConnectToNewClient(HANDLE hPipe, LPOVERLAPPED lpo)
{
	BOOL fConnected, fPendingIO = FALSE;

	// Start an overlapped connection for this pipe instance. 
	fConnected = ConnectNamedPipe(hPipe, lpo);

	// Overlapped ConnectNamedPipe should return zero. 
	if (fConnected)
	{
		printf("ConnectNamedPipe failed with %d.\n", GetLastError());
		return 0;
	}

	switch (GetLastError())
	{
		// The overlapped connection in progress. 
	case ERROR_IO_PENDING:
		fPendingIO = TRUE;
		break;

		// Client is already connected, so signal an event. 

	case ERROR_PIPE_CONNECTED:
		if (SetEvent(lpo->hEvent))
			break;

		// If an error occurs during the connect operation... 
	default:
	{
		printf("ConnectNamedPipe failed with %d.\n", GetLastError());
		return 0;
	}
	}

	return fPendingIO;
}


void CNamedPipe::Clear()
{
	delete[] m_hEvents;
	m_hEvents = NULL;

	delete[] m_pipes;
	m_pipes = NULL;
}
