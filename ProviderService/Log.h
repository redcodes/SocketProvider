#pragma once
#include "CTaskQueue.h"
#include <atlstr.h>

#ifndef _UNICODE
#define __TFILE__ __FILE__
#define __TFUNCTIONNAME__ __FUNCTION__
#else
#ifdef _MSC_VER
#define WIDEN(x) L ## x
#endif
#define WIDEN_HELPER(x) WIDEN(x)
#define __TFILE__ WIDEN_HELPER(__FILE__)
#define __TFUNCTIONNAME__ WIDEN_HELPER(__FUNCTION__)
#endif

void LOG_FORMATW(const WCHAR* fileName, int line, const WCHAR* format, ...);
void LOG_FORMATA(const CHAR* fileName, int line, const CHAR* format, ...);

#ifdef _UNICODE
#define LOG_INFO(format, ...) \
			LOG_FORMATW(PathFindFileName(__TFILE__),__LINE__,format, __VA_ARGS__)

#define LOG_INFOA(format, ...) \
			LOG_FORMATA(PathFindFileNameA(__FILE__),__LINE__,format, __VA_ARGS__)

#else
#define LOG_INFO LOG_INFOA
#define LOG_INFOW LOG_INFOW
#endif

class CLogTask : public CTask
{
public:
	CLogTask(LPCTSTR log);
	CLogTask(LPCSTR log);
	virtual ~CLogTask();

	virtual int Execute() override;
	virtual int Cancel() override;
private:
	CString m_sLog;
};
