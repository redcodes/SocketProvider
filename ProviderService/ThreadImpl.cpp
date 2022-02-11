#include "ThreadImpl.h"
#include <process.h>

CThreadImpl::CThreadImpl(IRunnable* pRunnable) : m_pRunnable(pRunnable),m_hThread(NULL)
{
	m_pCallbackParam = NULL;
	m_hThread = NULL;
}

CThreadImpl::CThreadImpl(CallbackFunc pCallback,void* pParam) : m_pRunnable(NULL)
	,m_pCallbackParam(NULL)
	,m_hThread(NULL)
{
	m_pCallbackParam = new THREAD_CALLBACK_PARAM;
	m_pCallbackParam->pCallbackFunc = pCallback;
	m_pCallbackParam->pCallbackParam = pParam;
}

CThreadImpl::~CThreadImpl(void)
{
	if (NULL != m_pCallbackParam)
		delete m_pCallbackParam;
	m_pCallbackParam = NULL;
}

int CThreadImpl::Start()
{
	if (NULL == m_hThread)
		m_hThread = (HANDLE)_beginthreadex(NULL, 0, ThreadProc, this, 0, NULL);

	return 0;
}

void CThreadImpl::Stop()
{
	if (NULL != m_pRunnable)
		m_pRunnable->Stop();

	Join(INFINITE);

	if (NULL != m_hThread)
		CloseHandle(m_hThread);
	m_hThread = NULL;
}

int CThreadImpl::Join(int nTimeout /*= INFINITE*/)
{
	if (NULL != m_hThread)
		return ::WaitForSingleObject(m_hThread,nTimeout);

	return -1;
}

unsigned int CThreadImpl::Run()
{
	if (NULL != m_pRunnable)
		m_pRunnable->Run();

	if (NULL != m_pCallbackParam)
	{
		CallbackFunc callbackFunc = m_pCallbackParam->pCallbackFunc;
		void* pCallbackParam = m_pCallbackParam->pCallbackParam;
		if (NULL != callbackFunc)
			callbackFunc(pCallbackParam);
	}

	return 0;
}

unsigned int __stdcall CThreadImpl::ThreadProc(void* pParam)
{
	CThreadImpl* pThis = (CThreadImpl*)pParam;
	if (NULL != pThis)
		return pThis->Run();

	return -1;
}
