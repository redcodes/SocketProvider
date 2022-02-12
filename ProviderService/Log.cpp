#include "Log.h"
#include "CTaskQueueImpl.h"
#include <tchar.h>
#include <stdio.h>
#include <stdarg.h>
#include <Shlwapi.h>
#pragma comment(lib,"Shlwapi.lib")

#define PUT_PUT_DEBUG_BUF_LEN   1024
static CTaskQueueImpl logQueue;

CLogTask::CLogTask(LPCTSTR log) : CTask()
{
	m_sLog = log;
}

CLogTask::CLogTask(LPCSTR log)
{
	USES_CONVERSION;
	m_sLog = A2W(log);
}

CLogTask::~CLogTask()
{

}

int CLogTask::Execute()
{
	_tprintf(m_sLog);
	_tprintf(_T("\r\n"));

	//	OutputDebugString(m_sLog);

	return 0;
}

int CLogTask::Cancel()
{
	return 0;
}

void LOG_FORMATW(const WCHAR* fileName, int line, const WCHAR* format, ...)
{
	WCHAR strBuffer[PUT_PUT_DEBUG_BUF_LEN] = { 0 };

	va_list vlArgs;
	va_start(vlArgs, format);
	_vsnwprintf_s(strBuffer, ARRAYSIZE(strBuffer) - 1, ARRAYSIZE(strBuffer) - 1, format, vlArgs);
	va_end(vlArgs);

	SYSTEMTIME stTime = { 0 };
	GetLocalTime(&stTime);

	CString sLog;
	sLog.Format(_T("[P%04d] [T%04d] [%04d-%02d-%02d %02d:%02d:%02d:%03d] [%s:%d]: %s\r\n"),
		(int)GetCurrentProcessId(),
		(int)GetCurrentThreadId(),
		(int)stTime.wYear,
		(int)stTime.wMonth,
		(int)stTime.wDay,
		(int)stTime.wHour,
		(int)stTime.wMinute,
		(int)stTime.wSecond,
		(int)stTime.wMilliseconds,
		fileName,
		line,
		strBuffer);

	logQueue.AddTask(new CLogTask(sLog));
	if (!logQueue.IsRunning())
		logQueue.Start();
}

void LOG_FORMATA(const CHAR* fileName, int line, const CHAR* format, ...)
{
	CHAR strBuffer[PUT_PUT_DEBUG_BUF_LEN] = { 0 };

	va_list vlArgs;
	va_start(vlArgs, format);
	_vsnprintf_s(strBuffer, sizeof(strBuffer) - 1, format, vlArgs);
	va_end(vlArgs);

	SYSTEMTIME stTime = { 0 };
	GetLocalTime(&stTime);

	CStringA sLog;
	sLog.Format(("[P%04d] [T%04d] [%04d-%02d-%02d %02d:%02d:%02d:%03d] [%s:%d]: %s\r\n"),
		(int)GetCurrentProcessId(),
		(int)GetCurrentThreadId(),
		(int)stTime.wYear,
		(int)stTime.wMonth,
		(int)stTime.wDay,
		(int)stTime.wHour,
		(int)stTime.wMinute,
		(int)stTime.wSecond,
		(int)stTime.wMilliseconds,
		fileName,
		line,
		strBuffer);

	logQueue.AddTask(new CLogTask(sLog));
	if (!logQueue.IsRunning())
		logQueue.Start();
}

