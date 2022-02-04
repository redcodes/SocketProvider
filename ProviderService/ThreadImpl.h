#pragma once
#include "IThread.h"

typedef void(* CallbackFunc)(void* pParam);
struct THREAD_CALLBACK_PARAM
{
	CallbackFunc pCallbackFunc;
	void* pCallbackParam;
};

class CThreadImpl : public IThread
{
public:
	CThreadImpl(IRunnable* pRunnable);
	CThreadImpl(CallbackFunc pCallback,void* pParam);
	virtual ~CThreadImpl(void);

	virtual int Start();
	virtual void Stop();
	virtual int Join(int nTimeout = INFINITE);
protected:
	virtual unsigned int Run();
private:
	static unsigned int __stdcall ThreadProc(void* pParam);
	IRunnable* m_pRunnable;
	HANDLE m_hThread;
	THREAD_CALLBACK_PARAM* m_pCallbackParam;
};

