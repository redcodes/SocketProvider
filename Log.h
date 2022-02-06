#pragma once
#include <stdarg.h>
#include <stdio.h>
#include <tchar.h>

#define PUT_PUT_DEBUG_BUF_LEN   1024
static void LOG_INFOA(const char* log, ...)
{
	char strBuffer[PUT_PUT_DEBUG_BUF_LEN] = { 0 };

	va_list vlArgs;
	va_start(vlArgs, log);
	_vsnprintf_s(strBuffer, sizeof(strBuffer) - 1, log, vlArgs);
	va_end(vlArgs);

	OutputDebugStringA(strBuffer);
}

static void LOG_INFOW(const WCHAR* strOutputString, ...)
{
	WCHAR strBuffer[PUT_PUT_DEBUG_BUF_LEN] = { 0 };

	va_list vlArgs;
	va_start(vlArgs, strOutputString);
	_vsnwprintf_s(strBuffer, ARRAYSIZE(strBuffer) - 1, ARRAYSIZE(strBuffer) - 1, strOutputString, vlArgs);
	va_end(vlArgs);

	OutputDebugStringW(strBuffer);
}

#ifdef _UNICODE
#define LOG_INFO LOG_INFOW
#define LOG_INFOA LOG_INFOA
#else
#define LOG_INFO LOG_INFOA
#define LOG_INFOW LOG_INFOW
#endif
